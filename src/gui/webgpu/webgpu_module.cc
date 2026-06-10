// WebGPU module — implementation.
//
// First slice: GPU singleton + GPUAdapter wrapper + requestAdapter end-to-end.
// Proves out: Dawn linkage, V8 wrapper registration via constructorSet,
// async dispatch via uv_check + wgpuInstanceProcessEvents.

#include "webgpu_module.h"
#include "webgpu_bindings.h"
#include "canvas_context.h"
#include "generated/webgpu_generated.h"

#include <uv.h>
#include <map>
#include <string>
#include <mutex>

// ---------- Live-wrapper instrumentation ----------
//
// Per-class atomic counter of live wrapper instances. Used to find which Dawn
// handle type is failing to release in steady-state. Dump prints a sorted
// snapshot via lprintf. Cheap enough to leave in for now; remove once the
// leak is identified.

namespace {
	std::mutex& wgpuLiveMu() { static std::mutex m; return m; }
	std::map<std::string,int>& wgpuLiveTbl() { static std::map<std::string,int> t; return t; }
}
extern "C" void wgpuLiveInc( const char* cls ) {
	std::lock_guard<std::mutex> g( wgpuLiveMu() );
	wgpuLiveTbl()[cls]++;
}
extern "C" void wgpuLiveDec( const char* cls ) {
	std::lock_guard<std::mutex> g( wgpuLiveMu() );
	wgpuLiveTbl()[cls]--;
}
// ---------- Auto-present tracking ----------
//
// Browser WebGPU auto-presents at end-of-task; native binding needs to do it
// explicitly. mystral's approach (which we mirror): when queue.submit fires
// with a command buffer whose render pass wrote to the canvas's current
// swap-chain texture, present the surface and reset.
//
// State is global (one active surface at a time). beginRenderPass marks the
// owning encoder when any color attachment's view targets g_surfaceTexture;
// end()/finish() flips g_surfacePassEnded when that encoder closes; submit()
// presents if both conditions hold.

static WGPUSurface  g_surface         = NULL;
static WGPUTexture  g_surfaceTexture  = NULL;
static void*        g_surfaceEncoder  = NULL;  // GPUCommandEncoder* identity
static bool         g_surfacePassEnded = false;

// Per-frame draw tally (reset on auto-present). Use these to diagnose "I
// see nothing rendered" issues — if drawCalls stays 0, THREE built no draws
// (likely a scene-graph / camera / culling issue). If drawCalls > 0 but
// nothing visible, it's a transform/depth/winding issue downstream.
static uint64_t g_drawVerts = 0;
static uint64_t g_drawInsts = 0;
static uint32_t g_drawCalls = 0;
static uint32_t g_drawIndexedCalls = 0;

extern "C" void wgpuSetActiveSurface( WGPUSurface s, WGPUTexture t ) {
	g_surface = s;
	g_surfaceTexture = t;
	g_surfaceEncoder = NULL;
	g_surfacePassEnded = false;
}
extern "C" void wgpuClearActiveSurface( void ) {
	g_surface = NULL;
	g_surfaceTexture = NULL;
	g_surfaceEncoder = NULL;
	g_surfacePassEnded = false;
}

extern "C" void wgpuLiveDump( void ) {
	std::lock_guard<std::mutex> g( wgpuLiveMu() );
	std::string line = "LIVE";
	for( auto& p : wgpuLiveTbl() ) {
		line += " ";
		line += p.first;
		line += "=";
		line += std::to_string( p.second );
	}
	lprintf( "%s", line.c_str() );
}

// ---------- Internal forward static definitions ----------

static void GPUDevice_queueGetter(Local<Name> property,const PropertyCallbackInfo<Value>& info);
static void GPUDevice_createShaderModule(const FunctionCallbackInfo<Value>& args);
static void GPUDevice_createRenderPipeline(const FunctionCallbackInfo<Value>& args);
static void GPUDevice_createComputePipeline(const FunctionCallbackInfo<Value>& args);
static void GPUQueue_writeBuffer(const FunctionCallbackInfo<Value>& args);
static void GPUDevice_featuresGetter(Local<Name> property, const PropertyCallbackInfo<Value>& info);
static void GPUDevice_limitsGetter(Local<Name> property,
	const PropertyCallbackInfo<Value>& info);
static void GPUDevice_lostGetter(Local<Name> property,
	const PropertyCallbackInfo<Value>& info);
static void GPUDevice_createTexture(const FunctionCallbackInfo<Value>& args);
static void GPUDevice_createBindGroup(const FunctionCallbackInfo<Value>& args);

static void GPUBuffer_mapAsync(const FunctionCallbackInfo<Value>& args);
static void GPUBuffer_getMappedRange(const FunctionCallbackInfo<Value>&args);
static void GPUBuffer_unmap(const FunctionCallbackInfo<Value>&args);
static void GPUQueue_writeTexture(const FunctionCallbackInfo<Value>&args);
static void GPUDevice_popErrorScope(const FunctionCallbackInfo<Value>& args);
static void GPUBuffer_destroy(const FunctionCallbackInfo<Value>& args);


// ---------- per-isolate Dawn-event pump ----------
//
// The instance is created with no descriptor; callbacks registered with
// WGPUCallbackMode_AllowProcessEvents fire synchronously inside calls to
// wgpuInstanceProcessEvents() on the calling thread. We schedule that call
// on every libuv tick via a uv_check_t, so callbacks always land on the
// V8/uv thread and can resolve Promises directly without any cross-thread
// queue.

struct DawnPump {
	uv_check_t check;
	WGPUInstance instance;
	int pendingAsync;   // refcount of in-flight Dawn callbacks
};

// One pump per process (single-isolate for now). Add per-isolate lookup if
// we ever start using Node worker threads with the binding.
static DawnPump* g_dawnPump = NULL;

WGPUInstance webgpu_get_instance( void ) {
	return g_dawnPump ? g_dawnPump->instance : NULL;
}

// Active-device tracking — see webgpu_module.h for rationale. We hold an
// extra ref on both handles while they're the active pair, so native
// consumers can use them without racing the JS-side GPUDevice wrapper's
// lifetime. The slot is overwritten (with proper release of the prior
// handles) every time a new device is produced.
static WGPUDevice g_active_device = NULL;
static WGPUQueue  g_active_queue  = NULL;

static void set_active_device( WGPUDevice device ) {
	if( g_active_device == device )
		return;
	if( g_active_device )
		wgpuDeviceRelease( g_active_device );
	if( g_active_queue ) {
		wgpuQueueRelease( g_active_queue );
		g_active_queue = NULL;
	}
	g_active_device = device;
	if( g_active_device ) {
		wgpuDeviceAddRef( g_active_device );
		g_active_queue = wgpuDeviceGetQueue( g_active_device );
		// wgpuDeviceGetQueue returns a +1 ref per Dawn convention; no
		// extra AddRef needed.
	}
}

extern "C" WGPUDevice webgpu_get_active_device( void ) { return g_active_device; }
extern "C" WGPUQueue  webgpu_get_active_queue( void )  { return g_active_queue;  }

static void DawnPumpCb( uv_check_t* check ) {
	DawnPump* p = (DawnPump*)check->data;
	if( p && p->instance )
		wgpuInstanceProcessEvents( p->instance );
}

// Request/reply uv loop ref. Each Dawn async call refs the pump before
// dispatching; the matching Dawn callback unrefs at the top. The pair is
// always tightly scoped, so the pump pointer is guaranteed live across
// begin/end without coupling to wrapper-object lifetimes (which can outlive
// the module's static storage during Node shutdown).
//
// What this does NOT cover: long-running idle work between awaits with
// nothing else holding the loop. Test scripts that exercise WebGPU in
// isolation need an external anchor — a sack.Renderer(), an open socket,
// a setTimeout — to keep the loop running between async ops. The deviceTest
// script's `sack.Renderer()` at the top serves exactly this purpose.
static void wgpu_async_begin( void ) {
	if( g_dawnPump && ++g_dawnPump->pendingAsync == 1 )
		uv_ref( (uv_handle_t*)&g_dawnPump->check );
}
static void wgpu_async_end( void ) {
	if( g_dawnPump && --g_dawnPump->pendingAsync == 0 )
		uv_unref( (uv_handle_t*)&g_dawnPump->check );
}


// ---------- GPU ----------

GPU::GPU() : handle_( NULL ) { wgpuLiveInc( "GPU" ); }

GPU::~GPU() {
	if( handle_ ) {
		wgpuInstanceRelease( handle_ );
		handle_ = NULL;
	}
	wgpuLiveDec( "GPU" );
}

void GPU::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( !args.IsConstructCall() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "GPU is not constructible from JS" ) ) );
		return;
	}
	// Internal construction: a fresh GPU with no handle. InitWebGPU fills the
	// handle right after; the constructor used by JS-side ctor.NewInstance()
	// for async result wrapping (not used by GPU itself — it's a singleton).
	GPU* g = new GPU();
	g->Wrap( args.This() );
	args.GetReturnValue().Set( args.This() );
}

void GPU::requestAdapter( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPU );

	// Options bag — optional first argument. Routed through the generator's
	// reader so { powerPreference: "high-performance" } actually flows through
	// to Dawn (controls discrete-vs-integrated GPU selection on hybrid boxes).
	Local<Object> optsObj = ( args.Length() >= 1 && args[ 0 ]->IsObject() )
		? args[ 0 ].As<Object>() : Object::New( isolate );
	wgpu_read_GPURequestAdapterOptions optsReader( isolate, context, optsObj );

	// Promise to hand back to JS.
	Local<Promise::Resolver> resolver =
		Promise::Resolver::New( context ).ToLocalChecked();

	// Per-request state. Lives until the Dawn callback fires.
	struct AdapterRequest {
		Isolate* isolate;
		Persistent<Promise::Resolver> resolver;
	};
	AdapterRequest* req = new AdapterRequest();
	req->isolate = isolate;
	req->resolver.Reset( isolate, resolver );

	WGPURequestAdapterCallbackInfo info = {};
	info.mode = WGPUCallbackMode_AllowProcessEvents;
	info.callback = &GPU::OnAdapterReady;
	info.userdata1 = req;
	info.userdata2 = NULL;

	wgpu_async_begin();
	wgpuInstanceRequestAdapter( self->handle_, &optsReader.desc, info );

	args.GetReturnValue().Set( resolver->GetPromise() );
}

void GPU::OnAdapterReady( WGPURequestAdapterStatus status,
	WGPUAdapter adapter, WGPUStringView message,
	void* userdata1, void* userdata2 )
{
	wgpu_async_end();
	struct AdapterRequest {
		Isolate* isolate;
		Persistent<Promise::Resolver> resolver;
	};
	AdapterRequest* req = (AdapterRequest*)userdata1;
	Isolate* isolate = req->isolate;

	// We're inside wgpuInstanceProcessEvents on the V8 thread (uv_check
	// scheduled the pump). Still need handle scope + context scope to safely
	// touch JS objects.
	HandleScope hs( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	Context::Scope cs( context );

	Local<Promise::Resolver> resolver = req->resolver.Get( isolate );

	if( status == WGPURequestAdapterStatus_Success && adapter ) {
		// Spec allows resolving with the adapter object directly.
		Local<Function> ctor = getConstructors( isolate )
			->GPUAdapter_constructor.Get( isolate );
		Local<Object> obj = ctor->NewInstance( context, 0, NULL )
			.ToLocalChecked();
		GPUAdapter* w = node::ObjectWrap::Unwrap<GPUAdapter>( obj );
		w->handle_ = adapter;
		(void)resolver->Resolve( context, obj );
	} else {
		// Spec: resolve with null if no adapter is available.
		// (Spec actually rejects only for invalid options; mirror that
		// later. For now, null is a defensible default.)
		(void)resolver->Resolve( context, Null( isolate ) );
		lprintf( "wgpuInstanceRequestAdapter failed: status=%d msg=%.*s",
			(int)status,
			message.length == SIZE_MAX ? (int)strlen( message.data ? message.data : "" )
				: (int)message.length,
			message.data ? message.data : "" );
	}

	req->resolver.Reset();
	delete req;
}

// navigator.gpu.getPreferredCanvasFormat() — spec returns "bgra8unorm" or
// "rgba8unorm" depending on the platform. Dawn doesn't expose a query for
// it (that's a browser concept), so we pick by build target. Windows/macOS
// canvases use BGRA; Android/some Linux compositors prefer RGBA.
static void GPU_getPreferredCanvasFormat( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
#if defined(_WIN32) || defined(__APPLE__)
	const char* fmt = "bgra8unorm";
#else
	const char* fmt = "rgba8unorm";
#endif
	args.GetReturnValue().Set(
		String::NewFromUtf8( isolate, fmt ).ToLocalChecked() );
}

void GPU::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();

	Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, GPU::New );
	tpl->SetClassName( localStringExternal( isolate, "GPU" ) );
	tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

	NODE_SET_PROTOTYPE_METHOD( tpl, "requestAdapter", GPU::requestAdapter );
	NODE_SET_PROTOTYPE_METHOD( tpl, "getPreferredCanvasFormat", GPU_getPreferredCanvasFormat );

	Local<Function> ctor = tpl->GetFunction( context ).ToLocalChecked();
	getConstructors( isolate )->GPU_constructor.Reset( isolate, ctor );
	// Class itself is not exported — only the singleton instance below.
}


// ---------- GPUAdapter ----------

GPUAdapter::GPUAdapter() : handle_( NULL ) { wgpuLiveInc( "GPUAdapter" ); }

GPUAdapter::~GPUAdapter() {
	if( handle_ ) {
		wgpuAdapterRelease( handle_ );
		handle_ = NULL;
	}
	wgpuLiveDec( "GPUAdapter" );
}

void GPUAdapter::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( !args.IsConstructCall() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "GPUAdapter is not constructible from JS" ) ) );
		return;
	}
	GPUAdapter* a = new GPUAdapter();
	a->Wrap( args.This() );
	args.GetReturnValue().Set( args.This() );
}

// readonly attribute GPUAdapterInfo info — synthesized into a plain JS object.
// (Per WebGPU spec a separate GPUAdapterInfo class exists; for now we expose
// the fields directly on a plain object. Promote to a wrapper class later if
// JS code starts caring about the class identity.)
void GPUAdapter::infoGetter( Local<Name> property,
	const PropertyCallbackInfo<Value>& cbInfo )
{
	Isolate* isolate = cbInfo.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	GPUAdapter* self = node::ObjectWrap::Unwrap<GPUAdapter>( cbInfo.This() );

	WGPUAdapterInfo info = {};
	WGPUStatus status = wgpuAdapterGetInfo( self->handle_, &info );
	if( status != WGPUStatus_Success ) {
		cbInfo.GetReturnValue().Set( Null( isolate ) );
		return;
	}

	Local<Object> out = Object::New( isolate );
	SET( out, "vendor",          WGPU_StringViewToV8( isolate, info.vendor ) );
	SET( out, "architecture",    WGPU_StringViewToV8( isolate, info.architecture ) );
	SET( out, "device",          WGPU_StringViewToV8( isolate, info.device ) );
	SET( out, "description",     WGPU_StringViewToV8( isolate, info.description ) );
	// TODO: enum→string for backendType/adapterType once we have generated tables
	SET( out, "backendType",     Integer::New( isolate, (int)info.backendType ) );
	SET( out, "adapterType",     Integer::New( isolate, (int)info.adapterType ) );
	SET( out, "vendorID",        Integer::NewFromUnsigned( isolate, info.vendorID ) );
	SET( out, "deviceID",        Integer::NewFromUnsigned( isolate, info.deviceID ) );
	SET( out, "subgroupMinSize", Integer::NewFromUnsigned( isolate, info.subgroupMinSize ) );
	SET( out, "subgroupMaxSize", Integer::NewFromUnsigned( isolate, info.subgroupMaxSize ) );

	wgpuAdapterInfoFreeMembers( info );

	cbInfo.GetReturnValue().Set( out );
}

// Dawn uncaptured error callback — fires synchronously inside processEvents
// when the device hits a validation or runtime error that wasn't captured by
// an error scope. We just log; the user can override the wiring later if they
// want JS-side notification.
static void GPUDevice_OnUncapturedError( WGPUDevice const* device,
	WGPUErrorType type, WGPUStringView message,
	void* userdata1, void* userdata2 )
{
	(void)device; (void)userdata1; (void)userdata2;
	const char* tname =
		type == WGPUErrorType_Validation ? "Validation" :
		type == WGPUErrorType_OutOfMemory ? "OutOfMemory" :
		type == WGPUErrorType_Internal ? "Internal" :
		type == WGPUErrorType_Unknown ? "Unknown" : "Other";
	lprintf( "WebGPU [%s] %.*s", tname,
		message.length == SIZE_MAX ? (int)strlen( message.data ? message.data : "" )
			: (int)message.length,
		message.data ? message.data : "" );
}

// Dawn device logging callback — distinct from uncapturedError. Receives
// VERBOSE/INFO/WARNING/ERROR-tagged messages from Dawn's internal logging
// (driver issues, internal validation surfacings, performance hints, etc.).
// Without this set, Dawn prints a "no logging callback" startup warning.
static void GPUDevice_OnLogging( WGPULoggingType type, WGPUStringView message,
	void* userdata1, void* userdata2 )
{
	(void)userdata1; (void)userdata2;
	const char* tname =
		type == WGPULoggingType_Verbose ? "Verbose" :
		type == WGPULoggingType_Info    ? "Info" :
		type == WGPULoggingType_Warning ? "Warning" :
		type == WGPULoggingType_Error   ? "Error" : "Log";
	lprintf( "WebGPU.log [%s] %.*s", tname,
		message.length == SIZE_MAX ? (int)strlen( message.data ? message.data : "" )
			: (int)message.length,
		message.data ? message.data : "" );
}

// Per-device lost-Promise state. Allocated at request time; the lost
// callback (which fires later — possibly never) consumes it. Lives on the
// GPUDevice wrapper after OnDeviceReady transfers ownership.
struct GPUDeviceLostState {
	Isolate* isolate;
	Persistent<Promise::Resolver> resolver;
};

static void GPUDevice_OnDeviceLost( WGPUDevice const* device,
	WGPUDeviceLostReason reason, WGPUStringView message,
	void* userdata1, void* userdata2 )
{
	(void)device; (void)userdata2;
	GPUDeviceLostState* st = (GPUDeviceLostState*)userdata1;
	if( !st ) return;
	Isolate* isolate = st->isolate;
	HandleScope hs( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	Context::Scope cs( context );

	const char* rname =
		reason == WGPUDeviceLostReason_Destroyed         ? "destroyed" :
		reason == WGPUDeviceLostReason_CallbackCancelled ? "callback-cancelled" :
		reason == WGPUDeviceLostReason_FailedCreation    ? "failed-creation" :
		"unknown";

	Local<Object> info = Object::New( isolate );
	SET( info, "reason",
		String::NewFromUtf8( isolate, rname ).ToLocalChecked() );
	SET( info, "message", WGPU_StringViewToV8( isolate, message ) );

	Local<Promise::Resolver> resolver = st->resolver.Get( isolate );
	(void)resolver->Resolve( context, info );
	st->resolver.Reset();
	// State is freed when the GPUDevice dtor runs. Don't delete here; the
	// device might still be alive even after the lost event (spec allows
	// calling some methods on a lost device, they just error).
}

void GPUAdapter::requestDevice( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUAdapter );

	// (TODO: parse requiredFeatures, requiredLimits, defaultQueue, label
	//  from optional args[0])

	Local<Promise::Resolver> resolver =
		Promise::Resolver::New( context ).ToLocalChecked();

	struct DeviceRequest {
		Isolate* isolate;
		Persistent<Promise::Resolver> resolver;
		GPUDeviceLostState* lostState;
		WGPUAdapter adapter;  // borrowed; transferred to device wrapper
	};
	DeviceRequest* req = new DeviceRequest();
	req->isolate = isolate;
	req->resolver.Reset( isolate, resolver );
	req->adapter = self->handle_;

	// Pre-allocate the lost-Promise state so we can wire its userdata into
	// the device descriptor right now. OnDeviceReady transfers ownership of
	// this state to the GPUDevice wrapper for cleanup on device release.
	Local<Promise::Resolver> lostResolver =
		Promise::Resolver::New( context ).ToLocalChecked();
	req->lostState = new GPUDeviceLostState();
	req->lostState->isolate = isolate;
	req->lostState->resolver.Reset( isolate, lostResolver );

	WGPURequestDeviceCallbackInfo info = {};
	info.mode = WGPUCallbackMode_AllowProcessEvents;
	info.callback = &GPUAdapter::OnDeviceReady;
	info.userdata1 = req;
	info.userdata2 = NULL;

	WGPUDeviceDescriptor desc = WGPU_DEVICE_DESCRIPTOR_INIT;
	desc.uncapturedErrorCallbackInfo.callback = GPUDevice_OnUncapturedError;
	desc.deviceLostCallbackInfo.mode     = WGPUCallbackMode_AllowProcessEvents;
	desc.deviceLostCallbackInfo.callback = GPUDevice_OnDeviceLost;
	desc.deviceLostCallbackInfo.userdata1 = req->lostState;

	wgpu_async_begin();
	wgpuAdapterRequestDevice( self->handle_, &desc, info );

	args.GetReturnValue().Set( resolver->GetPromise() );
}

void GPUAdapter::OnDeviceReady( WGPURequestDeviceStatus status,
	WGPUDevice device, WGPUStringView message,
	void* userdata1, void* userdata2 )
{
	wgpu_async_end();
	struct DeviceRequest {
		Isolate* isolate;
		Persistent<Promise::Resolver> resolver;
		GPUDeviceLostState* lostState;
		WGPUAdapter adapter;
	};
	DeviceRequest* req = (DeviceRequest*)userdata1;
	Isolate* isolate = req->isolate;
	HandleScope hs( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	Context::Scope cs( context );

	Local<Promise::Resolver> resolver = req->resolver.Get( isolate );

	if( status == WGPURequestDeviceStatus_Success && device ) {
		// Attach the logging callback before the JS code can use the device,
		// so Dawn's startup chatter (and any subsequent logs) reach lprintf.
		WGPULoggingCallbackInfo logInfo = WGPU_LOGGING_CALLBACK_INFO_INIT;
		logInfo.callback = &GPUDevice_OnLogging;
		wgpuDeviceSetLoggingCallback( device, logInfo );

		Local<Function> ctor = getConstructors( isolate )
			->GPUDevice_constructor.Get( isolate );
		Local<Object> obj = ctor->NewInstance( context, 0, NULL )
			.ToLocalChecked();
		GPUDevice* w = node::ObjectWrap::Unwrap<GPUDevice>( obj );
		w->handle_ = device;
		// Track as the active device for native sack-side consumers
		// (imglib-webgpu, etc.). set_active_device takes its own ref;
		// the wrapper's ref is independent.
		set_active_device( device );
		// Transfer ownership of the lost-state from the request to the
		// device wrapper, so it's freed when the wrapper is GC'd.
		w->lostState_ = req->lostState;
		req->lostState = NULL;
		// Keep our own ref to the adapter so the device can call adapter-
		// scoped APIs (surface capabilities, etc.) independent of the JS-
		// side adapter object's lifetime.
		if( req->adapter ) {
			wgpuAdapterAddRef( req->adapter );
			w->adapter_ = req->adapter;
		}
		(void)resolver->Resolve( context, obj );
	} else if( req->lostState ) {
		// Request failed — the lost callback will never fire. Clean up.
		req->lostState->resolver.Reset();
		delete req->lostState;
		req->lostState = NULL;
	} else {
		(void)resolver->Reject( context, Exception::Error( localStringExternal(
			isolate, "requestDevice failed" ) ) );
		lprintf( "wgpuAdapterRequestDevice failed: status=%d msg=%.*s",
			(int)status,
			message.length == SIZE_MAX ? (int)strlen( message.data ? message.data : "" )
				: (int)message.length,
			message.data ? message.data : "" );
	}

	req->resolver.Reset();
	delete req;
}

// Shared helper: copy a populated WGPULimits into a plain JS object with the
// IDL-spec field names. Used by both adapter.limits and device.limits.
// Field list mirrors dawn.json's `limits` structure (32 fields in this pin).
// uint32 fields → Integer; uint64 fields → Number (JS double) so values >
// 2^32 don't lose precision the way Integer does at 32 bits.
static Local<Object> wgpu_BuildLimitsObject( Isolate* isolate, const WGPULimits& lim ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> out = Object::New( isolate );
	#define _LIM_U32( n ) SET( out, #n, Integer::NewFromUnsigned( isolate, (uint32_t)lim.n ) )
	#define _LIM_U64( n ) SET( out, #n, Number::New( isolate, (double)lim.n ) )
	_LIM_U32( maxTextureDimension1D );
	_LIM_U32( maxTextureDimension2D );
	_LIM_U32( maxTextureDimension3D );
	_LIM_U32( maxTextureArrayLayers );
	_LIM_U32( maxBindGroups );
	_LIM_U32( maxBindGroupsPlusVertexBuffers );
	_LIM_U32( maxBindingsPerBindGroup );
	_LIM_U32( maxDynamicUniformBuffersPerPipelineLayout );
	_LIM_U32( maxDynamicStorageBuffersPerPipelineLayout );
	_LIM_U32( maxSampledTexturesPerShaderStage );
	_LIM_U32( maxSamplersPerShaderStage );
	_LIM_U32( maxStorageBuffersPerShaderStage );
	_LIM_U32( maxStorageTexturesPerShaderStage );
	_LIM_U32( maxUniformBuffersPerShaderStage );
	_LIM_U64( maxUniformBufferBindingSize );
	_LIM_U64( maxStorageBufferBindingSize );
	_LIM_U32( minUniformBufferOffsetAlignment );
	_LIM_U32( minStorageBufferOffsetAlignment );
	_LIM_U32( maxVertexBuffers );
	_LIM_U64( maxBufferSize );
	_LIM_U32( maxVertexAttributes );
	_LIM_U32( maxVertexBufferArrayStride );
	_LIM_U32( maxInterStageShaderVariables );
	_LIM_U32( maxColorAttachments );
	_LIM_U32( maxColorAttachmentBytesPerSample );
	_LIM_U32( maxComputeWorkgroupStorageSize );
	_LIM_U32( maxComputeInvocationsPerWorkgroup );
	_LIM_U32( maxComputeWorkgroupSizeX );
	_LIM_U32( maxComputeWorkgroupSizeY );
	_LIM_U32( maxComputeWorkgroupSizeZ );
	_LIM_U32( maxComputeWorkgroupsPerDimension );
	_LIM_U32( maxImmediateSize );
	#undef _LIM_U32
	#undef _LIM_U64
	return out;
}

// readonly attribute GPUSupportedFeatures features — setlike of strings.
// [SameObject] cached on the adapter JS instance via a private symbol.
static void GPUAdapter_featuresGetter( Local<Name> property,
	const PropertyCallbackInfo<Value>& info )
{
	Isolate* isolate = info.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> selfObj = info.This();
	(void)property;

	Local<Private> key = Private::ForApi( isolate,
		String::NewFromUtf8Literal( isolate, "__featuresCache" ) );
	Local<Value> cached;
	if( selfObj->GetPrivate( context, key ).ToLocal( &cached )
	    && cached->IsSet() ) {
		info.GetReturnValue().Set( cached );
		return;
	}

	GPUAdapter* self = node::ObjectWrap::Unwrap<GPUAdapter>( selfObj );
	WGPUSupportedFeatures feats = WGPU_SUPPORTED_FEATURES_INIT;
	wgpuAdapterGetFeatures( self->handle_, &feats );

	Local<Set> set = Set::New( isolate );
	for( size_t i = 0; i < feats.featureCount; i++ ) {
		const char* s = wgpu_str_from_GPUFeatureName( feats.features[ i ] );
		Local<Value> v;
		if( s ) v = String::NewFromUtf8( isolate, s ).ToLocalChecked().As<Value>();
		else    v = Integer::New( isolate, (int)feats.features[ i ] ).As<Value>();
		(void)set->Add( context, v );
	}
	wgpuSupportedFeaturesFreeMembers( feats );

	(void)selfObj->SetPrivate( context, key, set );
	info.GetReturnValue().Set( set );
}

// readonly attribute GPUSupportedLimits limits — fetched + cached.
static void GPUAdapter_limitsGetter( Local<Name> property,
	const PropertyCallbackInfo<Value>& info )
{
	Isolate* isolate = info.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> selfObj = info.This();
	(void)property;

	Local<Private> key = Private::ForApi( isolate,
		String::NewFromUtf8Literal( isolate, "__limitsCache" ) );
	Local<Value> cached;
	if( selfObj->GetPrivate( context, key ).ToLocal( &cached )
	    && cached->IsObject() ) {
		info.GetReturnValue().Set( cached );
		return;
	}

	GPUAdapter* self = node::ObjectWrap::Unwrap<GPUAdapter>( selfObj );
	WGPULimits lim = WGPU_LIMITS_INIT;
	wgpuAdapterGetLimits( self->handle_, &lim );
	Local<Object> out = wgpu_BuildLimitsObject( isolate, lim );
	(void)selfObj->SetPrivate( context, key, out );
	info.GetReturnValue().Set( out );
}

void GPUAdapter::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();

	Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, GPUAdapter::New );
	tpl->SetClassName( localStringExternal( isolate, "GPUAdapter" ) );
	tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

	NODE_SET_PROTOTYPE_METHOD( tpl, "requestDevice", GPUAdapter::requestDevice );

	// readonly attribute GPUAdapterInfo info
#if ( NODE_MAJOR_VERSION >= 22 )
	tpl->PrototypeTemplate()->SetNativeDataProperty(
		String::NewFromUtf8Literal( isolate, "info" ),
		GPUAdapter::infoGetter,
		NULL,
		Local<Value>(),
		PropertyAttribute::None,
		SideEffectType::kHasNoSideEffect,
		SideEffectType::kHasSideEffect );
#else
	tpl->PrototypeTemplate()->SetNativeDataProperty(
		String::NewFromUtf8Literal( isolate, "info" ),
		GPUAdapter::infoGetter,
		NULL,
		Local<Value>(),
		PropertyAttribute::ReadOnly );
#endif

	// readonly attribute GPUSupportedFeatures features
	tpl->PrototypeTemplate()->SetNativeDataProperty(
		String::NewFromUtf8Literal( isolate, "features" ),
		GPUAdapter_featuresGetter );

	// readonly attribute GPUSupportedLimits limits
	tpl->PrototypeTemplate()->SetNativeDataProperty(
		String::NewFromUtf8Literal( isolate, "limits" ),
		GPUAdapter_limitsGetter );

	Local<Function> ctor = tpl->GetFunction( context ).ToLocalChecked();
	getConstructors( isolate )->GPUAdapter_constructor.Reset( isolate, ctor );
}


// ---------- GPUDevice ----------

GPUDevice::GPUDevice() : handle_( NULL ), adapter_( NULL ), lostState_( NULL ) {
	wgpuLiveInc( "GPUDevice" );
}

GPUDevice::~GPUDevice() {
	if( handle_ ) {
		// If we were the active-device, clear that slot first (it holds
		// its own ref, so this is purely a "stop pointing at us" cleanup;
		// the slot's release of the device is independent of ours).
		if( g_active_device == handle_ )
			set_active_device( NULL );
		wgpuDeviceRelease( handle_ );
		handle_ = NULL;
	}
	if( adapter_ ) {
		wgpuAdapterRelease( adapter_ );
		adapter_ = NULL;
	}
	if( lostState_ ) {
		lostState_->resolver.Reset();
		delete lostState_;
		lostState_ = NULL;
	}
	wgpuLiveDec( "GPUDevice" );
}

void GPUDevice::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( !args.IsConstructCall() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "GPUDevice is not constructible from JS" ) ) );
		return;
	}
	GPUDevice* d = new GPUDevice();
	d->Wrap( args.This() );
	args.GetReturnValue().Set( args.This() );
}

void GPUDevice::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, GPUDevice::New );
	tpl->SetClassName( localStringExternal( isolate, "GPUDevice" ) );
	tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

	// Generator-emitted methods first, then hand-written overrides for
	// the cases the generator can't faithfully express.
	wireGenerated_GPUDevice( isolate, tpl );

	// Hand-written [SameObject] queue accessor.
	tpl->PrototypeTemplate()->SetNativeDataProperty(
		String::NewFromUtf8Literal( isolate, "queue" ),
		GPUDevice_queueGetter );

	// readonly attribute GPUSupportedFeatures features
	tpl->PrototypeTemplate()->SetNativeDataProperty(
		String::NewFromUtf8Literal( isolate, "features" ),
		GPUDevice_featuresGetter );

	// readonly attribute GPUSupportedLimits limits
	tpl->PrototypeTemplate()->SetNativeDataProperty(
		String::NewFromUtf8Literal( isolate, "limits" ),
		GPUDevice_limitsGetter );

	// readonly attribute Promise<GPUDeviceLostInfo> lost
	tpl->PrototypeTemplate()->SetNativeDataProperty(
		String::NewFromUtf8Literal( isolate, "lost" ),
		GPUDevice_lostGetter );

	// Overrides — these replace the generated versions, which can't model
	// chained source structs (createShaderModule) or union args (the layout
	// union on the pipeline descriptors).

	NODE_SET_PROTOTYPE_METHOD( tpl, "createShaderModule",   GPUDevice_createShaderModule );
	NODE_SET_PROTOTYPE_METHOD( tpl, "createRenderPipeline", GPUDevice_createRenderPipeline );
	NODE_SET_PROTOTYPE_METHOD( tpl, "createComputePipeline", GPUDevice_createComputePipeline );
	NODE_SET_PROTOTYPE_METHOD( tpl, "createTexture",        GPUDevice_createTexture );
	NODE_SET_PROTOTYPE_METHOD( tpl, "createBindGroup",      GPUDevice_createBindGroup );
	NODE_SET_PROTOTYPE_METHOD( tpl, "popErrorScope",        GPUDevice_popErrorScope );

	Local<Function> ctor = tpl->GetFunction( context ).ToLocalChecked();
	getConstructors( isolate )->GPUDevice_constructor.Reset( isolate, ctor );
}


// ---------- GPUBuffer ----------

GPUBuffer::GPUBuffer() : handle_( NULL ) { wgpuLiveInc( "GPUBuffer" ); }

GPUBuffer::~GPUBuffer() {
	if( handle_ ) {
		wgpuBufferRelease( handle_ );
		handle_ = NULL;
	}
	wgpuLiveDec( "GPUBuffer" );
}

void GPUBuffer::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( !args.IsConstructCall() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "GPUBuffer is not constructible from JS" ) ) );
		return;
	}
	GPUBuffer* b = new GPUBuffer();
	b->Wrap( args.This() );
	args.GetReturnValue().Set( args.This() );
}

void GPUBuffer::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, GPUBuffer::New );
	tpl->SetClassName( localStringExternal( isolate, "GPUBuffer" ) );
	tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

	// Generator-emitted methods (destroy + size/usage/mapState attribute
	// getters). The skip list intentionally excludes unmap/mapAsync/
	// getMappedRange — those are hand-written below for the ArrayBuffer
	// aliasing path.
	wireGenerated_GPUBuffer( isolate, tpl );

	NODE_SET_PROTOTYPE_METHOD( tpl, "mapAsync",       GPUBuffer_mapAsync );
	NODE_SET_PROTOTYPE_METHOD( tpl, "getMappedRange", GPUBuffer_getMappedRange );
	NODE_SET_PROTOTYPE_METHOD( tpl, "unmap",          GPUBuffer_unmap );
	NODE_SET_PROTOTYPE_METHOD( tpl, "destroy",        GPUBuffer_destroy );

	Local<Function> ctor = tpl->GetFunction( context ).ToLocalChecked();
	getConstructors( isolate )->GPUBuffer_constructor.Reset( isolate, ctor );
}


// ---------- Simple handle-wrapper class implementations ----------
//
// Each class is a thin RAII shell over a WGPU* handle, dispatching all method
// surface to the generator-emitted wireGenerated_<X> (or none, for classes
// whose methods are still HAND).
//
// WGPU_SIMPLE_WRAPPER expands to ctor (handle = NULL), dtor (release if
// non-NULL), New (rejects JS-side construction), and Init (registers the
// template, wires the generated methods if any, stashes the constructor in
// the per-isolate constructorSet).

// Shared body of the wrapper class definition (ctor/dtor/New). The two
// outer macros below append a different Init based on whether the class has
// generator-emitted methods to wire up.
#define WGPU_WRAPPER_COMMON( Cls, releaseFn )                                 \
	Cls::Cls() : handle_( NULL ) { wgpuLiveInc( #Cls ); }                 \
	Cls::~Cls() {                                                         \
		if( handle_ ) {                                               \
			releaseFn( handle_ );                                 \
			handle_ = NULL;                                       \
			wgpuLiveDec( #Cls );                                  \
		}                                                             \
	}                                                                     \
	void Cls::New( const FunctionCallbackInfo<Value>& args ) {            \
		Isolate* isolate = args.GetIsolate();                         \
		if( !args.IsConstructCall() ) {                               \
			isolate->ThrowException( Exception::TypeError(        \
				localStringExternal( isolate,                 \
					#Cls " is not constructible from JS" ) ) ); \
			return;                                               \
		}                                                             \
		Cls* w = new Cls();                                           \
		w->Wrap( args.This() );                                       \
		args.GetReturnValue().Set( args.This() );                     \
	}

#define WGPU_SIMPLE_WRAPPER( Cls, releaseFn )                                 \
	WGPU_WRAPPER_COMMON( Cls, releaseFn )                                 \
	void Cls::Init( Isolate* isolate, Local<Object> exports ) {           \
		Local<Context> context = isolate->GetCurrentContext();        \
		Local<FunctionTemplate> tpl = FunctionTemplate::New(          \
			isolate, Cls::New );                                  \
		tpl->SetClassName( localStringExternal( isolate, #Cls ) );    \
		tpl->InstanceTemplate()->SetInternalFieldCount( 1 );          \
		Local<Function> ctor = tpl->GetFunction( context )            \
			.ToLocalChecked();                                    \
		getConstructors( isolate )->Cls##_constructor.Reset(          \
			isolate, ctor );                                      \
	}

#define WGPU_WIRED_WRAPPER( Cls, releaseFn )                                  \
	WGPU_WRAPPER_COMMON( Cls, releaseFn )                                 \
	void Cls::Init( Isolate* isolate, Local<Object> exports ) {           \
		Local<Context> context = isolate->GetCurrentContext();        \
		Local<FunctionTemplate> tpl = FunctionTemplate::New(          \
			isolate, Cls::New );                                  \
		tpl->SetClassName( localStringExternal( isolate, #Cls ) );    \
		tpl->InstanceTemplate()->SetInternalFieldCount( 1 );          \
		wireGenerated_##Cls( isolate, tpl );                          \
		Local<Function> ctor = tpl->GetFunction( context )            \
			.ToLocalChecked();                                    \
		getConstructors( isolate )->Cls##_constructor.Reset(          \
			isolate, ctor );                                      \
	}

// No GEN methods yet — exist solely so other classes' generated methods can
// take/return them as handle types.
WGPU_SIMPLE_WRAPPER( GPUSampler,         wgpuSamplerRelease )
WGPU_SIMPLE_WRAPPER( GPUTextureView,     wgpuTextureViewRelease )
WGPU_SIMPLE_WRAPPER( GPUCommandBuffer,   wgpuCommandBufferRelease )
WGPU_SIMPLE_WRAPPER( GPUBindGroupLayout, wgpuBindGroupLayoutRelease )

// Have GEN method blocks — Init wires the generated prototype methods.
// GPUTexture: COMMON so custom Init can register hand-written createView
// (needs to stash the source texture on the new view for the auto-present
// surface-attachment detection).
WGPU_WRAPPER_COMMON( GPUTexture, wgpuTextureRelease )
WGPU_WIRED_WRAPPER( GPUQuerySet,            wgpuQuerySetRelease )
// GPUCommandEncoder: ctor/dtor/New via the common macro; Init is hand-written
// below so we can add beginRenderPass (HAND because GPURenderPassDescriptor
// has sequence-of-dict colorAttachments that the generator can't emit yet).
WGPU_WRAPPER_COMMON( GPUCommandEncoder, wgpuCommandEncoderRelease )
WGPU_WIRED_WRAPPER( GPUComputePassEncoder,  wgpuComputePassEncoderRelease )
// GPURenderPassEncoder: COMMON so custom Init can add hand-written end()
// that releases the WGPU handle immediately (spec invalidates the encoder
// at end-of-pass — holding the ref just for GC stalls Dawn memory).
WGPU_WRAPPER_COMMON( GPURenderPassEncoder, wgpuRenderPassEncoderRelease )
WGPU_WIRED_WRAPPER( GPURenderBundleEncoder, wgpuRenderBundleEncoderRelease )
WGPU_WIRED_WRAPPER( GPURenderPipeline,      wgpuRenderPipelineRelease )
WGPU_WIRED_WRAPPER( GPUComputePipeline,     wgpuComputePipelineRelease )

// GPUQueue: COMMON so Init can register hand-written writeBuffer alongside
// the (hand-written) submit method.
WGPU_WRAPPER_COMMON( GPUQueue,          wgpuQueueRelease )
WGPU_SIMPLE_WRAPPER( GPURenderBundle,   wgpuRenderBundleRelease )
WGPU_SIMPLE_WRAPPER( GPUBindGroup,      wgpuBindGroupRelease )
WGPU_SIMPLE_WRAPPER( GPUPipelineLayout, wgpuPipelineLayoutRelease )
WGPU_SIMPLE_WRAPPER( GPUShaderModule,   wgpuShaderModuleRelease )

#undef WGPU_SIMPLE_WRAPPER
#undef WGPU_WIRED_WRAPPER
#undef WGPU_WRAPPER_COMMON


// ---------- GPUCommandEncoder.beginRenderPass (hand-written) ----------
//
// GPURenderPassDescriptor.colorAttachments is sequence<GPURenderPassColorAttachment?>
// which the generator can't currently emit (sequence-of-dict requires
// per-element string-holder lifetimes). Hand-written to unblock the
// see-a-red-window milestone. Supports the minimum useful surface:
// colorAttachments[*].{view, clearValue, loadOp, storeOp}. depthStencil,
// timestamp writes, occlusion query set are not yet read here.

static void GPUCommandEncoder_beginRenderPass( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUCommandEncoder );
	if( args.Length() < 1 || !args[ 0 ]->IsObject() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "beginRenderPass requires a descriptor object" ) ) );
		return;
	}
	Local<Object> desc = args[ 0 ].As<Object>();

	Local<Value> attachmentsVal = desc->Get( context,
		String::NewFromUtf8Literal( isolate, "colorAttachments" ) ).ToLocalChecked();
	if( !attachmentsVal->IsArray() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "beginRenderPass: colorAttachments must be an array" ) ) );
		return;
	}
	Local<Array> attachments = attachmentsVal.As<Array>();
	uint32_t n = attachments->Length();

	// All per-attachment storage lives on the stack frame for the lifetime
	// of the wgpuCommandEncoderBeginRenderPass call.
	WGPURenderPassColorAttachment* arr = n
		? new WGPURenderPassColorAttachment[ n ]() : NULL;

	for( uint32_t i = 0; i < n; i++ ) {
		Local<Value> entryVal = attachments->Get( context, i ).ToLocalChecked();
		if( !entryVal->IsObject() ) continue;
		Local<Object> entry = entryVal.As<Object>();
		WGPURenderPassColorAttachment& a = arr[ i ];
		a = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;

		// view (required)
		Local<Value> viewVal = entry->Get( context,
			String::NewFromUtf8Literal( isolate, "view" ) ).ToLocalChecked();
		if( viewVal->IsObject() ) {
			GPUTextureView* w = node::ObjectWrap::Unwrap<GPUTextureView>(
				viewVal.As<Object>() );
			a.view = w->handle_;
			// If this attachment targets the canvas's current swap-chain
			// texture, mark this command encoder as the surface writer.
			// queue.submit will auto-present when it sees a submit that
			// contains this encoder's command buffer.
			if( g_surfaceTexture && w->sourceTexture_ == g_surfaceTexture )
				g_surfaceEncoder = (void*)self;
		}

		// resolveTarget (optional)
		Local<Value> resolveVal;
		if( entry->Get( context,
		      String::NewFromUtf8Literal( isolate, "resolveTarget" ) )
		    .ToLocal( &resolveVal ) && resolveVal->IsObject() ) {
			GPUTextureView* w = node::ObjectWrap::Unwrap<GPUTextureView>(
				resolveVal.As<Object>() );
			a.resolveTarget = w->handle_;
		}

		// loadOp / storeOp — string enums
		Local<Value> loadVal;
		if( entry->Get( context,
		      String::NewFromUtf8Literal( isolate, "loadOp" ) )
		    .ToLocal( &loadVal ) && loadVal->IsString() ) {
			String::Utf8Value s( isolate, loadVal );
			a.loadOp = wgpu_str_to_GPULoadOp( *s, (size_t)s.length(), a.loadOp );
		}
		Local<Value> storeVal;
		if( entry->Get( context,
		      String::NewFromUtf8Literal( isolate, "storeOp" ) )
		    .ToLocal( &storeVal ) && storeVal->IsString() ) {
			String::Utf8Value s( isolate, storeVal );
			a.storeOp = wgpu_str_to_GPUStoreOp( *s, (size_t)s.length(), a.storeOp );
		}

		// clearValue — accept either {r,g,b,a} dict or [r,g,b,a] sequence.
		Local<Value> clearVal;
		if( entry->Get( context,
		      String::NewFromUtf8Literal( isolate, "clearValue" ) )
		    .ToLocal( &clearVal ) ) {
			if( clearVal->IsArray() ) {
				Local<Array> arr4 = clearVal.As<Array>();
				if( arr4->Length() >= 4 ) {
					a.clearValue.r = arr4->Get( context, 0 ).ToLocalChecked()->NumberValue( context ).FromMaybe( 0 );
					a.clearValue.g = arr4->Get( context, 1 ).ToLocalChecked()->NumberValue( context ).FromMaybe( 0 );
					a.clearValue.b = arr4->Get( context, 2 ).ToLocalChecked()->NumberValue( context ).FromMaybe( 0 );
					a.clearValue.a = arr4->Get( context, 3 ).ToLocalChecked()->NumberValue( context ).FromMaybe( 1 );
				}
			} else if( clearVal->IsObject() ) {
				Local<Object> co = clearVal.As<Object>();
				a.clearValue.r = co->Get( context, String::NewFromUtf8Literal( isolate, "r" ) ).ToLocalChecked()->NumberValue( context ).FromMaybe( 0 );
				a.clearValue.g = co->Get( context, String::NewFromUtf8Literal( isolate, "g" ) ).ToLocalChecked()->NumberValue( context ).FromMaybe( 0 );
				a.clearValue.b = co->Get( context, String::NewFromUtf8Literal( isolate, "b" ) ).ToLocalChecked()->NumberValue( context ).FromMaybe( 0 );
				a.clearValue.a = co->Get( context, String::NewFromUtf8Literal( isolate, "a" ) ).ToLocalChecked()->NumberValue( context ).FromMaybe( 1 );
			}
		}
	}

	WGPURenderPassDescriptor rpd = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
	rpd.colorAttachmentCount = n;
	rpd.colorAttachments = arr;

	// depthStencilAttachment — reuse the generated dict reader.
	// Pointer must remain valid across the begin call, so keep the reader
	// instance alive on the stack via a local optional-like holder.
	std::unique_ptr<wgpu_read_GPURenderPassDepthStencilAttachment> dsa_r;
	Local<Value> dsaVal;
	if( desc->Get( context,
	      String::NewFromUtf8Literal( isolate, "depthStencilAttachment" ) )
	    .ToLocal( &dsaVal ) && dsaVal->IsObject() ) {
		Local<Object> dsaObj = dsaVal.As<Object>();
		dsa_r.reset( new wgpu_read_GPURenderPassDepthStencilAttachment(
			isolate, context, dsaObj ) );
		// view is a union (GPUTexture or GPUTextureView) — the generator
		// can't emit unions, so unwrap it here. THREE passes a GPUTextureView.
		Local<Value> dsaViewVal;
		if( dsaObj->Get( context, String::NewFromUtf8Literal( isolate, "view" ) )
		    .ToLocal( &dsaViewVal ) && dsaViewVal->IsObject() ) {
			GPUTextureView* w = node::ObjectWrap::Unwrap<GPUTextureView>(
				dsaViewVal.As<Object>() );
			dsa_r->desc.view = w->handle_;
		}
		rpd.depthStencilAttachment = &dsa_r->desc;
	}

	WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(
		self->handle_, &rpd );
	if( arr ) delete[] arr;

	// Inline WGPU_RETURN_NEW so we can also tag the pass with its parent
	// command encoder for the auto-present path.
	Local<Function> _ctor = getConstructors( isolate )
		->GPURenderPassEncoder_constructor.Get( isolate );
	Local<Object> _obj = _ctor->NewInstance( context, 0, NULL ).ToLocalChecked();
	GPURenderPassEncoder* _w = node::ObjectWrap::Unwrap<GPURenderPassEncoder>( _obj );
	_w->handle_ = pass;
	_w->commandEncoderTag_ = (void*)self;
	args.GetReturnValue().Set( _obj );
}


// ---------- GPUCommandEncoder.finish (hand-written) ----------
//
// The generated version returns a fresh GPUCommandBuffer wrapper but leaves
// the encoder's Dawn handle alive until V8 GC reaps the JS wrapper. The
// WebGPU spec invalidates the encoder at end of finish(), so we release the
// Dawn handle immediately (matches mystral pattern). Dramatically reduces
// steady-state Dawn memory at THREE's per-frame encoder churn.

static void GPUCommandEncoder_finish( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUCommandEncoder );
	// descriptor (label) not yet supported — pass NULL.
	WGPUCommandBuffer cb = wgpuCommandEncoderFinish( self->handle_, NULL );
	// If this is the surface-writing encoder, mark its pass as ended (the
	// app may have called finish without end()'ing the pass, which Dawn
	// would reject — but the marker is what queue.submit checks).
	if( g_surfaceEncoder == (void*)self ) g_surfacePassEnded = true;
	wgpuCommandEncoderRelease( self->handle_ );
	self->handle_ = NULL;
	wgpuLiveDec( "GPUCommandEncoder" );
	WGPU_RETURN_NEW( GPUCommandBuffer, cb );
}


// ---------- GPURenderPassEncoder.end (hand-written) ----------
//
// Spec invalidates the pass encoder at end-of-pass — release the Dawn handle
// here so memory frees on the same tick instead of waiting on V8 GC.

// ---------- GPURenderPassEncoder.draw / drawIndexed (hand-written) ----------
//
// Same body as the generator's version, plus a tally bump so queue.submit
// can log per-frame draw activity. Lets us diagnose "no geometry visible"
// at a glance.

static void GPURenderPassEncoder_draw( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPURenderPassEncoder );
	uint32_t vertexCount   = args.Length() > 0 ? (uint32_t)args[0]->Int32Value( context ).FromMaybe( 0 ) : 0;
	uint32_t instanceCount = args.Length() > 1 ? (uint32_t)args[1]->Int32Value( context ).FromMaybe( 1 ) : 1;
	uint32_t firstVertex   = args.Length() > 2 ? (uint32_t)args[2]->Int32Value( context ).FromMaybe( 0 ) : 0;
	uint32_t firstInstance = args.Length() > 3 ? (uint32_t)args[3]->Int32Value( context ).FromMaybe( 0 ) : 0;
	wgpuRenderPassEncoderDraw( self->handle_, vertexCount, instanceCount, firstVertex, firstInstance );
	g_drawCalls++;
	g_drawVerts += (uint64_t)vertexCount * (uint64_t)instanceCount;
	g_drawInsts += instanceCount;
}

static void GPURenderPassEncoder_drawIndexed( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPURenderPassEncoder );
	uint32_t indexCount    = args.Length() > 0 ? (uint32_t)args[0]->Int32Value( context ).FromMaybe( 0 ) : 0;
	uint32_t instanceCount = args.Length() > 1 ? (uint32_t)args[1]->Int32Value( context ).FromMaybe( 1 ) : 1;
	uint32_t firstIndex    = args.Length() > 2 ? (uint32_t)args[2]->Int32Value( context ).FromMaybe( 0 ) : 0;
	int32_t  baseVertex    = args.Length() > 3 ? (int32_t) args[3]->Int32Value( context ).FromMaybe( 0 ) : 0;
	uint32_t firstInstance = args.Length() > 4 ? (uint32_t)args[4]->Int32Value( context ).FromMaybe( 0 ) : 0;
	wgpuRenderPassEncoderDrawIndexed( self->handle_, indexCount, instanceCount, firstIndex, baseVertex, firstInstance );
	g_drawIndexedCalls++;
	g_drawVerts += (uint64_t)indexCount * (uint64_t)instanceCount;
	g_drawInsts += instanceCount;
}


// Forward-declared in imglib-webgpu/local.h. Called below for the
// surface-writing pass so the imglib driver can replay any retained
// PSI subimage bundles into the same pass before it ends. No-op if the
// driver isn't linked in or no sack Image has been bound to a canvas.
#if defined( INCLUDE_DAWN )
extern "C" void webgpu_image_dispatch_surface_pass( WGPURenderPassEncoder pass );
#endif

static void GPURenderPassEncoder_end( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPURenderPassEncoder );
	// If this pass targets the surface, give the imglib-webgpu driver a
	// chance to execute its retained PSI bundles into the same pass
	// before we end it. Three.js's content has already been encoded by
	// the JS caller; PSI overlay goes after.
	if( g_surfaceEncoder
	    && self->commandEncoderTag_ == g_surfaceEncoder ) {
#if defined( INCLUDE_DAWN )
		webgpu_image_dispatch_surface_pass( self->handle_ );
#endif
		g_surfacePassEnded = true;
	}
	wgpuRenderPassEncoderEnd( self->handle_ );
	wgpuRenderPassEncoderRelease( self->handle_ );
	self->handle_ = NULL;
	wgpuLiveDec( "GPURenderPassEncoder" );
}


// ---------- GPUBuffer.destroy / GPUTexture.destroy (hand-written) ----------
//
// Spec: after .destroy(), the JS object is unusable. The generator's version
// only calls wgpuBufferDestroy/wgpuTextureDestroy (frees the GPU allocation)
// but leaves the Dawn API handle live until V8 GC, which stalls reclaim of
// the per-handle state. Release immediately — same lifecycle pattern as
// finish()/end().

void GPUBuffer_destroy( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUBuffer );
	if( self->handle_ ) {
		wgpuBufferDestroy( self->handle_ );
		wgpuBufferRelease( self->handle_ );
		self->handle_ = NULL;
		wgpuLiveDec( "GPUBuffer" );
	}
}

static void GPUTexture_destroy( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUTexture );
	if( self->handle_ ) {
		wgpuTextureDestroy( self->handle_ );
		wgpuTextureRelease( self->handle_ );
		self->handle_ = NULL;
		wgpuLiveDec( "GPUTexture" );
	}
}


// ---------- GPUTexture.createView (hand-written) ----------
//
// Same body as the generator's version (reuses the generated descriptor
// reader), but tags the resulting GPUTextureView wrapper with its source
// WGPUTexture. The auto-present path in beginRenderPass uses that tag to
// recognise color attachments that target the canvas's swap-chain texture.

static void GPUTexture_createView( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUTexture );
	Local<Object> opts = ( args.Length() > 0 && args[0]->IsObject() )
		? args[0].As<Object>() : Object::New( isolate );
	wgpu_read_GPUTextureViewDescriptor r( isolate, context, opts );
	WGPUTextureView v = wgpuTextureCreateView( self->handle_, &r.desc );

	Local<Function> ctor = getConstructors( isolate )
		->GPUTextureView_constructor.Get( isolate );
	Local<Object> obj = ctor->NewInstance( context, 0, NULL ).ToLocalChecked();
	GPUTextureView* w = node::ObjectWrap::Unwrap<GPUTextureView>( obj );
	w->handle_ = v;
	w->sourceTexture_ = self->handle_;
	args.GetReturnValue().Set( obj );
}


void GPUTexture::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, GPUTexture::New );
	tpl->SetClassName( localStringExternal( isolate, "GPUTexture" ) );
	tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

	wireGenerated_GPUTexture( isolate, tpl );  // attribute getters
	NODE_SET_PROTOTYPE_METHOD( tpl, "createView", GPUTexture_createView );
	NODE_SET_PROTOTYPE_METHOD( tpl, "destroy",    GPUTexture_destroy );

	Local<Function> ctor = tpl->GetFunction( context ).ToLocalChecked();
	getConstructors( isolate )->GPUTexture_constructor.Reset( isolate, ctor );
}


// ---------- GPUQueue.submit (hand-written, auto-present) ----------
//
// Same wire format as the generator's body, plus: if a submitted command
// buffer came from the encoder that wrote to the canvas's current swap-chain
// texture (tracked by beginRenderPass + end/finish), call wgpuSurfacePresent
// and reset state. Mirrors mystral's pattern — JS code that follows browser
// semantics (no explicit present) just works.

static void GPUQueue_submit( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUQueue );
	Local<Array> arr = ( args.Length() > 0 && args[0]->IsArray() )
		? args[0].As<Array>() : Array::New( isolate, 0 );
	size_t n = (size_t)arr->Length();
	WGPUCommandBuffer* bufs = n ? new WGPUCommandBuffer[n]() : NULL;
	for( size_t i = 0; i < n; ++i ) {
		Local<Value> v = arr->Get( context, (uint32_t)i ).ToLocalChecked();
		if( v->IsObject() ) {
			GPUCommandBuffer* w = node::ObjectWrap::Unwrap<GPUCommandBuffer>( v.As<Object>() );
			bufs[ i ] = w->handle_;
		}
	}
	wgpuQueueSubmit( self->handle_, n, bufs );
	if( bufs ) delete[] bufs;

	if( g_surface && g_surfaceTexture && g_surfacePassEnded ) {
		wgpuSurfacePresent( g_surface );
		g_surface = NULL;
		g_surfaceTexture = NULL;
		g_surfaceEncoder = NULL;
		g_surfacePassEnded = false;

		// Per-frame draw tally. Rate-limited so it doesn't spam.
#if 0
		static int frameTick = 0;
		if( ( ++frameTick % 60 ) == 0 ) {
			lprintf( "frame %d: draws=%u drawIndexed=%u verts=%llu insts=%llu",
				frameTick,
				(unsigned)g_drawCalls,
				(unsigned)g_drawIndexedCalls,
				(unsigned long long)g_drawVerts,
				(unsigned long long)g_drawInsts );
		}
		g_drawCalls = 0;
		g_drawIndexedCalls = 0;
		g_drawVerts = 0;
		g_drawInsts = 0;
#endif
	}
}


void GPURenderPassEncoder::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, GPURenderPassEncoder::New );
	tpl->SetClassName( localStringExternal( isolate, "GPURenderPassEncoder" ) );
	tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

	wireGenerated_GPURenderPassEncoder( isolate, tpl );
	NODE_SET_PROTOTYPE_METHOD( tpl, "end",         GPURenderPassEncoder_end );
	NODE_SET_PROTOTYPE_METHOD( tpl, "draw",        GPURenderPassEncoder_draw );
	NODE_SET_PROTOTYPE_METHOD( tpl, "drawIndexed", GPURenderPassEncoder_drawIndexed );

	Local<Function> ctor = tpl->GetFunction( context ).ToLocalChecked();
	getConstructors( isolate )->GPURenderPassEncoder_constructor.Reset( isolate, ctor );
}


void GPUQueue::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, GPUQueue::New );
	tpl->SetClassName( localStringExternal( isolate, "GPUQueue" ) );
	tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

	// submit is hand-written (auto-present). No generated methods to wire.
	NODE_SET_PROTOTYPE_METHOD( tpl, "submit",       GPUQueue_submit );
	NODE_SET_PROTOTYPE_METHOD( tpl, "writeBuffer",  GPUQueue_writeBuffer );
	NODE_SET_PROTOTYPE_METHOD( tpl, "writeTexture", GPUQueue_writeTexture );

	Local<Function> ctor = tpl->GetFunction( context ).ToLocalChecked();
	getConstructors( isolate )->GPUQueue_constructor.Reset( isolate, ctor );
}


void GPUCommandEncoder::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, GPUCommandEncoder::New );
	tpl->SetClassName( localStringExternal( isolate, "GPUCommandEncoder" ) );
	tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

	wireGenerated_GPUCommandEncoder( isolate, tpl );
	NODE_SET_PROTOTYPE_METHOD( tpl, "beginRenderPass", GPUCommandEncoder_beginRenderPass );
	NODE_SET_PROTOTYPE_METHOD( tpl, "finish",          GPUCommandEncoder_finish );

	Local<Function> ctor = tpl->GetFunction( context ).ToLocalChecked();
	getConstructors( isolate )->GPUCommandEncoder_constructor.Reset( isolate, ctor );
}


// ---------- GPUDevice.queue getter (hand-written, [SameObject] cache) ----------
//
// JS spec marks this `[SameObject]` — multiple reads should return the same
// JS object. We cache the wrapper on the GPUDevice's V8-tracked Persistent.
// wgpuDeviceGetQueue does NOT addref, so we balance via wgpuQueueAddRef so
// our wrapper's dtor can release independently of the device's lifecycle.

// readonly attribute Promise<GPUDeviceLostInfo> lost — returns the Promise
// created at device-request time. Spec marks it [SameObject]; since we
// always return the same persistent resolver's promise, that's automatic.
void GPUDevice_lostGetter( Local<Name> property,
	const PropertyCallbackInfo<Value>& info )
{
	Isolate* isolate = info.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	(void)property; (void)context;

	GPUDevice* self = node::ObjectWrap::Unwrap<GPUDevice>( info.This() );
	if( !self->lostState_ ) {
		info.GetReturnValue().Set( Null( isolate ) );
		return;
	}
	Local<Promise::Resolver> resolver = self->lostState_->resolver.Get( isolate );
	info.GetReturnValue().Set( resolver->GetPromise() );
}

// readonly attribute GPUSupportedLimits limits — fetched + cached on first
// access, like the adapter's.
void GPUDevice_limitsGetter( Local<Name> property,
	const PropertyCallbackInfo<Value>& info )
{
	Isolate* isolate = info.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> selfObj = info.This();
	(void)property;

	Local<Private> key = Private::ForApi( isolate,
		String::NewFromUtf8Literal( isolate, "__limitsCache" ) );
	Local<Value> cached;
	if( selfObj->GetPrivate( context, key ).ToLocal( &cached )
	    && cached->IsObject() ) {
		info.GetReturnValue().Set( cached );
		return;
	}

	GPUDevice* self = node::ObjectWrap::Unwrap<GPUDevice>( selfObj );
	WGPULimits lim = WGPU_LIMITS_INIT;
	wgpuDeviceGetLimits( self->handle_, &lim );
	Local<Object> out = wgpu_BuildLimitsObject( isolate, lim );
	(void)selfObj->SetPrivate( context, key, out );
	info.GetReturnValue().Set( out );
}

// readonly attribute GPUSupportedFeatures features — same shape as the
// adapter's. [SameObject] cached on the device JS instance via private key.
void GPUDevice_featuresGetter( Local<Name> property,
	const PropertyCallbackInfo<Value>& info )
{
	Isolate* isolate = info.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> selfObj = info.This();
	(void)property;

	Local<Private> key = Private::ForApi( isolate,
		String::NewFromUtf8Literal( isolate, "__featuresCache" ) );
	Local<Value> cached;
	if( selfObj->GetPrivate( context, key ).ToLocal( &cached )
	    && cached->IsSet() ) {
		info.GetReturnValue().Set( cached );
		return;
	}

	GPUDevice* self = node::ObjectWrap::Unwrap<GPUDevice>( selfObj );
	WGPUSupportedFeatures feats = WGPU_SUPPORTED_FEATURES_INIT;
	wgpuDeviceGetFeatures( self->handle_, &feats );

	Local<Set> set = Set::New( isolate );
	for( size_t i = 0; i < feats.featureCount; i++ ) {
		const char* s = wgpu_str_from_GPUFeatureName( feats.features[ i ] );
		Local<Value> v;
		if( s ) v = String::NewFromUtf8( isolate, s ).ToLocalChecked().As<Value>();
		else    v = Integer::New( isolate, (int)feats.features[ i ] ).As<Value>();
		(void)set->Add( context, v );
	}
	wgpuSupportedFeaturesFreeMembers( feats );

	(void)selfObj->SetPrivate( context, key, set );
	info.GetReturnValue().Set( set );
}

void GPUDevice_queueGetter( Local<Name> property,
	const PropertyCallbackInfo<Value>& info )
{
	Isolate* isolate = info.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> devObj = info.This();
	(void)property;

	// Read-cached check — store the queue wrapper as a private property.
	Local<Private> key = Private::ForApi( isolate,
		String::NewFromUtf8Literal( isolate, "__queueCache" ) );
	Local<Value> cached;
	if( devObj->GetPrivate( context, key ).ToLocal( &cached )
	    && cached->IsObject() ) {
		info.GetReturnValue().Set( cached );
		return;
	}

	GPUDevice* self = node::ObjectWrap::Unwrap<GPUDevice>( devObj );
	WGPUQueue q = wgpuDeviceGetQueue( self->handle_ );
	if( !q ) { info.GetReturnValue().SetNull(); return; }
	wgpuQueueAddRef( q );  // balance our dtor's wgpuQueueRelease

	Local<Function> ctor = getConstructors( isolate )
		->GPUQueue_constructor.Get( isolate );
	Local<Object> obj = ctor->NewInstance( context, 0, NULL ).ToLocalChecked();
	GPUQueue* w = node::ObjectWrap::Unwrap<GPUQueue>( obj );
	w->handle_ = q;

	(void)devObj->SetPrivate( context, key, obj );
	info.GetReturnValue().Set( obj );
}


// ---------- GPUExtent3D union reader ----------
//
// IDL `GPUExtent3D` is `(sequence<GPUIntegerCoordinate> or GPUExtent3DDict)`.
// Three.js (and most existing WebGPU example code) passes it as an array
// `[w, h, d]`. The generator's nested dict reader only knows the dict form,
// so any descriptor with a `size` field gets width/height = 0 when JS
// passes an array. This helper covers both forms and is used by hand-written
// createTexture etc.

static void wgpu_read_extent3D( Isolate* isolate, Local<Context> context,
	Local<Value> v, WGPUExtent3D* out )
{
	out->width  = 1;
	out->height = 1;
	out->depthOrArrayLayers = 1;
	if( v->IsArray() ) {
		Local<Array> arr = v.As<Array>();
		uint32_t n = arr->Length();
		if( n >= 1 ) out->width  = arr->Get( context, 0 ).ToLocalChecked()
			->Uint32Value( context ).FromMaybe( 1 );
		if( n >= 2 ) out->height = arr->Get( context, 1 ).ToLocalChecked()
			->Uint32Value( context ).FromMaybe( 1 );
		if( n >= 3 ) out->depthOrArrayLayers = arr->Get( context, 2 )
			.ToLocalChecked()->Uint32Value( context ).FromMaybe( 1 );
	} else if( v->IsObject() ) {
		Local<Object> o = v.As<Object>();
		Local<Value> w;
		if( o->Get( context, String::NewFromUtf8Literal( isolate, "width" ) ).ToLocal( &w ) && w->IsNumber() )
			out->width = w->Uint32Value( context ).FromMaybe( 1 );
		Local<Value> h;
		if( o->Get( context, String::NewFromUtf8Literal( isolate, "height" ) ).ToLocal( &h ) && h->IsNumber() )
			out->height = h->Uint32Value( context ).FromMaybe( 1 );
		Local<Value> d;
		if( o->Get( context, String::NewFromUtf8Literal( isolate, "depthOrArrayLayers" ) ).ToLocal( &d ) && d->IsNumber() )
			out->depthOrArrayLayers = d->Uint32Value( context ).FromMaybe( 1 );
	}
}


// ---------- GPUDevice.createTexture (hand-written) ----------
//
// `GPUTextureDescriptor.size` is the union above. Reuse the generated reader
// for everything else (label, mipLevelCount, sampleCount, dimension, format,
// usage, viewFormats), then patch `size` from our union-aware helper.

void GPUDevice_createTexture( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUDevice );
	if( args.Length() < 1 || !args[ 0 ]->IsObject() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "createTexture requires a descriptor" ) ) );
		return;
	}
	Local<Object> opts = args[ 0 ].As<Object>();
	wgpu_read_GPUTextureDescriptor reader( isolate, context, opts );

	Local<Value> sizeVal;
	if( opts->Get( context, String::NewFromUtf8Literal( isolate, "size" ) )
	     .ToLocal( &sizeVal ) ) {
		wgpu_read_extent3D( isolate, context, sizeVal, &reader.desc.size );
	}

	WGPUTexture tex = wgpuDeviceCreateTexture( self->handle_, &reader.desc );
	WGPU_RETURN_NEW( GPUTexture, tex );
}


// ---------- GPUDevice.popErrorScope (hand-written) ----------
//
// Promise<GPUError?>. Resolves with null if the scope captured no error,
// or a {message, type} object describing the error. We don't wrap a real
// GPUError class — THREE only checks for falsy/truthy and reads .message.

static void GPUDevice_OnPopErrorScope( WGPUPopErrorScopeStatus status,
	WGPUErrorType type, WGPUStringView msg,
	void* userdata1, void* userdata2 )
{
	struct PopRequest {
		Isolate* isolate;
		Persistent<Promise::Resolver> resolver;
	};
	PopRequest* req = (PopRequest*)userdata1;
	(void)userdata2;
	wgpu_async_end();

	Isolate* isolate = req->isolate;
	HandleScope hs( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	Context::Scope cs( context );
	Local<Promise::Resolver> resolver = req->resolver.Get( isolate );

	(void)status;
	if( type == WGPUErrorType_NoError ) {
		(void)resolver->Resolve( context, Null( isolate ) );
	} else {
		const char* tname =
			type == WGPUErrorType_Validation  ? "validation"   :
			type == WGPUErrorType_OutOfMemory ? "out-of-memory":
			type == WGPUErrorType_Internal    ? "internal"     :
			"unknown";
		Local<Object> errObj = Object::New( isolate );
		SET( errObj, "type",
			String::NewFromUtf8( isolate, tname ).ToLocalChecked() );
		SET( errObj, "message", WGPU_StringViewToV8( isolate, msg ) );
		(void)resolver->Resolve( context, errObj );
	}
	req->resolver.Reset();
	delete req;
}

void GPUDevice_popErrorScope( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUDevice );

	Local<Promise::Resolver> resolver = Promise::Resolver::New( context )
		.ToLocalChecked();

	struct PopRequest {
		Isolate* isolate;
		Persistent<Promise::Resolver> resolver;
	};
	PopRequest* req = new PopRequest();
	req->isolate = isolate;
	req->resolver.Reset( isolate, resolver );

	WGPUPopErrorScopeCallbackInfo info = {};
	info.mode = WGPUCallbackMode_AllowProcessEvents;
	info.callback = GPUDevice_OnPopErrorScope;
	info.userdata1 = req;

	wgpu_async_begin();
	wgpuDevicePopErrorScope( self->handle_, info );

	args.GetReturnValue().Set( resolver->GetPromise() );
}


// ---------- GPUDevice.createBindGroup (hand-written) ----------
//
// IDL `GPUBindGroupEntry.resource` is the union
//   (GPUSampler or GPUTextureView or GPUBufferBinding or GPUExternalTexture).
// Dawn's WGPUBindGroupEntry has separate optional fields per type
// (buffer/sampler/textureView). We dispatch on what kind of JS object is
// supplied. GPUExternalTexture is skipped — uncommon and would need a
// chained struct.

// Check whether a JS object's prototype matches a wrapper-class function.
// Used to discriminate the bind-group resource union by class.
static bool wgpu_isInstanceOf( Isolate* isolate, Local<Object> obj,
	Persistent<Function>& ctorSlot )
{
	if( ctorSlot.IsEmpty() ) return false;
	Local<Context> context = isolate->GetCurrentContext();
	Local<Function> f = ctorSlot.Get( isolate );
	Local<Value> proto;
	if( !f->Get( context, String::NewFromUtf8Literal( isolate, "prototype" ) )
	     .ToLocal( &proto ) ) return false;
	return obj->GetPrototype()->StrictEquals( proto );
}

void GPUDevice_createBindGroup( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUDevice );
	if( args.Length() < 1 || !args[ 0 ]->IsObject() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "createBindGroup requires a descriptor" ) ) );
		return;
	}
	Local<Object> opts = args[ 0 ].As<Object>();
	constructorSet* cset = getConstructors( isolate );

	// Label — Utf8Value is non-copyable, so construct the holder once with
	// the value we'll actually use rather than trying to reassign.
	Local<Value> labelVal;
	bool hasLabel = opts->Get( context, String::NewFromUtf8Literal( isolate, "label" ) )
		.ToLocal( &labelVal ) && labelVal->IsString();
	WGPU_StringViewHolder labelH( isolate,
		hasLabel ? labelVal : Local<Value>::Cast( String::Empty( isolate ) ) );

	// Layout — required, GPUBindGroupLayout handle.
	Local<Value> layoutVal = opts->Get( context,
		String::NewFromUtf8Literal( isolate, "layout" ) ).ToLocalChecked();
	if( !layoutVal->IsObject() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "createBindGroup: layout is required" ) ) );
		return;
	}
	GPUBindGroupLayout* layout = node::ObjectWrap::Unwrap<GPUBindGroupLayout>(
		layoutVal.As<Object>() );

	// Entries — sequence of {binding, resource}.
	Local<Value> entriesVal = opts->Get( context,
		String::NewFromUtf8Literal( isolate, "entries" ) ).ToLocalChecked();
	if( !entriesVal->IsArray() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "createBindGroup: entries must be an array" ) ) );
		return;
	}
	Local<Array> entriesArr = entriesVal.As<Array>();
	uint32_t n = entriesArr->Length();

	WGPUBindGroupEntry* entries = n ? new WGPUBindGroupEntry[ n ]() : NULL;

	for( uint32_t i = 0; i < n; i++ ) {
		Local<Value> entryVal = entriesArr->Get( context, i ).ToLocalChecked();
		if( !entryVal->IsObject() ) continue;
		Local<Object> entry = entryVal.As<Object>();
		WGPUBindGroupEntry& e = entries[ i ];

		e.binding = entry->Get( context, String::NewFromUtf8Literal( isolate, "binding" ) )
			.ToLocalChecked()->Uint32Value( context ).FromMaybe( 0 );

		Local<Value> resourceVal = entry->Get( context,
			String::NewFromUtf8Literal( isolate, "resource" ) ).ToLocalChecked();
		if( !resourceVal->IsObject() ) continue;
		Local<Object> resource = resourceVal.As<Object>();

		if( wgpu_isInstanceOf( isolate, resource, cset->GPUSampler_constructor ) ) {
			e.sampler = node::ObjectWrap::Unwrap<GPUSampler>( resource )->handle_;
		} else if( wgpu_isInstanceOf( isolate, resource, cset->GPUTextureView_constructor ) ) {
			e.textureView = node::ObjectWrap::Unwrap<GPUTextureView>( resource )->handle_;
		} else {
			// Treat as GPUBufferBinding dict: {buffer, offset?, size?}
			Local<Value> bufVal;
			if( resource->Get( context, String::NewFromUtf8Literal( isolate, "buffer" ) )
			     .ToLocal( &bufVal ) && bufVal->IsObject() ) {
				e.buffer = node::ObjectWrap::Unwrap<GPUBuffer>( bufVal.As<Object>() )->handle_;
			}
			Local<Value> offVal;
			if( resource->Get( context, String::NewFromUtf8Literal( isolate, "offset" ) )
			     .ToLocal( &offVal ) && offVal->IsNumber() ) {
				e.offset = (uint64_t)offVal->IntegerValue( context ).FromMaybe( 0 );
			}
			Local<Value> szVal;
			if( resource->Get( context, String::NewFromUtf8Literal( isolate, "size" ) )
			     .ToLocal( &szVal ) && szVal->IsNumber() ) {
				e.size = (uint64_t)szVal->IntegerValue( context ).FromMaybe( 0 );
			} else {
				e.size = WGPU_WHOLE_SIZE;  // spec default
			}
		}
	}

	WGPUBindGroupDescriptor desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
	if( hasLabel ) desc.label = labelH.view();
	desc.layout = layout->handle_;
	desc.entryCount = n;
	desc.entries = entries;

	WGPUBindGroup bg = wgpuDeviceCreateBindGroup( self->handle_, &desc );
	if( entries ) delete[] entries;

	WGPU_RETURN_NEW( GPUBindGroup, bg );
}


// ---------- GPUDevice.createShaderModule (hand-written) ----------
//
// IDL puts `code` directly on GPUShaderModuleDescriptor; Dawn puts the WGSL
// source in a WGPUShaderSourceWGSL chained struct attached via nextInChain.
// The generated reader can't model chained sources, so we hand-wrap.

void GPUDevice_createShaderModule( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUDevice );
	if( args.Length() < 1 || !args[ 0 ]->IsObject() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "createShaderModule requires a descriptor" ) ) );
		return;
	}
	Local<Object> opts = args[ 0 ].As<Object>();

	// Pull the WGSL source. SPIRV path (uint32 array) could be added later.
	Local<Value> codeVal = opts->Get( context,
		String::NewFromUtf8Literal( isolate, "code" ) ).ToLocalChecked();
	if( !codeVal->IsString() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "createShaderModule: code must be a string (WGSL)" ) ) );
		return;
	}
	WGPU_StringViewHolder codeH( isolate, codeVal );

	WGPUShaderSourceWGSL wgsl = WGPU_SHADER_SOURCE_WGSL_INIT;
	wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
	wgsl.code = codeH.view();

	// Reuse the generated reader for the label etc.
	wgpu_read_GPUShaderModuleDescriptor reader( isolate, context, opts );
	reader.desc.nextInChain = &wgsl.chain;

	WGPUShaderModule mod = wgpuDeviceCreateShaderModule( self->handle_, &reader.desc );
	WGPU_RETURN_NEW( GPUShaderModule, mod );
}


// ---------- GPUDevice.createRenderPipeline / createComputePipeline ----------
//
// IDL `layout` is union (GPUPipelineLayout | "auto"). Dawn just takes a
// WGPUPipelineLayout, where NULL means auto. Generated reader skips the
// union; we patch in the layout after construction.

static void GPUDevice_patchLayout( Isolate* isolate, Local<Context> context,
	Local<Object> opts, WGPUPipelineLayout* outLayout )
{
	*outLayout = NULL;
	Local<Value> layoutVal;
	if( !opts->Get( context, String::NewFromUtf8Literal( isolate, "layout" ) )
	     .ToLocal( &layoutVal ) ) return;
	if( layoutVal->IsObject() ) {
		GPUPipelineLayout* w = node::ObjectWrap::Unwrap<GPUPipelineLayout>(
			layoutVal.As<Object>() );
		*outLayout = w->handle_;
	}
	// String "auto" or anything else → NULL → Dawn defaults to auto.
}

void GPUDevice_createRenderPipeline( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUDevice );
	if( args.Length() < 1 || !args[ 0 ]->IsObject() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "createRenderPipeline requires a descriptor" ) ) );
		return;
	}
	Local<Object> opts = args[ 0 ].As<Object>();
	wgpu_read_GPURenderPipelineDescriptor reader( isolate, context, opts );
	GPUDevice_patchLayout( isolate, context, opts, &reader.desc.layout );

	WGPURenderPipeline p = wgpuDeviceCreateRenderPipeline( self->handle_, &reader.desc );
	WGPU_RETURN_NEW( GPURenderPipeline, p );
}

void GPUDevice_createComputePipeline( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUDevice );
	if( args.Length() < 1 || !args[ 0 ]->IsObject() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "createComputePipeline requires a descriptor" ) ) );
		return;
	}
	Local<Object> opts = args[ 0 ].As<Object>();
	wgpu_read_GPUComputePipelineDescriptor reader( isolate, context, opts );
	GPUDevice_patchLayout( isolate, context, opts, &reader.desc.layout );

	WGPUComputePipeline p = wgpuDeviceCreateComputePipeline( self->handle_, &reader.desc );
	WGPU_RETURN_NEW( GPUComputePipeline, p );
}


// ---------- GPUBuffer.mapAsync / getMappedRange / unmap (hand-written) ----------
//
// IDL:
//   Promise<undefined> mapAsync(GPUMapModeFlags mode, optional offset = 0, optional size);
//   ArrayBuffer getMappedRange(optional offset = 0, optional size);
//   undefined unmap();
//
// getMappedRange returns an ArrayBuffer that aliases Dawn's mapped pointer
// (zero-copy). On unmap, we detach the ArrayBuffer so JS-side reads/writes
// don't fault on memory the GPU has reclaimed.

static void wgpu_NoopBackingDeleter( void* data, size_t length, void* info ) {
	// Dawn owns the memory backing this ArrayBuffer; V8 must not free it.
	(void)data; (void)length; (void)info;
}

static void GPUBuffer_OnMapAsync( WGPUMapAsyncStatus status, WGPUStringView msg,
	void* userdata1, void* userdata2 )
{
	struct MapRequest {
		Isolate* isolate;
		Persistent<Promise::Resolver> resolver;
	};
	MapRequest* req = (MapRequest*)userdata1;
	(void)userdata2;
	Isolate* isolate = req->isolate;
	wgpu_async_end();

	HandleScope hs( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	Context::Scope cs( context );

	Local<Promise::Resolver> resolver = req->resolver.Get( isolate );
	if( status == WGPUMapAsyncStatus_Success ) {
		(void)resolver->Resolve( context, Undefined( isolate ) );
	} else {
		const char* m = msg.data ? msg.data : "mapAsync failed";
		int len = (int)( msg.length == SIZE_MAX
			? strlen( m ) : msg.length );
		Local<String> mstr = String::NewFromUtf8( isolate, m,
			NewStringType::kNormal, len ).ToLocalChecked();
		(void)resolver->Reject( context, Exception::Error( mstr ) );
	}
	req->resolver.Reset();
	delete req;
}

void GPUBuffer_mapAsync( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUBuffer );
	uint32_t mode = args.Length() > 0
		? args[ 0 ]->Uint32Value( context ).FromMaybe( 0 ) : 0;
	uint64_t offset = args.Length() > 1
		? (uint64_t)args[ 1 ]->IntegerValue( context ).FromMaybe( 0 ) : 0;
	uint64_t size = args.Length() > 2
		? (uint64_t)args[ 2 ]->IntegerValue( context ).FromMaybe( WGPU_WHOLE_MAP_SIZE )
		: WGPU_WHOLE_MAP_SIZE;

	Local<Promise::Resolver> resolver = Promise::Resolver::New( context )
		.ToLocalChecked();

	struct MapRequest {
		Isolate* isolate;
		Persistent<Promise::Resolver> resolver;
	};
	MapRequest* req = new MapRequest();
	req->isolate = isolate;
	req->resolver.Reset( isolate, resolver );

	WGPUBufferMapCallbackInfo info = {};
	info.mode = WGPUCallbackMode_AllowProcessEvents;
	info.callback = GPUBuffer_OnMapAsync;
	info.userdata1 = req;

	wgpu_async_begin();
	wgpuBufferMapAsync( self->handle_, (WGPUMapMode)mode,
		(size_t)offset, (size_t)size, info );

	args.GetReturnValue().Set( resolver->GetPromise() );
}

void GPUBuffer_getMappedRange( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUBuffer );
	uint64_t offset = args.Length() > 0
		? (uint64_t)args[ 0 ]->IntegerValue( context ).FromMaybe( 0 ) : 0;
	uint64_t size = args.Length() > 1
		? (uint64_t)args[ 1 ]->IntegerValue( context ).FromMaybe( WGPU_WHOLE_MAP_SIZE )
		: WGPU_WHOLE_MAP_SIZE;

	// Dawn returns a writable mapped pointer; use the const variant for
	// read-only mappings (no way to tell from here without tracking mode).
	void* ptr = wgpuBufferGetMappedRange( self->handle_,
		(size_t)offset, (size_t)size );
	if( !ptr ) {
		// Likely a read-only mapping; fall back to const variant.
		const void* cptr = wgpuBufferGetConstMappedRange( self->handle_,
			(size_t)offset, (size_t)size );
		ptr = const_cast<void*>( cptr );
	}
	if( !ptr ) {
		args.GetReturnValue().Set( Null( isolate ) );
		return;
	}

	// Resolve the actual byte length we asked for. Dawn doesn't tell us;
	// for WGPU_WHOLE_MAP_SIZE we'd ideally query bufferSize - offset.
	size_t byteLen = (size == WGPU_WHOLE_MAP_SIZE)
		? (size_t)( wgpuBufferGetSize( self->handle_ ) - offset )
		: (size_t)size;

	std::unique_ptr<BackingStore> store = ArrayBuffer::NewBackingStore(
		ptr, byteLen, wgpu_NoopBackingDeleter, NULL );
	Local<ArrayBuffer> ab = ArrayBuffer::New( isolate, std::move( store ) );

	// Track the most recently issued ArrayBuffer on the JS-side buffer so
	// unmap can detach it. Stored as a private property.
	Local<Private> key = Private::ForApi( isolate,
		String::NewFromUtf8Literal( isolate, "__lastMappedAB" ) );
	(void)getFCIHolder( args )->SetPrivate( context, key, ab );

	args.GetReturnValue().Set( ab );
}

void GPUBuffer_unmap( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUBuffer );

	// Detach any outstanding mapped ArrayBuffer so subsequent JS accesses
	// throw cleanly instead of touching freed Dawn memory.
	Local<Private> key = Private::ForApi( isolate,
		String::NewFromUtf8Literal( isolate, "__lastMappedAB" ) );
	Local<Value> abVal;
	if( getFCIHolder( args )->GetPrivate( context, key ).ToLocal( &abVal )
	    && abVal->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = abVal.As<ArrayBuffer>();
		if( ab->IsDetachable() ) (void)ab->Detach( Local<Value>() );
		(void)getFCIHolder( args )->DeletePrivate( context, key );
	}
	wgpuBufferUnmap( self->handle_ );
}


// ---------- GPUQueue.writeTexture (hand-written) ----------
//
// IDL: writeTexture(destination: GPUTexelCopyTextureInfo, data: AllowSharedBufferSource,
//                   dataLayout: GPUTexelCopyBufferLayout, size: GPUExtent3D)
// dataLayout fields land in dawn as a separate WGPUTexelCopyBufferLayout struct.
// `size` is the GPUExtent3D union (we reuse wgpu_read_extent3D).

static void wgpu_read_GPUOrigin3D_union( Isolate* isolate, Local<Context> context,
	Local<Value> v, WGPUOrigin3D* out )
{
	out->x = 0; out->y = 0; out->z = 0;
	if( v->IsArray() ) {
		Local<Array> arr = v.As<Array>();
		uint32_t n = arr->Length();
		if( n >= 1 ) out->x = arr->Get( context, 0 ).ToLocalChecked()
			->Uint32Value( context ).FromMaybe( 0 );
		if( n >= 2 ) out->y = arr->Get( context, 1 ).ToLocalChecked()
			->Uint32Value( context ).FromMaybe( 0 );
		if( n >= 3 ) out->z = arr->Get( context, 2 ).ToLocalChecked()
			->Uint32Value( context ).FromMaybe( 0 );
	} else if( v->IsObject() ) {
		Local<Object> o = v.As<Object>();
		Local<Value> t;
		if( o->Get( context, String::NewFromUtf8Literal( isolate, "x" ) ).ToLocal( &t ) && t->IsNumber() )
			out->x = t->Uint32Value( context ).FromMaybe( 0 );
		if( o->Get( context, String::NewFromUtf8Literal( isolate, "y" ) ).ToLocal( &t ) && t->IsNumber() )
			out->y = t->Uint32Value( context ).FromMaybe( 0 );
		if( o->Get( context, String::NewFromUtf8Literal( isolate, "z" ) ).ToLocal( &t ) && t->IsNumber() )
			out->z = t->Uint32Value( context ).FromMaybe( 0 );
	}
}

void GPUQueue_writeTexture( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUQueue );
	if( args.Length() < 4 ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "writeTexture requires (destination, data, dataLayout, size)" ) ) );
		return;
	}

	// destination: GPUTexelCopyTextureInfo { texture, mipLevel?, origin?, aspect? }
	WGPUTexelCopyTextureInfo dest = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
	if( args[ 0 ]->IsObject() ) {
		Local<Object> d = args[ 0 ].As<Object>();
		Local<Value> texVal = d->Get( context,
			String::NewFromUtf8Literal( isolate, "texture" ) ).ToLocalChecked();
		if( texVal->IsObject() )
			dest.texture = node::ObjectWrap::Unwrap<GPUTexture>( texVal.As<Object>() )->handle_;
		Local<Value> mipVal;
		if( d->Get( context, String::NewFromUtf8Literal( isolate, "mipLevel" ) )
		     .ToLocal( &mipVal ) && mipVal->IsNumber() )
			dest.mipLevel = mipVal->Uint32Value( context ).FromMaybe( 0 );
		Local<Value> originVal;
		if( d->Get( context, String::NewFromUtf8Literal( isolate, "origin" ) )
		     .ToLocal( &originVal ) )
			wgpu_read_GPUOrigin3D_union( isolate, context, originVal, &dest.origin );
		Local<Value> aspectVal;
		if( d->Get( context, String::NewFromUtf8Literal( isolate, "aspect" ) )
		     .ToLocal( &aspectVal ) && aspectVal->IsString() ) {
			String::Utf8Value s( isolate, aspectVal );
			dest.aspect = wgpu_str_to_GPUTextureAspect( *s, (size_t)s.length(), dest.aspect );
		}
	}

	// data: ArrayBuffer or TypedArray
	void* basePtr   = NULL;
	size_t baseSize = 0;
	size_t srcOff   = 0;
	Local<Value> data = args[ 1 ];
	if( data->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = data.As<ArrayBuffer>();
		basePtr  = ab->Data();
		baseSize = ab->ByteLength();
	} else if( data->IsArrayBufferView() ) {
		Local<ArrayBufferView> view = data.As<ArrayBufferView>();
		basePtr  = view->Buffer()->Data();
		baseSize = view->ByteLength();
		srcOff   = view->ByteOffset();
	} else {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "writeTexture: data must be ArrayBuffer or TypedArray" ) ) );
		return;
	}

	// dataLayout: GPUTexelCopyBufferLayout { offset?, bytesPerRow?, rowsPerImage? }
	WGPUTexelCopyBufferLayout layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
	if( args[ 2 ]->IsObject() ) {
		Local<Object> dl = args[ 2 ].As<Object>();
		Local<Value> t;
		if( dl->Get( context, String::NewFromUtf8Literal( isolate, "offset" ) ).ToLocal( &t ) && t->IsNumber() )
			layout.offset = (uint64_t)t->IntegerValue( context ).FromMaybe( 0 );
		if( dl->Get( context, String::NewFromUtf8Literal( isolate, "bytesPerRow" ) ).ToLocal( &t ) && t->IsNumber() )
			layout.bytesPerRow = (uint32_t)t->Uint32Value( context ).FromMaybe( WGPU_COPY_STRIDE_UNDEFINED );
		if( dl->Get( context, String::NewFromUtf8Literal( isolate, "rowsPerImage" ) ).ToLocal( &t ) && t->IsNumber() )
			layout.rowsPerImage = (uint32_t)t->Uint32Value( context ).FromMaybe( WGPU_COPY_STRIDE_UNDEFINED );
	}

	// size: GPUExtent3D union
	WGPUExtent3D writeSize;
	wgpu_read_extent3D( isolate, context, args[ 3 ], &writeSize );

	const uint8_t* src = (const uint8_t*)basePtr + srcOff;
	wgpuQueueWriteTexture( self->handle_, &dest, src, baseSize - srcOff,
		&layout, &writeSize );
}


// ---------- GPUQueue.writeBuffer (hand-written) ----------
//
// IDL: writeBuffer(buffer, bufferOffset, data, dataOffset?, size?)
// data is AllowSharedBufferSource (ArrayBuffer / TypedArray / DataView / SAB).
// Dawn: wgpuQueueWriteBuffer(queue, buffer, bufferOffset, ptr, size).
// We resolve the JS data view to a pointer + length, apply dataOffset/size.

void GPUQueue_writeBuffer( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( GPUQueue );
	if( args.Length() < 3 ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "writeBuffer requires (buffer, bufferOffset, data)" ) ) );
		return;
	}
	if( !args[ 0 ]->IsObject() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "writeBuffer: buffer arg must be a GPUBuffer" ) ) );
		return;
	}
	GPUBuffer* dest = node::ObjectWrap::Unwrap<GPUBuffer>( args[ 0 ].As<Object>() );
	uint64_t bufferOffset = (uint64_t)args[ 1 ]->IntegerValue( context ).FromMaybe( 0 );

	// Resolve data view. Per WebGPU spec, dataOffset/size are interpreted in
	// ELEMENTS when data is a TypedArray, BYTES when data is ArrayBuffer or
	// DataView. We compute bytes-per-element from V8's typed-array kind.
	void*  basePtr   = NULL;
	size_t baseSize  = 0;
	size_t srcOffset = 0;
	size_t elemSize  = 1;   // bytes per dataOffset/size unit
	Local<Value> data = args[ 2 ];
	if( data->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = data.As<ArrayBuffer>();
		basePtr  = ab->Data();
		baseSize = ab->ByteLength();
	} else if( data->IsArrayBufferView() ) {
		Local<ArrayBufferView> view = data.As<ArrayBufferView>();
		basePtr   = view->Buffer()->Data();
		baseSize  = view->ByteLength();
		srcOffset = view->ByteOffset();
		// DataView is byte-indexed (elemSize stays 1). Typed arrays are
		// element-indexed; element size derived from view properties.
		if( data->IsDataView() ) {
			elemSize = 1;
		} else if( data->IsTypedArray() ) {
			Local<TypedArray> ta = data.As<TypedArray>();
			size_t len = ta->Length();
			elemSize = len ? ( baseSize / len ) : 1;
		}
	} else {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "writeBuffer: data must be an ArrayBuffer or TypedArray" ) ) );
		return;
	}

	// dataOffset in source units (elements for TypedArray, bytes else).
	uint64_t dataOffsetUnits = 0;
	if( args.Length() > 3 && args[ 3 ]->IsNumber() )
		dataOffsetUnits = (uint64_t)args[ 3 ]->IntegerValue( context ).FromMaybe( 0 );
	uint64_t dataOffsetBytes = dataOffsetUnits * elemSize;

	// size in source units. Default: rest of the buffer after dataOffset.
	uint64_t sizeUnits = ( baseSize - dataOffsetBytes ) / elemSize;
	if( args.Length() > 4 && args[ 4 ]->IsNumber() )
		sizeUnits = (uint64_t)args[ 4 ]->IntegerValue( context ).FromMaybe( 0 );
	uint64_t sizeBytes = sizeUnits * elemSize;

	const uint8_t* src = (const uint8_t*)basePtr + srcOffset + dataOffsetBytes;
	wgpuQueueWriteBuffer( self->handle_, dest->handle_, bufferOffset, src, (size_t)sizeBytes );
}


// ---------- entry point ----------

void InitWebGPU( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();

	// Register classes (stores constructor templates on the isolate's
	// constructorSet but does not export the class names — JS sees only the
	// `gpu` singleton instance, matching browser conventions).
	GPU::Init( isolate, exports );
	GPUAdapter::Init( isolate, exports );
	GPUDevice::Init( isolate, exports );
	GPUBuffer::Init( isolate, exports );
	GPUSampler::Init( isolate, exports );
	GPUTexture::Init( isolate, exports );
	GPUTextureView::Init( isolate, exports );
	GPUQuerySet::Init( isolate, exports );
	GPUCommandBuffer::Init( isolate, exports );
	GPUCommandEncoder::Init( isolate, exports );
	GPUComputePassEncoder::Init( isolate, exports );
	GPURenderPassEncoder::Init( isolate, exports );
	GPURenderBundleEncoder::Init( isolate, exports );
	GPUBindGroupLayout::Init( isolate, exports );
	GPURenderPipeline::Init( isolate, exports );
	GPUComputePipeline::Init( isolate, exports );
	GPUQueue::Init( isolate, exports );
	GPURenderBundle::Init( isolate, exports );
	GPUBindGroup::Init( isolate, exports );
	GPUPipelineLayout::Init( isolate, exports );
	GPUShaderModule::Init( isolate, exports );
	GPUCanvasContext::Init( isolate, exports );

	// Create a WGPUInstance for this isolate.
	WGPUInstance instance = wgpuCreateInstance( NULL );
	if( !instance ) {
		lprintf( "wgpuCreateInstance returned NULL — WebGPU disabled" );
		return;
	}

	// Build the singleton GPU object and inject the instance handle.
	Local<Function> gpuCtor = getConstructors( isolate )
		->GPU_constructor.Get( isolate );
	Local<Object> gpuObj = gpuCtor->NewInstance( context, 0, NULL )
		.ToLocalChecked();
	GPU* gpu = node::ObjectWrap::Unwrap<GPU>( gpuObj );
	gpu->handle_ = instance;

	SET_READONLY( exports, "gpu", gpuObj );

	// Also attach to the existing global `navigator`. Node 21+ exposes a
	// built-in `navigator` with hardwareConcurrency / language / userAgent /
	// platform — we add `gpu` so spec-shaped WebGPU code (and THREE's
	// WebGPURenderer) sees `navigator.gpu` like in a browser. If for some
	// reason there is no navigator, we install one.
	Local<Object> global = context->Global();
	Local<String> navKey = String::NewFromUtf8Literal( isolate, "navigator" );
	Local<Value> navVal;
	Local<Object> navigatorObj;
	if( global->Get( context, navKey ).ToLocal( &navVal ) && navVal->IsObject() ) {
		navigatorObj = navVal.As<Object>();
	} else {
		navigatorObj = Object::New( isolate );
		(void)global->Set( context, navKey, navigatorObj );
	}
	(void)navigatorObj->Set( context,
		String::NewFromUtf8Literal( isolate, "gpu" ), gpuObj );

	// Install bitmask namespace constants on globalThis so JS code can use
	// the spec-shaped names (GPUBufferUsage.MAP_READ, GPUShaderStage.VERTEX, ...).
	wireGeneratedConstants( isolate, global );

	// Schedule wgpuInstanceProcessEvents on every uv tick so Dawn callbacks
	// land on the V8 thread without explicit queue plumbing.
	DawnPump* pump = new DawnPump();  // leaked at shutdown — single object
	pump->instance = instance;
	pump->pendingAsync = 0;
	uv_loop_t* loop = uv_default_loop();
	uv_check_init( loop, &pump->check );
	pump->check.data = pump;
	uv_check_start( &pump->check, DawnPumpCb );
	// Start unref'd so an idle process can exit. wgpu_async_begin() refs
	// while async ops are in flight; wgpu_async_end() unrefs when the last
	// one resolves. That keeps Node alive across awaits but lets it exit
	// once all WebGPU work has settled.
	uv_unref( (uv_handle_t*)&pump->check );
	g_dawnPump = pump;

	lprintf( "WebGPU initialized — sack.gpu available" );
}
