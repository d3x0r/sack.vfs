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

// imglib-webgpu driver hooks (see imglib-webgpu/local.h for full list).
namespace sack { namespace image {
	void webgpu_image_set_surface_depth_format( WGPUTextureFormat fmt );
	void webgpu_image_refresh_psi_palette( void );
}}

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

// The canvas context with a frame currently in progress. Set by
// getCurrentTexture, consumed by the auto-present hook in queue.submit
// (or cleared by an explicit JS-side present()). Allows the submit-time
// auto-present in webgpu_module.cc to reach back into this file and do
// the back-buffer → surface blit before presenting, without having to
// know about back buffers itself. Declared up here so the dtor can clear
// it defensively.
static GPUCanvasContext* g_present_ctx = NULL;

GPUCanvasContext::GPUCanvasContext()
	: surface_( NULL ), renderer_( NULL ), configured_( false ),
	  configuredDevice_( NULL )
	{ wgpuLiveInc( "GPUCanvasContext" ); }

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
	// Surface MUST be released before the configured device — its
	// swapchain teardown reaches into device-owned bookkeeping (Vulkan
	// FencedDeleter etc.). Our addref on configuredDevice_ from
	// configure() keeps the device alive across the surface release below
	// even if JS dropped its GPUDevice wrapper first.
	if( surface_ ) {
		if( configured_ ) wgpuSurfaceUnconfigure( surface_ );
		wgpuSurfaceRelease( surface_ );
		surface_ = NULL;
	}
	if( configuredDevice_ ) {
		wgpuDeviceRelease( configuredDevice_ );
		configuredDevice_ = NULL;
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
	//
	// CRITICAL ORDERING: the swapchain's PerImage holds VkSemaphores it
	// releases via the old device's FencedDeleter at surface-release time.
	// If the old device's wrapper was already GC'd, that deleter is gone
	// and wgpuSurfaceRelease walks freed memory. We hold our own addref on
	// configuredDevice_ specifically to keep it alive across this teardown.
	if( self->configuredDevice_ && self->configuredDevice_ != cfg.device ) {
		// Any pending auto-present is on the old surface we're about to
		// tear down — drop the pointer first.
		if( g_present_ctx == self ) g_present_ctx = NULL;
		if( self->configured_ ) {
			wgpuSurfaceUnconfigure( self->surface_ );
			self->configured_ = false;
		}
		wgpuSurfaceRelease( self->surface_ );  // uses configuredDevice_
		// NOW it's safe to drop the old device ref.
		wgpuDeviceRelease( self->configuredDevice_ );
		self->configuredDevice_ = NULL;
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

	wgpuSurfaceConfigure( self->surface_, &cfg );
	if( viewFormats ) delete[] viewFormats;

	self->configured_ = true;
	// Hold our own ref on the configured device — the surface depends on
	// device-owned bookkeeping (FencedDeleter, etc.) at release time, and
	// the JS-side device wrapper might GC out from under us before we do.
	// Re-addref every successful configure even with the same device, since
	// we paired this with a release in any prior switch/unconfigure path.
	if( self->configuredDevice_ != cfg.device ) {
		if( self->configuredDevice_ ) wgpuDeviceRelease( self->configuredDevice_ );
		wgpuDeviceAddRef( cfg.device );
		self->configuredDevice_ = cfg.device;
	}

	// (Back-buffer preservation removed — multipass renderers manage their
	// own intermediate render targets, and intercepting the canvas's
	// current swap-chain texture interferes with that. JS code that needs
	// content preservation across frames should manage its own render
	// target and composite to the canvas on the final pass.)

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
	if( g_present_ctx == self ) g_present_ctx = NULL;
	if( self->surface_ && self->configured_ ) {
		wgpuSurfaceUnconfigure( self->surface_ );
		self->configured_ = false;
	}
	// Drop our device ref — wgpuSurfaceUnconfigure has detached the swapchain
	// so we no longer need device-owned bookkeeping kept alive on our behalf.
	if( self->configuredDevice_ ) {
		wgpuDeviceRelease( self->configuredDevice_ );
		self->configuredDevice_ = NULL;
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
	g_present_ctx = self;

	args.GetReturnValue().Set( obj );
}


// ---------- setSurfaceDepthFormat ----------
//
// Forwards to the imglib-webgpu driver so future render bundles record
// with a matching depthStencilFormat (+ depthReadOnly=true since our 2D
// HUD never writes depth). Without this, bundles only validate inside
// passes that have no depth attachment.

void GPUCanvasContext::setSurfaceDepthFormat( const v8::FunctionCallbackInfo<v8::Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	WGPUTextureFormat fmt = WGPUTextureFormat_Undefined;
	if( args.Length() >= 1 && args[ 0 ]->IsString() ) {
		String::Utf8Value s( isolate, args[ 0 ] );
		fmt = wgpu_str_to_GPUTextureFormat(
			*s, (size_t)s.length(), WGPUTextureFormat_Undefined );
	}
	// null / undefined / unrecognised string → Undefined, resetting to
	// "no depth attachment" mode (the original behaviour). That's the
	// safe fallback rather than throwing.
	sack::image::webgpu_image_set_surface_depth_format( fmt );
}


// ---------- refreshPSIPalette ----------
//
// Forwards to the imglib-webgpu driver to re-pull PSI's base colours
// into palette slots 1..14. No args, no return value.

void GPUCanvasContext::refreshPSIPalette( const v8::FunctionCallbackInfo<v8::Value>& args ) {
	(void)args;
	sack::image::webgpu_image_refresh_psi_palette();
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
	g_present_ctx = NULL;
	wgpuClearActiveSurface();
}


// ---------- Init ----------

void GPUCanvasContext::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();

	Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, GPUCanvasContext::New );
	tpl->SetClassName( localStringExternal( isolate, "GPUCanvasContext" ) );
	tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

	NODE_SET_PROTOTYPE_METHOD( tpl, "configure",             GPUCanvasContext::configure );
	NODE_SET_PROTOTYPE_METHOD( tpl, "unconfigure",           GPUCanvasContext::unconfigure );
	NODE_SET_PROTOTYPE_METHOD( tpl, "getCurrentTexture",     GPUCanvasContext::getCurrentTexture );
	NODE_SET_PROTOTYPE_METHOD( tpl, "setSurfaceDepthFormat", GPUCanvasContext::setSurfaceDepthFormat );
	NODE_SET_PROTOTYPE_METHOD( tpl, "refreshPSIPalette",     GPUCanvasContext::refreshPSIPalette );
	NODE_SET_PROTOTYPE_METHOD( tpl, "present",               GPUCanvasContext::present );

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
