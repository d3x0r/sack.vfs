// OpenXR session — implementation.
//
// Binds an XrSession to the app's existing Dawn D3D12 device, so runtime-owned
// swapchain images become ordinary WGPUTextures the rest of the stack can render
// into. No copies and no second device: OpenXR is handed Dawn's own ID3D12Device
// and command queue, which is what makes SharedTextureMemoryD3D12Resource's
// "must be created from the same ID3D12Device" requirement free rather than a
// problem.
//
// The frame loop is driven from JS (see xr-shell/document.mjs) — this file
// exposes primitives, not a loop:
//
//     const s     = sack.xr.beginSession( device );
//     s.poll();                       // pump session state; must reach "FOCUSED"-ish
//     const frame = s.waitFrame();    // paced by the runtime; null when not running
//     const tex   = s.acquire( eye ); // GPUTexture for this eye, this frame
//     s.release( eye );
//     s.endFrame();

// This TU sits between two macro sets that each want the same identifiers, so
// both the guard below and the include order matter.
//
// 1. SACK's sharemem.h defines Release(p) -> ReleaseEx(p, file, line), which
//    rewrites IUnknown::Release() and detonates <wrl.h> ("no member named
//    'ReleaseEx' in 'IUnknown'"). sharemem.h ships this guard for exactly that
//    case; it also suppresses Allocate/Reallocate, none of which this TU uses.
//    (New is handled separately — global.h already #undefs it so Object::New
//    keeps working.)
#define FIX_RELEASE_COM_COLLISION

// 2. global.h #undefs COM's THIS (plain `void` from combaseapi.h) and redefines
//    it as a function-like V8 helper. D3D12Backend.h reaches d3d10shader.h via
//    d3d11on12.h -> d3d11.h -> d3d10.h, and those use bare THIS in their C-style
//    COM declarations — so they must be parsed before global.h redefines it, or
//    every one of them fails with "unknown type name 'THIS'".
//
// Both symptoms report only against Windows SDK headers, with nothing pointing
// back at this file, so leave this ordering alone.
//
// 3. Going first also means this TU, rather than global.h, is responsible for
//    taming windows.h:
//      NOMINMAX            — its min/max macros eat std::numeric_limits<>::min()
//                            inside V8's headers.
//      WIN32_LEAN_AND_MEAN — stops it pulling winsock.h, which then redefines
//                            sockaddr/fd_set/timeval against sack's winsock2.h.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "webgpu/webgpu.h"
// The D3D12 resource import descriptor exists only in Dawn's C++ native API —
// webgpu.h has the SType enum but no matching C struct — so this TU is the one
// place that reaches for webgpu_cpp.h instead of the C header used elsewhere.
#include "dawn/webgpu_cpp.h"
#include "dawn/native/D3D12Backend.h"

#include "../../global.h"
#include "xr_session.h"
#include "../webgpu/webgpu_module.h"
#include "../webgpu/webgpu_bindings.h"

#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

// ----------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------

static std::string xrs_result( XrInstance inst, XrResult res ) {
	if( inst != XR_NULL_HANDLE ) {
		char buf[ XR_MAX_RESULT_STRING_SIZE ];
		if( XR_SUCCEEDED( xrResultToString( inst, res, buf ) ) )
			return std::string( buf );
	}
	return std::to_string( (int)res );
}

static const char* xrs_state_name( XrSessionState s ) {
	switch( s ) {
	case XR_SESSION_STATE_IDLE:         return "IDLE";
	case XR_SESSION_STATE_READY:        return "READY";
	case XR_SESSION_STATE_SYNCHRONIZED: return "SYNCHRONIZED";
	case XR_SESSION_STATE_VISIBLE:      return "VISIBLE";
	case XR_SESSION_STATE_FOCUSED:      return "FOCUSED";
	case XR_SESSION_STATE_STOPPING:     return "STOPPING";
	case XR_SESSION_STATE_LOSS_PENDING: return "LOSS_PENDING";
	case XR_SESSION_STATE_EXITING:      return "EXITING";
	default:                            return "UNKNOWN";
	}
}

static WGPUTextureFormat xrs_dxgi_to_wgpu( int64_t f ) {
	switch( f ) {
	case DXGI_FORMAT_B8G8R8A8_UNORM:      return WGPUTextureFormat_BGRA8Unorm;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return WGPUTextureFormat_BGRA8UnormSrgb;
	case DXGI_FORMAT_R8G8B8A8_UNORM:      return WGPUTextureFormat_RGBA8Unorm;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return WGPUTextureFormat_RGBA8UnormSrgb;
	default:                              return WGPUTextureFormat_Undefined;
	}
}

// ----------------------------------------------------------------------
// XRSession
// ----------------------------------------------------------------------

class XRSession : public node::ObjectWrap {
public:
	XrInstance     instance_ = XR_NULL_HANDLE;
	XrSystemId     systemId_ = XR_NULL_SYSTEM_ID;
	XrSession      session_  = XR_NULL_HANDLE;
	XrSpace        space_    = XR_NULL_HANDLE;
	XrSessionState state_    = XR_SESSION_STATE_UNKNOWN;
	bool           running_  = false;   // between xrBeginSession and xrEndSession
	bool           inFrame_  = false;   // between xrBeginFrame and xrEndFrame

	WGPUDevice device_ = NULL;          // borrowed; the app owns it

	struct Eye {
		XrSwapchain swapchain = XR_NULL_HANDLE;
		uint32_t    width = 0, height = 0;

		// The runtime's images can't be handed to Dawn directly: Dawn requires
		// D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS on anything imported as
		// SharedTextureMemory (it relies on state-decay-to-COMMON to know the
		// starting state), and OpenXR runtimes create plain render targets. So
		// we keep them as raw D3D12 and never show them to Dawn.
		std::vector<ComPtr<ID3D12Resource>> xrImages;

		// ...and render into an intermediate we create ourselves *with* that
		// flag, then copy across on the D3D12 queue in release(). One is enough:
		// the copy and the next frame's rendering are ordered by the queue.
		ComPtr<ID3D12Resource> interRes;
		WGPUSharedTextureMemory interMem = NULL;
		WGPUTexture             interTex = NULL;

		uint32_t index    = 0;   // runtime-chosen image index this frame
		bool     acquired = false;
	};
	std::vector<Eye> eyes_;
	int64_t          dxgiFormat_ = 0;

	// D3D12 objects for the intermediate->runtime copy. Dawn's own device and
	// queue, so the copy is ordered against Dawn's rendering with no fences
	// between us and it.
	ComPtr<ID3D12Device>       d3dDevice_;
	ComPtr<ID3D12CommandQueue> d3dQueue_;
	ComPtr<ID3D12Fence>        fence_;
	UINT64                     fenceValue_ = 0;
	HANDLE                     fenceEvent_ = NULL;

	// Small ring so a frame's copy doesn't have to complete before the next
	// frame can record one; only reuse of a slot waits.
	struct CopySlot {
		ComPtr<ID3D12CommandAllocator>    alloc;
		ComPtr<ID3D12GraphicsCommandList> list;
		UINT64 fenceValue = 0;
	};
	static const uint32_t kCopySlots = 3;
	CopySlot copyRing_[ kCopySlots ];
	uint32_t copySlot_ = 0;

	void blitToRuntime( Eye& e );

	XrFrameState                                   frameState_{};
	std::vector<XrView>                            views_;
	std::vector<XrCompositionLayerProjectionView>  projViews_;

	static Persistent<Function> constructor;

	XRSession() { frameState_.type = XR_TYPE_FRAME_STATE; }
	~XRSession() { teardown(); }

	void teardown() {
		// Let any in-flight copies finish before their resources go away.
		if( fence_ && fenceEvent_ && fenceValue_ &&
		    fence_->GetCompletedValue() < fenceValue_ ) {
			fence_->SetEventOnCompletion( fenceValue_, fenceEvent_ );
			WaitForSingleObject( fenceEvent_, 2000 );
		}
		for( auto& e : eyes_ ) {
			if( e.interTex ) { wgpuTextureRelease( e.interTex ); e.interTex = NULL; }
			if( e.interMem ) { wgpuSharedTextureMemoryRelease( e.interMem ); e.interMem = NULL; }
			e.interRes.Reset();
			e.xrImages.clear();
			if( e.swapchain != XR_NULL_HANDLE ) {
				xrDestroySwapchain( e.swapchain );
				e.swapchain = XR_NULL_HANDLE;
			}
		}
		eyes_.clear();
		for( auto& s : copyRing_ ) { s.list.Reset(); s.alloc.Reset(); s.fenceValue = 0; }
		if( fenceEvent_ ) { CloseHandle( fenceEvent_ ); fenceEvent_ = NULL; }
		fence_.Reset();
		d3dQueue_.Reset();
		d3dDevice_.Reset();
		if( space_    != XR_NULL_HANDLE ) { xrDestroySpace( space_ );     space_ = XR_NULL_HANDLE; }
		if( session_  != XR_NULL_HANDLE ) { xrDestroySession( session_ ); session_ = XR_NULL_HANDLE; }
		if( instance_ != XR_NULL_HANDLE ) { xrDestroyInstance( instance_ ); instance_ = XR_NULL_HANDLE; }
		if( device_ ) { wgpuDeviceRelease( device_ ); device_ = NULL; }
	}

	static void Init( Isolate* isolate, Local<Object> exports );
	static void New( const FunctionCallbackInfo<Value>& args );

	static void poll     ( const FunctionCallbackInfo<Value>& args );
	static void waitFrame( const FunctionCallbackInfo<Value>& args );
	static void acquire  ( const FunctionCallbackInfo<Value>& args );
	static void release  ( const FunctionCallbackInfo<Value>& args );
	static void endFrame ( const FunctionCallbackInfo<Value>& args );
	static void destroy  ( const FunctionCallbackInfo<Value>& args );
};

Persistent<Function> XRSession::constructor;

void XRSession::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( !args.IsConstructCall() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "XRSession is not constructible from JS; use sack.xr.beginSession()" ) ) );
		return;
	}
	XRSession* s = new XRSession();
	s->Wrap( args.This() );
	args.GetReturnValue().Set( args.This() );
}

// ---- poll: pump the session state machine ----------------------------
//
// OpenXR won't let you submit frames until the runtime has walked the session
// to READY and you've called xrBeginSession. JS calls this each tick; the
// returned string is the current state.
void XRSession::poll( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( XRSession );
	if( self->instance_ == XR_NULL_HANDLE ) return;

	for(;;) {
		XrEventDataBuffer ev = {};
		ev.type = XR_TYPE_EVENT_DATA_BUFFER;
		XrResult res = xrPollEvent( self->instance_, &ev );
		if( res == XR_EVENT_UNAVAILABLE ) break;
		if( XR_FAILED( res ) ) break;

		if( ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED ) {
			auto* e = reinterpret_cast<XrEventDataSessionStateChanged*>( &ev );
			self->state_ = e->state;

			if( e->state == XR_SESSION_STATE_READY && !self->running_ ) {
				XrSessionBeginInfo bi = {};
				bi.type = XR_TYPE_SESSION_BEGIN_INFO;
				bi.primaryViewConfigurationType =
					XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
				if( XR_SUCCEEDED( xrBeginSession( self->session_, &bi ) ) )
					self->running_ = true;
			} else if( e->state == XR_SESSION_STATE_STOPPING && self->running_ ) {
				xrEndSession( self->session_ );
				self->running_ = false;
			}
		}
		// Other events (instance loss, interaction profile changes) are not
		// interesting until input lands.
	}

	Local<Object> out = Object::New( isolate );
	SET_READONLY( out, "state",
		localStringExternal( isolate, xrs_state_name( self->state_ ) ) );
	SET_READONLY( out, "running", Boolean::New( isolate, self->running_ ) );
	args.GetReturnValue().Set( out );
}

// ---- waitFrame -------------------------------------------------------
//
// Blocks to the runtime's pacing. This is what replaces the setTimeout-based
// rAF: the runtime decides when the next frame starts, and predictedDisplayTime
// is what poses are predicted against.
void XRSession::waitFrame( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( XRSession );
	if( !self->running_ ) { args.GetReturnValue().SetNull(); return; }

	XrFrameWaitInfo wi = {};
	wi.type = XR_TYPE_FRAME_WAIT_INFO;
	self->frameState_ = {};
	self->frameState_.type = XR_TYPE_FRAME_STATE;
	XrResult res = xrWaitFrame( self->session_, &wi, &self->frameState_ );
	if( XR_FAILED( res ) ) { args.GetReturnValue().SetNull(); return; }

	XrFrameBeginInfo bi = {};
	bi.type = XR_TYPE_FRAME_BEGIN_INFO;
	if( XR_FAILED( xrBeginFrame( self->session_, &bi ) ) ) {
		args.GetReturnValue().SetNull();
		return;
	}
	self->inFrame_ = true;

	// Locate the eyes for this frame's predicted display time.
	uint32_t viewCount = 0;
	XrViewLocateInfo li = {};
	li.type = XR_TYPE_VIEW_LOCATE_INFO;
	li.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	li.displayTime = self->frameState_.predictedDisplayTime;
	li.space = self->space_;

	XrViewState vs = {};
	vs.type = XR_TYPE_VIEW_STATE;
	for( auto& v : self->views_ ) { v = {}; v.type = XR_TYPE_VIEW; }
	xrLocateViews( self->session_, &li, &vs, (uint32_t)self->views_.size(),
	               &viewCount, self->views_.data() );

	const bool posesValid =
		( vs.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT ) &&
		( vs.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT );

	Local<Object> out = Object::New( isolate );
	SET_READONLY( out, "shouldRender",
		Boolean::New( isolate, self->frameState_.shouldRender != XR_FALSE ) );
	SET_READONLY( out, "displayTime",
		Number::New( isolate, (double)self->frameState_.predictedDisplayTime ) );
	SET_READONLY( out, "posesValid", Boolean::New( isolate, posesValid ) );

	Local<Array> views = Array::New( isolate, (int)viewCount );
	for( uint32_t i = 0; i < viewCount; i++ ) {
		const XrView& v = self->views_[ i ];
		Local<Object> jv = Object::New( isolate );

		Local<Object> pos = Object::New( isolate );
		SET_READONLY( pos, "x", Number::New( isolate, v.pose.position.x ) );
		SET_READONLY( pos, "y", Number::New( isolate, v.pose.position.y ) );
		SET_READONLY( pos, "z", Number::New( isolate, v.pose.position.z ) );
		SET_READONLY( jv, "position", pos );

		Local<Object> ori = Object::New( isolate );
		SET_READONLY( ori, "x", Number::New( isolate, v.pose.orientation.x ) );
		SET_READONLY( ori, "y", Number::New( isolate, v.pose.orientation.y ) );
		SET_READONLY( ori, "z", Number::New( isolate, v.pose.orientation.z ) );
		SET_READONLY( ori, "w", Number::New( isolate, v.pose.orientation.w ) );
		SET_READONLY( jv, "orientation", ori );

		// Angles are half-angles in radians and asymmetric — left/right and
		// up/down differ. Build the projection from these directly; a single
		// symmetric "fov" would be wrong on every headset.
		Local<Object> fov = Object::New( isolate );
		SET_READONLY( fov, "left",  Number::New( isolate, v.fov.angleLeft ) );
		SET_READONLY( fov, "right", Number::New( isolate, v.fov.angleRight ) );
		SET_READONLY( fov, "up",    Number::New( isolate, v.fov.angleUp ) );
		SET_READONLY( fov, "down",  Number::New( isolate, v.fov.angleDown ) );
		SET_READONLY( jv, "fov", fov );

		views->Set( context, i, jv ).FromMaybe( false );
	}
	SET_READONLY( out, "views", views );

	args.GetReturnValue().Set( out );
}

// ---- acquire ---------------------------------------------------------
void XRSession::acquire( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( XRSession );
	uint32_t eye = args.Length() > 0
		? args[ 0 ]->Uint32Value( context ).FromMaybe( 0 ) : 0;
	if( eye >= self->eyes_.size() ) {
		isolate->ThrowException( Exception::RangeError( localStringExternal(
			isolate, "acquire: eye index out of range" ) ) );
		return;
	}
	XRSession::Eye& e = self->eyes_[ eye ];
	if( e.acquired ) {
		isolate->ThrowException( Exception::Error( localStringExternal(
			isolate, "acquire: this eye is already acquired this frame" ) ) );
		return;
	}

	XrSwapchainImageAcquireInfo ai = {};
	ai.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
	XrResult res = xrAcquireSwapchainImage( e.swapchain, &ai, &e.index );
	if( XR_FAILED( res ) ) {
		isolate->ThrowException( Exception::Error( localStringExternal( isolate,
			( "xrAcquireSwapchainImage: " + xrs_result( self->instance_, res ) ).c_str() ) ) );
		return;
	}

	XrSwapchainImageWaitInfo wi = {};
	wi.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
	wi.timeout = XR_INFINITE_DURATION;
	res = xrWaitSwapchainImage( e.swapchain, &wi );
	if( XR_FAILED( res ) ) {
		isolate->ThrowException( Exception::Error( localStringExternal( isolate,
			( "xrWaitSwapchainImage: " + xrs_result( self->instance_, res ) ).c_str() ) ) );
		return;
	}

	// Open our intermediate for rendering. Not marked isSwapchain: this texture
	// is ours, not a presentation surface, and Dawn's swapchain handling would
	// add present bookkeeping we don't want. The runtime's image is copied into
	// separately in release().
	WGPUSharedTextureMemoryBeginAccessDescriptor bad =
		WGPU_SHARED_TEXTURE_MEMORY_BEGIN_ACCESS_DESCRIPTOR_INIT;
	bad.initialized = WGPU_TRUE;

	if( wgpuSharedTextureMemoryBeginAccess( e.interMem, e.interTex, &bad )
	    != WGPUStatus_Success ) {
		isolate->ThrowException( Exception::Error( localStringExternal(
			isolate, "SharedTextureMemoryBeginAccess failed" ) ) );
		return;
	}

	e.acquired = true;
	wgpuTextureAddRef( e.interTex );
	WGPU_RETURN_NEW( GPUTexture, e.interTex );
}

// ---- release ---------------------------------------------------------
void XRSession::release( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( XRSession );
	uint32_t eye = args.Length() > 0
		? args[ 0 ]->Uint32Value( context ).FromMaybe( 0 ) : 0;
	if( eye >= self->eyes_.size() ) return;
	XRSession::Eye& e = self->eyes_[ eye ];
	if( !e.acquired ) return;

	// EndAccess also forces Dawn to flush the work that used this texture, which
	// is what lets the copy below rely on plain queue ordering instead of
	// importing Dawn's fences.
	WGPUSharedTextureMemoryEndAccessState end =
		WGPU_SHARED_TEXTURE_MEMORY_END_ACCESS_STATE_INIT;
	wgpuSharedTextureMemoryEndAccess( e.interMem, e.interTex, &end );
	wgpuSharedTextureMemoryEndAccessStateFreeMembers( end );

	self->blitToRuntime( e );

	XrSwapchainImageReleaseInfo ri = {};
	ri.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
	xrReleaseSwapchainImage( e.swapchain, &ri );
	e.acquired = false;
}

// Copy our intermediate into the runtime's swapchain image.
//
// Ordering is by queue, not by fence: this runs on the same ID3D12CommandQueue
// Dawn renders with, and EndAccess above has already flushed Dawn's work onto
// it, so the copy cannot start early.
void XRSession::blitToRuntime( Eye& e ) {
	if( !d3dQueue_ || e.index >= e.xrImages.size() ) return;
	ID3D12Resource* dst = e.xrImages[ e.index ].Get();
	ID3D12Resource* src = e.interRes.Get();
	if( !dst || !src ) return;

	CopySlot& slot = copyRing_[ copySlot_ ];

	// Only stall if this slot's previous copy is still running.
	if( slot.fenceValue && fence_->GetCompletedValue() < slot.fenceValue ) {
		fence_->SetEventOnCompletion( slot.fenceValue, fenceEvent_ );
		WaitForSingleObject( fenceEvent_, INFINITE );
	}
	slot.alloc->Reset();
	slot.list->Reset( slot.alloc.Get(), NULL );

	// Our intermediate has ALLOW_SIMULTANEOUS_ACCESS, so after Dawn's work it
	// has decayed to COMMON. The runtime hands its image over in RENDER_TARGET
	// and requires it back in RENDER_TARGET, so both transitions are paired.
	D3D12_RESOURCE_BARRIER in[ 2 ] = {};
	in[ 0 ].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	in[ 0 ].Transition.pResource   = src;
	in[ 0 ].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	in[ 0 ].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	in[ 0 ].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
	in[ 1 ].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	in[ 1 ].Transition.pResource   = dst;
	in[ 1 ].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	in[ 1 ].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	in[ 1 ].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
	slot.list->ResourceBarrier( 2, in );

	slot.list->CopyResource( dst, src );

	D3D12_RESOURCE_BARRIER out[ 2 ] = { in[ 0 ], in[ 1 ] };
	out[ 0 ].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	out[ 0 ].Transition.StateAfter  = D3D12_RESOURCE_STATE_COMMON;
	out[ 1 ].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	out[ 1 ].Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	slot.list->ResourceBarrier( 2, out );

	slot.list->Close();
	ID3D12CommandList* lists[] = { slot.list.Get() };
	d3dQueue_->ExecuteCommandLists( 1, lists );

	slot.fenceValue = ++fenceValue_;
	d3dQueue_->Signal( fence_.Get(), slot.fenceValue );
	copySlot_ = ( copySlot_ + 1 ) % kCopySlots;
}

// ---- endFrame --------------------------------------------------------
void XRSession::endFrame( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( XRSession );
	if( !self->inFrame_ ) return;

	XrCompositionLayerProjection layer = {};
	layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
	layer.space = self->space_;
	layer.layerFlags = 0;

	for( size_t i = 0; i < self->eyes_.size() && i < self->views_.size(); i++ ) {
		XrCompositionLayerProjectionView& pv = self->projViews_[ i ];
		pv = {};
		pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
		pv.pose = self->views_[ i ].pose;
		pv.fov  = self->views_[ i ].fov;
		pv.subImage.swapchain = self->eyes_[ i ].swapchain;
		pv.subImage.imageRect.offset = { 0, 0 };
		pv.subImage.imageRect.extent = { (int32_t)self->eyes_[ i ].width,
		                                 (int32_t)self->eyes_[ i ].height };
		pv.subImage.imageArrayIndex = 0;
	}
	layer.viewCount = (uint32_t)self->projViews_.size();
	layer.views = self->projViews_.data();

	// Submit no layers at all when the runtime says not to render (headset off
	// the head, app not visible) — passing a layer then is a protocol error.
	const XrCompositionLayerBaseHeader* layers[] = {
		reinterpret_cast<const XrCompositionLayerBaseHeader*>( &layer ) };

	XrFrameEndInfo ei = {};
	ei.type = XR_TYPE_FRAME_END_INFO;
	ei.displayTime = self->frameState_.predictedDisplayTime;
	ei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	ei.layerCount = self->frameState_.shouldRender ? 1 : 0;
	ei.layers     = self->frameState_.shouldRender ? layers : NULL;

	XrResult res = xrEndFrame( self->session_, &ei );
	self->inFrame_ = false;
	if( XR_FAILED( res ) )
		lprintf( "xrEndFrame: %s", xrs_result( self->instance_, res ).c_str() );
}

void XRSession::destroy( const FunctionCallbackInfo<Value>& args ) {
	WGPU_THIS( XRSession );
	self->teardown();
}

void XRSession::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, XRSession::New );
	tpl->SetClassName( localStringExternal( isolate, "XRSession" ) );
	tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

	NODE_SET_PROTOTYPE_METHOD( tpl, "poll",      XRSession::poll );
	NODE_SET_PROTOTYPE_METHOD( tpl, "waitFrame", XRSession::waitFrame );
	NODE_SET_PROTOTYPE_METHOD( tpl, "acquire",   XRSession::acquire );
	NODE_SET_PROTOTYPE_METHOD( tpl, "release",   XRSession::release );
	NODE_SET_PROTOTYPE_METHOD( tpl, "endFrame",  XRSession::endFrame );
	NODE_SET_PROTOTYPE_METHOD( tpl, "destroy",   XRSession::destroy );

	XRSession::constructor.Reset( isolate,
		tpl->GetFunction( context ).ToLocalChecked() );
}

// ----------------------------------------------------------------------
// beginSession
// ----------------------------------------------------------------------

#define XR_FAIL( stage, res ) do {                                            \
		std::string _m = std::string( stage ) + ": "                  \
			+ xrs_result( s->instance_, (res) );                  \
		s->teardown();                                                \
		isolate->ThrowException( Exception::Error(                    \
			localStringExternal( isolate, _m.c_str() ) ) );       \
		return;                                                       \
	} while( 0 )

void XR_beginSession( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	if( args.Length() < 1 || !args[ 0 ]->IsObject() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "beginSession(device): expected a GPUDevice" ) ) );
		return;
	}
	GPUDevice* devWrap = node::ObjectWrap::Unwrap<GPUDevice>( args[ 0 ].As<Object>() );
	if( !devWrap || !devWrap->handle_ ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, "beginSession: device is not a live GPUDevice" ) ) );
		return;
	}

	Local<Object> obj = XRSession::constructor.Get( isolate )
		->NewInstance( context, 0, NULL ).ToLocalChecked();
	XRSession* s = node::ObjectWrap::Unwrap<XRSession>( obj );

	s->device_ = devWrap->handle_;
	wgpuDeviceAddRef( s->device_ );

	// ---- instance with the D3D12 binding -----------------------------
	const char* exts[] = { XR_KHR_D3D12_ENABLE_EXTENSION_NAME };
	XrInstanceCreateInfo ici = {};
	ici.type = XR_TYPE_INSTANCE_CREATE_INFO;
	ici.applicationInfo.apiVersion = XR_API_VERSION_1_0;
	snprintf( ici.applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE, "sack-gui" );
	snprintf( ici.applicationInfo.engineName, XR_MAX_ENGINE_NAME_SIZE, "sack" );
	ici.applicationInfo.applicationVersion = 1;
	ici.applicationInfo.engineVersion = 1;
	ici.enabledExtensionCount = 1;
	ici.enabledExtensionNames = exts;

	XrResult res = xrCreateInstance( &ici, &s->instance_ );
	if( XR_FAILED( res ) ) XR_FAIL( "xrCreateInstance", res );

	XrSystemGetInfo sgi = {};
	sgi.type = XR_TYPE_SYSTEM_GET_INFO;
	sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	res = xrGetSystem( s->instance_, &sgi, &s->systemId_ );
	if( XR_FAILED( res ) ) XR_FAIL( "xrGetSystem", res );

	// ---- graphics requirements ---------------------------------------
	// Extension entry points are never exported by the loader; they must be
	// resolved through xrGetInstanceProcAddr. And the spec requires this call
	// before xrCreateSession even when its answer is ignored.
	PFN_xrGetD3D12GraphicsRequirementsKHR getReq = NULL;
	res = xrGetInstanceProcAddr( s->instance_, "xrGetD3D12GraphicsRequirementsKHR",
		reinterpret_cast<PFN_xrVoidFunction*>( &getReq ) );
	if( XR_FAILED( res ) || !getReq )
		XR_FAIL( "xrGetInstanceProcAddr(xrGetD3D12GraphicsRequirementsKHR)", res );

	XrGraphicsRequirementsD3D12KHR gr = {};
	gr.type = XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR;
	res = getReq( s->instance_, s->systemId_, &gr );
	if( XR_FAILED( res ) ) XR_FAIL( "xrGetD3D12GraphicsRequirementsKHR", res );

	// ---- Dawn's device + queue ---------------------------------------
	ComPtr<ID3D12Device>       d3dDevice = dawn::native::d3d12::GetD3D12Device( s->device_ );
	ComPtr<ID3D12CommandQueue> d3dQueue  = dawn::native::d3d12::GetD3D12CommandQueue( s->device_ );
	if( !d3dDevice || !d3dQueue ) {
		s->teardown();
		isolate->ThrowException( Exception::Error( localStringExternal( isolate,
			"beginSession: device is not a D3D12 Dawn device "
			"(request an adapter with backendType 'd3d12')" ) ) );
		return;
	}

	// The runtime dictates which adapter it can present from. Mismatch here is
	// not recoverable by us — the app would have to create its device on the
	// right adapter via RequestAdapterOptionsLUID — so say so plainly rather
	// than failing later inside xrCreateSession.
	LUID want = gr.adapterLuid;
	LUID have = d3dDevice->GetAdapterLuid();
	if( want.LowPart != have.LowPart || want.HighPart != have.HighPart ) {
		s->teardown();
		isolate->ThrowException( Exception::Error( localStringExternal( isolate,
			"beginSession: the Dawn device is on a different adapter than the XR "
			"runtime requires; create the device on the runtime's adapter (LUID)" ) ) );
		return;
	}

	s->d3dDevice_ = d3dDevice;
	s->d3dQueue_  = d3dQueue;

	// Fence + command-list ring for the intermediate->runtime copies.
	if( FAILED( d3dDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE,
	            IID_PPV_ARGS( &s->fence_ ) ) ) ) {
		s->teardown();
		isolate->ThrowException( Exception::Error( localStringExternal(
			isolate, "CreateFence failed" ) ) );
		return;
	}
	s->fenceEvent_ = CreateEventW( NULL, FALSE, FALSE, NULL );
	for( uint32_t i = 0; i < XRSession::kCopySlots; i++ ) {
		if( FAILED( d3dDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT,
		            IID_PPV_ARGS( &s->copyRing_[ i ].alloc ) ) )
		 || FAILED( d3dDevice->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		            s->copyRing_[ i ].alloc.Get(), NULL,
		            IID_PPV_ARGS( &s->copyRing_[ i ].list ) ) ) ) {
			s->teardown();
			isolate->ThrowException( Exception::Error( localStringExternal(
				isolate, "creating the copy command list failed" ) ) );
			return;
		}
		// Created open; close so the first Reset in blitToRuntime is legal.
		s->copyRing_[ i ].list->Close();
	}

	XrGraphicsBindingD3D12KHR binding = {};
	binding.type = XR_TYPE_GRAPHICS_BINDING_D3D12_KHR;
	binding.device = d3dDevice.Get();
	binding.queue  = d3dQueue.Get();

	XrSessionCreateInfo sci = {};
	sci.type = XR_TYPE_SESSION_CREATE_INFO;
	sci.next = &binding;
	sci.systemId = s->systemId_;
	res = xrCreateSession( s->instance_, &sci, &s->session_ );
	if( XR_FAILED( res ) ) XR_FAIL( "xrCreateSession", res );

	XrReferenceSpaceCreateInfo rsci = {};
	rsci.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
	rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	rsci.poseInReferenceSpace.orientation.w = 1.0f;
	res = xrCreateReferenceSpace( s->session_, &rsci, &s->space_ );
	if( XR_FAILED( res ) ) XR_FAIL( "xrCreateReferenceSpace", res );

	// ---- view configuration ------------------------------------------
	uint32_t viewCount = 0;
	res = xrEnumerateViewConfigurationViews( s->instance_, s->systemId_,
		XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, NULL );
	if( XR_FAILED( res ) || viewCount == 0 )
		XR_FAIL( "xrEnumerateViewConfigurationViews", res );

	std::vector<XrViewConfigurationView> cfgViews( viewCount );
	for( auto& v : cfgViews ) { v = {}; v.type = XR_TYPE_VIEW_CONFIGURATION_VIEW; }
	res = xrEnumerateViewConfigurationViews( s->instance_, s->systemId_,
		XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount,
		cfgViews.data() );
	if( XR_FAILED( res ) ) XR_FAIL( "xrEnumerateViewConfigurationViews(fill)", res );

	s->views_.resize( viewCount );
	s->projViews_.resize( viewCount );

	// ---- swapchain format --------------------------------------------
	uint32_t fmtCount = 0;
	xrEnumerateSwapchainFormats( s->session_, 0, &fmtCount, NULL );
	std::vector<int64_t> formats( fmtCount );
	if( fmtCount )
		xrEnumerateSwapchainFormats( s->session_, fmtCount, &fmtCount, formats.data() );

	// Prefer the non-sRGB BGRA the imglib-webgpu HUD pipelines are already baked
	// to (see webgpu_image_set_surface_format, which only takes effect before
	// build_all). If the runtime only offers sRGB variants, the HUD will need a
	// second pipeline set — that's a known follow-up, not a silent mismatch.
	const int64_t preferred[] = {
		DXGI_FORMAT_B8G8R8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	};
	for( int64_t p : preferred ) {
		for( int64_t f : formats ) if( f == p ) { s->dxgiFormat_ = p; break; }
		if( s->dxgiFormat_ ) break;
	}
	if( !s->dxgiFormat_ && fmtCount ) s->dxgiFormat_ = formats[ 0 ];

	WGPUTextureFormat wgpuFormat = xrs_dxgi_to_wgpu( s->dxgiFormat_ );
	if( wgpuFormat == WGPUTextureFormat_Undefined ) {
		s->teardown();
		isolate->ThrowException( Exception::Error( localStringExternal( isolate,
			"beginSession: runtime offered no swapchain format this binding maps" ) ) );
		return;
	}

	// ---- per-eye swapchains ------------------------------------------
	// Two separate swapchains rather than one texture-array: the imglib-webgpu
	// bundle path assumes a plain single-layer 2D target, and the active-surface
	// tracking in webgpu_module.cc is single-target global state. Multiview would
	// need both reworked.
	//
	// The C++ native API appears here because the D3D12 resource import
	// descriptor has no C equivalent. Balance the addref so the temporary C++
	// Device wrapper doesn't release a device we only borrowed.
	wgpuDeviceAddRef( s->device_ );
	wgpu::Device cppDevice = wgpu::Device::Acquire( s->device_ );

	s->eyes_.resize( viewCount );
	for( uint32_t i = 0; i < viewCount; i++ ) {
		XRSession::Eye& e = s->eyes_[ i ];
		e.width  = cfgViews[ i ].recommendedImageRectWidth;
		e.height = cfgViews[ i ].recommendedImageRectHeight;

		XrSwapchainCreateInfo sc = {};
		sc.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
		sc.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
		                XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
		sc.format = s->dxgiFormat_;
		sc.sampleCount = 1;
		sc.width  = e.width;
		sc.height = e.height;
		sc.faceCount  = 1;
		sc.arraySize  = 1;
		sc.mipCount   = 1;
		res = xrCreateSwapchain( s->session_, &sc, &e.swapchain );
		if( XR_FAILED( res ) ) XR_FAIL( "xrCreateSwapchain", res );

		uint32_t imgCount = 0;
		xrEnumerateSwapchainImages( e.swapchain, 0, &imgCount, NULL );
		std::vector<XrSwapchainImageD3D12KHR> images( imgCount );
		for( auto& im : images ) { im = {}; im.type = XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR; }
		res = xrEnumerateSwapchainImages( e.swapchain, imgCount, &imgCount,
			reinterpret_cast<XrSwapchainImageBaseHeader*>( images.data() ) );
		if( XR_FAILED( res ) ) XR_FAIL( "xrEnumerateSwapchainImages", res );

		// Runtime images stay raw D3D12 — Dawn will not import them (no
		// ALLOW_SIMULTANEOUS_ACCESS). They're only ever a CopyResource target.
		e.xrImages.resize( imgCount );
		for( uint32_t k = 0; k < imgCount; k++ )
			e.xrImages[ k ] = images[ k ].texture;

		// Our intermediate, created with the flag Dawn requires so it can be
		// imported and rendered into like any other texture.
		D3D12_HEAP_PROPERTIES heap = {};
		heap.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC rd = {};
		rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		rd.Width  = e.width;
		rd.Height = e.height;
		rd.DepthOrArraySize = 1;
		rd.MipLevels = 1;
		rd.Format = (DXGI_FORMAT)s->dxgiFormat_;
		rd.SampleDesc.Count = 1;
		rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
		           D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;

		HRESULT hr = s->d3dDevice_->CreateCommittedResource(
			&heap, D3D12_HEAP_FLAG_NONE, &rd,
			D3D12_RESOURCE_STATE_COMMON, NULL,
			IID_PPV_ARGS( &e.interRes ) );
		if( FAILED( hr ) ) {
			s->teardown();
			isolate->ThrowException( Exception::Error( localStringExternal( isolate,
				"CreateCommittedResource for the XR intermediate failed" ) ) );
			return;
		}

		dawn::native::d3d12::SharedTextureMemoryD3D12ResourceDescriptor srd;
		srd.resource = e.interRes;

		wgpu::SharedTextureMemoryDescriptor md = {};
		md.nextInChain = &srd;

		wgpu::SharedTextureMemory mem = cppDevice.ImportSharedTextureMemory( &md );
		if( !mem ) {
			s->teardown();
			isolate->ThrowException( Exception::Error( localStringExternal( isolate,
				"ImportSharedTextureMemory failed — the device most likely lacks "
				"'shared-texture-memory-d3d12-resource' (needs dawnToggles "
				"allow_unsafe_apis at requestDevice)" ) ) );
			return;
		}

		wgpu::TextureDescriptor td = {};
		td.dimension = wgpu::TextureDimension::e2D;
		td.size = { e.width, e.height, 1 };
		td.format = static_cast<wgpu::TextureFormat>( wgpuFormat );
		td.mipLevelCount = 1;
		td.sampleCount = 1;
		td.usage = wgpu::TextureUsage::RenderAttachment |
		           wgpu::TextureUsage::TextureBinding |
		           wgpu::TextureUsage::CopySrc;

		wgpu::Texture tex = mem.CreateTexture( &td );
		e.interMem = mem.MoveToCHandle();
		e.interTex = tex.MoveToCHandle();
	}

	// Hand the borrowed handle back out of the C++ wrapper.
	cppDevice.MoveToCHandle();

	// ---- report ------------------------------------------------------
	Local<Array> eyes = Array::New( isolate, (int)viewCount );
	for( uint32_t i = 0; i < viewCount; i++ ) {
		Local<Object> je = Object::New( isolate );
		SET_READONLY( je, "width",  Integer::NewFromUnsigned( isolate, s->eyes_[ i ].width ) );
		SET_READONLY( je, "height", Integer::NewFromUnsigned( isolate, s->eyes_[ i ].height ) );
		SET_READONLY( je, "images",
			Integer::NewFromUnsigned( isolate, (uint32_t)s->eyes_[ i ].xrImages.size() ) );
		eyes->Set( context, i, je ).FromMaybe( false );
	}
	SET_READONLY( obj, "eyes", eyes );
	SET_READONLY( obj, "dxgiFormat", Number::New( isolate, (double)s->dxgiFormat_ ) );

	args.GetReturnValue().Set( obj );
}

#undef XR_FAIL

void InitXRSession( Isolate* isolate, Local<Object> exports ) {
	XRSession::Init( isolate, exports );
}
