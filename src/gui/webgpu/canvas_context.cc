// GPUCanvasContext implementation. See canvas_context.h.

#include "canvas_context.h"
#include "webgpu_module.h"
#include "webgpu_bindings.h"
#include "generated/webgpu_generated.h"   // wgpu_str_to_GPUTextureFormat

#ifdef _WIN32
#include <windows.h>
#endif

extern "C" void wgpuLiveInc( const char* );
extern "C" void wgpuLiveDec( const char* );
extern "C" void wgpuLiveDump( void );
extern "C" void wgpuSetActiveSurface( WGPUSurface, WGPUTexture );
extern "C" void wgpuClearActiveSurface( void );

// ----- imglib-webgpu surface hooks ---------------------------------------
// The imglib driver registers webgpu.image and provides a per-subimage
// render-bundle composition step that runs at the end of the surface pass.
// It needs three things from the canvas binding:
//   - the surface's pixel dimensions (to convert PSI pixel coords to NDC)
//   - the swapchain color format (so render bundles use a compatible target)
//   - the sack Image that's bound as the surface's root (PSI tree walks from
//     here in webgpu_image_dispatch_surface_pass).
// Declared inline rather than via local.h to avoid pulling sack imglib
// headers into the binding. Image is opaque (void*).
namespace sack { namespace image {
	void webgpu_image_set_surface_size  ( uint32_t w, uint32_t h );
	void webgpu_image_set_surface_format( WGPUTextureFormat fmt );
	void webgpu_image_set_surface_root  ( void *root );
}}

GPUCanvasContext::GPUCanvasContext()
	: surface_( NULL ), renderer_( NULL ), configured_( false ),
	  configuredDevice_( NULL ) { wgpuLiveInc( "GPUCanvasContext" ); }

// Build a fresh WGPUSurface for our renderer. Used at construction and
// whenever configure() is called with a different device than last time
// (Vulkan's swapchain can't switch devices, so we have to recreate).
static WGPUSurface wgpu_create_surface_for_renderer( PRENDERER r ) {
	WGPUInstance instance = webgpu_get_instance();
	if( !instance || !r ) return NULL;
	WGPUSurfaceDescriptor desc = {};
#ifdef _WIN32
	WGPUSurfaceSourceWindowsHWND src = WGPU_SURFACE_SOURCE_WINDOWS_HWND_INIT;
	src.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
	src.hinstance = (void*)GetModuleHandle( NULL );
	src.hwnd      = (void*)GetNativeHandle( r );
	desc.nextInChain = &src.chain;
	// One-shot diagnostic: report HWND extended style at surface-creation
	// time. WS_EX_LAYERED=0x80000, WS_EX_NOREDIRECTIONBITMAP=0x00200000.
	// Both must be present for the underlying Vulkan/DXGI surface to
	// expose Premultiplied alpha mode (and thus per-pixel transparency).
	LONG_PTR exStyle = GetWindowLongPtr( (HWND)src.hwnd, GWL_EXSTYLE );
	lprintf( "surface hwnd=%p exStyle=0x%llX  layered=%d noredirbitmap=%d"
	       , src.hwnd
	       , (long long)exStyle
	       , (int)( ( exStyle & WS_EX_LAYERED            ) != 0 )
	       , (int)( ( exStyle & WS_EX_NOREDIRECTIONBITMAP ) != 0 ) );
#else
	(void)r;
	return NULL;
#endif
	return wgpuInstanceCreateSurface( instance, &desc );
}

GPUCanvasContext::~GPUCanvasContext() {
	if( surface_ ) {
		if( configured_ ) wgpuSurfaceUnconfigure( surface_ );
		wgpuSurfaceRelease( surface_ );
		surface_ = NULL;
	}
	wgpuLiveDec( "GPUCanvasContext" );
}

void GPUCanvasContext::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( !args.IsConstructCall() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "GPUCanvasContext is not constructible from JS" ) ) );
		return;
	}
	GPUCanvasContext* c = new GPUCanvasContext();
	c->Wrap( args.This() );
	args.GetReturnValue().Set( args.This() );
}


// ---------- configure ----------

void GPUCanvasContext::configure( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	GPUCanvasContext* self = node::ObjectWrap::Unwrap<GPUCanvasContext>( getFCIHolder( args ) );

	if( !self->surface_ ) {
		isolate->ThrowException( Exception::Error( localStringExternal(
			isolate, "GPUCanvasContext has no surface" ) ) );
		return;
	}
	if( args.Length() < 1 || !args[ 0 ]->IsObject() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "configure requires an options object" ) ) );
		return;
	}
	Local<Object> opts = args[ 0 ].As<Object>();

	WGPUSurfaceConfiguration cfg = WGPU_SURFACE_CONFIGURATION_INIT;

	// device — required, GPUDevice handle
	Local<Value> deviceVal = opts->Get( context,
		String::NewFromUtf8Literal( isolate, "device" ) ).ToLocalChecked();
	if( !deviceVal->IsObject() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "configure: device is required" ) ) );
		return;
	}
	GPUDevice* dev = node::ObjectWrap::Unwrap<GPUDevice>( deviceVal.As<Object>() );
	cfg.device = dev->handle_;

	// If we're reconfiguring with a different device, Vulkan's swapchain
	// can't switch — we have to release the old surface and build a new one.
	if( self->configuredDevice_ && self->configuredDevice_ != cfg.device ) {
		if( self->configured_ ) {
			wgpuSurfaceUnconfigure( self->surface_ );
			self->configured_ = false;
		}
		wgpuSurfaceRelease( self->surface_ );
		self->surface_ = wgpu_create_surface_for_renderer( self->renderer_ );
		if( !self->surface_ ) {
			isolate->ThrowException( Exception::Error( localStringExternal(
				isolate, "Failed to recreate surface for device switch." ) ) );
			return;
		}
	}

	// format — required, GPUTextureFormat enum string
	Local<Value> formatVal = opts->Get( context,
		String::NewFromUtf8Literal( isolate, "format" ) ).ToLocalChecked();
	if( !formatVal->IsString() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "configure: format is required" ) ) );
		return;
	}
	{
		String::Utf8Value fs( isolate, formatVal );
		cfg.format = wgpu_str_to_GPUTextureFormat(
			*fs, (size_t)fs.length(), WGPUTextureFormat_Undefined );
	}

	// usage — optional, GPUTextureUsageFlags. Default RenderAttachment(0x10).
	Local<Value> usageVal;
	if( opts->Get( context, String::NewFromUtf8Literal( isolate, "usage" ) )
	     .ToLocal( &usageVal ) && usageVal->IsNumber() ) {
		cfg.usage = (WGPUTextureUsage)usageVal->Uint32Value( context )
			.FromMaybe( (uint32_t)WGPUTextureUsage_RenderAttachment );
	}

	// alphaMode — optional, string. GPUCanvasAlphaMode is a JS-spec enum
	// that doesn't have a Dawn lookup table; handle inline.
	Local<Value> alphaVal;
	if( opts->Get( context, String::NewFromUtf8Literal( isolate, "alphaMode" ) )
	     .ToLocal( &alphaVal ) && alphaVal->IsString() ) {
		String::Utf8Value as( isolate, alphaVal );
		if(      strcmp( *as, "premultiplied" ) == 0 ) cfg.alphaMode = WGPUCompositeAlphaMode_Premultiplied;
		else if( strcmp( *as, "opaque" )        == 0 ) cfg.alphaMode = WGPUCompositeAlphaMode_Opaque;
		else                                            cfg.alphaMode = WGPUCompositeAlphaMode_Auto;
	}

	// Validate alphaMode against the surface's supported modes. Native
	// Windows/Vulkan typically doesn't support Premultiplied (it's a
	// browser-compositor concept). Three.js's WebGPURenderer defaults to
	// "premultiplied" for canvas blending — silently fall back to a
	// supported mode rather than erroring.
	if( dev->adapter_ ) {
		WGPUSurfaceCapabilities caps = WGPU_SURFACE_CAPABILITIES_INIT;
		if( wgpuSurfaceGetCapabilities( self->surface_, dev->adapter_, &caps )
		    == WGPUStatus_Success ) {
			// One-shot diagnostic: list every alphaMode Dawn returns.
			// WGPUCompositeAlphaMode: 0=Auto 1=Opaque 2=Premultiplied
			//                         3=Unpremultiplied 4=Inherit
			for( size_t i = 0; i < caps.alphaModeCount; i++ )
				lprintf( "surface caps alphaMode[%zu] = %d", i, (int)caps.alphaModes[ i ] );
			bool supported = false;
			for( size_t i = 0; i < caps.alphaModeCount; i++ )
				if( caps.alphaModes[ i ] == cfg.alphaMode ) { supported = true; break; }
			if( !supported && caps.alphaModeCount > 0 ) {
				// Auto silently picks the surface's preferred mode — only
				// warn on explicit fallbacks (Opaque/Premultiplied requested
				// but not supported).
				if( cfg.alphaMode != WGPUCompositeAlphaMode_Auto )
					lprintf( "GPUCanvasContext.configure: alphaMode %d not supported, using %d",
						(int)cfg.alphaMode, (int)caps.alphaModes[ 0 ] );
				cfg.alphaMode = caps.alphaModes[ 0 ];
			}
			wgpuSurfaceCapabilitiesFreeMembers( caps );
		}
	}

	// viewFormats — optional, sequence<GPUTextureFormat>
	WGPUTextureFormat* viewFormats = NULL;
	Local<Value> vfVal;
	if( opts->Get( context, String::NewFromUtf8Literal( isolate, "viewFormats" ) )
	     .ToLocal( &vfVal ) && vfVal->IsArray() ) {
		Local<Array> arr = vfVal.As<Array>();
		size_t n = (size_t)arr->Length();
		if( n > 0 ) {
			viewFormats = new WGPUTextureFormat[ n ];
			for( size_t i = 0; i < n; i++ ) {
				Local<Value> v = arr->Get( context, (uint32_t)i ).ToLocalChecked();
				String::Utf8Value s( isolate, v );
				viewFormats[ i ] = wgpu_str_to_GPUTextureFormat(
					*s, (size_t)s.length(), WGPUTextureFormat_Undefined );
			}
			cfg.viewFormatCount = n;
			cfg.viewFormats = viewFormats;
		}
	}

	// width / height — derive from the renderer's current display size, but
	// allow opts.{width,height} to override (useful for off-screen or scaled
	// surfaces). The WebGPU spec sources these from canvas.width/height; we
	// approximate via GetDisplayPosition on the underlying PRENDERER.
	if( self->renderer_ ) {
		int32_t x, y; uint32_t w, h;
		GetDisplayPosition( self->renderer_, &x, &y, &w, &h );
		cfg.width  = w;
		cfg.height = h;
	}
	Local<Value> wVal;
	if( opts->Get( context, String::NewFromUtf8Literal( isolate, "width" ) )
	     .ToLocal( &wVal ) && wVal->IsNumber() ) {
		cfg.width = wVal->Uint32Value( context ).FromMaybe( cfg.width );
	}
	Local<Value> hVal;
	if( opts->Get( context, String::NewFromUtf8Literal( isolate, "height" ) )
	     .ToLocal( &hVal ) && hVal->IsNumber() ) {
		cfg.height = hVal->Uint32Value( context ).FromMaybe( cfg.height );
	}

	lprintf( "configure ctx=%p surface=%p dev=%p prev_dev=%p format=%d size=%ux%u"
	         " alphaMode=%d usage=0x%X presentMode=%d",
		(void*)self, (void*)self->surface_, (void*)cfg.device,
		(void*)self->configuredDevice_, (int)cfg.format,
		(unsigned)cfg.width, (unsigned)cfg.height,
		(int)cfg.alphaMode, (unsigned)cfg.usage, (int)cfg.presentMode );

	wgpuSurfaceConfigure( self->surface_, &cfg );
	if( viewFormats ) delete[] viewFormats;

	self->configured_ = true;
	self->configuredDevice_ = cfg.device;

	// Inform the imglib-webgpu driver of the surface's dimensions, format,
	// and root Image. The driver's frame walker (invoked from
	// GPURenderPassEncoder.end on the surface pass) walks the PSI image
	// tree from this root, building NDC quads against these dimensions and
	// recording render bundles compatible with this format.
	sack::image::webgpu_image_set_surface_size  ( cfg.width, cfg.height );
	sack::image::webgpu_image_set_surface_format( cfg.format );
	if( self->renderer_ ) {
		// GetDisplayImage returns the sack Image backing the PRENDERER.
		// Cast through void* to dodge having to drag sack image headers
		// into the binding TU.
		void *root = (void*)GetDisplayImage( self->renderer_ );
		sack::image::webgpu_image_set_surface_root( root );
	}
}


// ---------- unconfigure ----------

void GPUCanvasContext::unconfigure( const FunctionCallbackInfo<Value>& args ) {
	GPUCanvasContext* self = node::ObjectWrap::Unwrap<GPUCanvasContext>( getFCIHolder( args ) );
	if( self->surface_ && self->configured_ ) {
		wgpuSurfaceUnconfigure( self->surface_ );
		self->configured_ = false;
	}
}


// ---------- getCurrentTexture ----------

void GPUCanvasContext::getCurrentTexture( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	GPUCanvasContext* self = node::ObjectWrap::Unwrap<GPUCanvasContext>( getFCIHolder( args ) );

	static int dumpTick = 0;
	if ((++dumpTick % 600) == 0) wgpuLiveDump();   // ~once per 10s at 60fps

	if( !self->surface_ || !self->configured_ ) {
		isolate->ThrowException( Exception::Error( localStringExternal(
			isolate, "getCurrentTexture: context is not configured" ) ) );
		return;
	}

	WGPUSurfaceTexture st = WGPU_SURFACE_TEXTURE_INIT;
	wgpuSurfaceGetCurrentTexture( self->surface_, &st );
	if( st.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal
	 && st.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal ) {
		isolate->ThrowException( Exception::Error( localStringExternal(
			isolate, "getCurrentTexture: surface texture not available" ) ) );
		return;
	}

	Local<Function> ctor = getConstructors( isolate )
		->GPUTexture_constructor.Get( isolate );
	Local<Object> obj = ctor->NewInstance( context, 0, NULL ).ToLocalChecked();
	GPUTexture* w = node::ObjectWrap::Unwrap<GPUTexture>( obj );
	w->handle_ = st.texture;

	// Publish this surface+texture to the auto-present tracker. The next
	// queue.submit that includes a CB written to this texture will fire
	// wgpuSurfacePresent automatically (browser-style semantics).
	wgpuSetActiveSurface( self->surface_, st.texture );

	args.GetReturnValue().Set( obj );
}


// ---------- present ----------
//
// Browsers auto-present on the next requestAnimationFrame; for our native
// runner, JS calls ctx.present() to swap the front/back buffers.

void GPUCanvasContext::present( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	GPUCanvasContext* self = node::ObjectWrap::Unwrap<GPUCanvasContext>( getFCIHolder( args ) );
	if( !self->surface_ || !self->configured_ ) {
		isolate->ThrowException( Exception::Error( localStringExternal(
			isolate, "present: context not configured" ) ) );
		return;
	}
	wgpuSurfacePresent( self->surface_ );
	// Explicit present invalidates the auto-present state so submit() won't
	// double-present this frame.
	wgpuClearActiveSurface();

}


// ---------- Init ----------

void GPUCanvasContext::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();

	Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, GPUCanvasContext::New );
	tpl->SetClassName( localStringExternal( isolate, "GPUCanvasContext" ) );
	tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

	NODE_SET_PROTOTYPE_METHOD( tpl, "configure",         GPUCanvasContext::configure );
	NODE_SET_PROTOTYPE_METHOD( tpl, "unconfigure",       GPUCanvasContext::unconfigure );
	NODE_SET_PROTOTYPE_METHOD( tpl, "getCurrentTexture", GPUCanvasContext::getCurrentTexture );
	NODE_SET_PROTOTYPE_METHOD( tpl, "present",           GPUCanvasContext::present );

	Local<Function> ctor = tpl->GetFunction( context ).ToLocalChecked();
	getConstructors( isolate )->GPUCanvasContext_constructor.Reset( isolate, ctor );
}


// ---------- factory called from sack_render_module ----------

Local<Object> webgpu_canvas_context_for_renderer( Isolate* isolate, PRENDERER r ) {
	if( !r ) return Local<Object>();
	Local<Context> context = isolate->GetCurrentContext();

	WGPUSurface surface = wgpu_create_surface_for_renderer( r );
	if( !surface ) return Local<Object>();

	Local<Function> ctor = getConstructors( isolate )
		->GPUCanvasContext_constructor.Get( isolate );
	Local<Object> obj = ctor->NewInstance( context, 0, NULL ).ToLocalChecked();
	GPUCanvasContext* c = node::ObjectWrap::Unwrap<GPUCanvasContext>( obj );
	c->surface_  = surface;
	c->renderer_ = r;
	return obj;
}
