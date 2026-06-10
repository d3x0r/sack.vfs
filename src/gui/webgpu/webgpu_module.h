// WebGPU module — entry point + per-class declarations.
//
// Call InitWebGPU(isolate, exports) from the main module dispatcher to expose
// `sack.gpu` (a singleton GPU instance) and the GPUAdapter class.
//
// More classes (GPUDevice, GPUQueue, GPUBuffer, ...) will be added here and
// in webgpu_module.cc as the binding fills out.

#pragma once

#include "../../global.h"
#include "webgpu/webgpu.h"

// Main entry — register WebGPU surface on the given exports object.
// Creates one WGPUInstance for this isolate and exposes it as `exports.gpu`.
extern void InitWebGPU( v8::Isolate* isolate, v8::Local<v8::Object> exports );


// ---------- GPU (= navigator.gpu) ----------

class GPU : public node::ObjectWrap {
public:
	WGPUInstance handle_;  // owned: destroyed in dtor

	GPU();
	~GPU();

	static void Init( v8::Isolate* isolate, v8::Local<v8::Object> exports );

	// JS-side constructor — only used internally; throws if called from JS.
	static void New( const v8::FunctionCallbackInfo<v8::Value>& args );

	// async requestAdapter(options?) → Promise<GPUAdapter | null>
	static void requestAdapter( const v8::FunctionCallbackInfo<v8::Value>& args );

	// Dawn callback — fires synchronously inside wgpuInstanceProcessEvents.
	static void OnAdapterReady( WGPURequestAdapterStatus status,
		WGPUAdapter adapter, WGPUStringView message,
		void* userdata1, void* userdata2 );
};


// ---------- GPUAdapter ----------

class GPUAdapter : public node::ObjectWrap {
public:
	WGPUAdapter handle_;  // released in dtor via wgpuAdapterRelease

	GPUAdapter();
	~GPUAdapter();

	static void Init( v8::Isolate* isolate, v8::Local<v8::Object> exports );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& args );

	// readonly attribute GPUAdapterInfo info;
	static void infoGetter( v8::Local<v8::Name> property,
		const v8::PropertyCallbackInfo<v8::Value>& info );

	// Promise<GPUDevice> requestDevice(optional GPUDeviceDescriptor descriptor);
	static void requestDevice( const v8::FunctionCallbackInfo<v8::Value>& args );

	// Dawn callback for requestDevice (fires inside processEvents).
	static void OnDeviceReady( WGPURequestDeviceStatus status,
		WGPUDevice device, WGPUStringView message,
		void* userdata1, void* userdata2 );
};


// ---------- GPUDevice ----------
//
// WGPU_HAVE_GPUDevice enables the generator-produced method block:
//   destroy, createBuffer, createSampler, createCommandEncoder, createQuerySet,
//   pushErrorScope.
// Hand-written next: queue getter, lost promise, popErrorScope (async),
// the dict-args-with-nested-dicts createX methods.

#define WGPU_HAVE_GPUDevice

struct GPUDeviceLostState;  // defined in webgpu_module.cc

class GPUDevice : public node::ObjectWrap {
public:
	WGPUDevice handle_;  // released in dtor via wgpuDeviceRelease
	WGPUAdapter adapter_; // addref'd in OnDeviceReady; released in dtor.
	                     // Needed for wgpuSurfaceGetCapabilities() to drive
	                     // canvas configure fallbacks.
	GPUDeviceLostState* lostState_;  // owned; freed in dtor

	GPUDevice();
	~GPUDevice();

	static void Init( v8::Isolate* isolate, v8::Local<v8::Object> exports );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& args );

	// Methods to come: createBuffer, createTexture, createShaderModule, ...
};


// ---------- GPUBuffer ----------
//
// WGPU_HAVE_GPUBuffer enables the generator-produced method block in
// webgpu_generated.cc (destroy, unmap). createBuffer / mapAsync /
// getMappedRange are still hand-written and TBD.

#define WGPU_HAVE_GPUBuffer

class GPUBuffer : public node::ObjectWrap {
public:
	WGPUBuffer handle_;

	GPUBuffer();
	~GPUBuffer();

	static void Init( v8::Isolate* isolate, v8::Local<v8::Object> exports );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& args );
};


// ---------- Simple handle-wrapper classes ----------
//
// All follow the same shape: hold a WGPU* handle, release it in the dtor,
// reject JS-side construction, run Init from InitWebGPU. The WGPU_HAVE_*
// flag below each lights up the matching generated method block (and
// allows other classes' generated methods to refer to it as a handle type).
//
// Classes here that have GEN methods get wireGenerated_<X> called from Init;
// classes without (currently GPUSampler, GPUTextureView, GPUCommandBuffer)
// just declare the slot so other generated bodies can wrap/return them.

#define WGPU_HAVE_GPUSampler
class GPUSampler : public node::ObjectWrap {
public:
	WGPUSampler handle_;
	GPUSampler();  ~GPUSampler();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUTexture
class GPUTexture : public node::ObjectWrap {
public:
	WGPUTexture handle_;
	GPUTexture();  ~GPUTexture();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUTextureView
class GPUTextureView : public node::ObjectWrap {
public:
	WGPUTextureView handle_;
	// Source texture this view was created from (borrowed — Dawn keeps the
	// texture alive while the view exists). Used by the auto-present path to
	// recognise color attachments that target the canvas's swap-chain image.
	WGPUTexture     sourceTexture_ = NULL;
	GPUTextureView();  ~GPUTextureView();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUQuerySet
class GPUQuerySet : public node::ObjectWrap {
public:
	WGPUQuerySet handle_;
	GPUQuerySet();  ~GPUQuerySet();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUCommandBuffer
class GPUCommandBuffer : public node::ObjectWrap {
public:
	WGPUCommandBuffer handle_;
	GPUCommandBuffer();  ~GPUCommandBuffer();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUCommandEncoder
class GPUCommandEncoder : public node::ObjectWrap {
public:
	WGPUCommandEncoder handle_;
	GPUCommandEncoder();  ~GPUCommandEncoder();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUComputePassEncoder
class GPUComputePassEncoder : public node::ObjectWrap {
public:
	WGPUComputePassEncoder handle_;
	GPUComputePassEncoder();  ~GPUComputePassEncoder();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPURenderPassEncoder
class GPURenderPassEncoder : public node::ObjectWrap {
public:
	WGPURenderPassEncoder handle_;
	// Tag of the parent GPUCommandEncoder (opaque — only used for equality
	// against the auto-present tracking). Set by beginRenderPass.
	void*                 commandEncoderTag_ = NULL;
	GPURenderPassEncoder();  ~GPURenderPassEncoder();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPURenderBundleEncoder
class GPURenderBundleEncoder : public node::ObjectWrap {
public:
	WGPURenderBundleEncoder handle_;
	GPURenderBundleEncoder();  ~GPURenderBundleEncoder();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUBindGroupLayout
class GPUBindGroupLayout : public node::ObjectWrap {
public:
	WGPUBindGroupLayout handle_;
	GPUBindGroupLayout();  ~GPUBindGroupLayout();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPURenderPipeline
class GPURenderPipeline : public node::ObjectWrap {
public:
	WGPURenderPipeline handle_;
	GPURenderPipeline();  ~GPURenderPipeline();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUComputePipeline
class GPUComputePipeline : public node::ObjectWrap {
public:
	WGPUComputePipeline handle_;
	GPUComputePipeline();  ~GPUComputePipeline();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUQueue
class GPUQueue : public node::ObjectWrap {
public:
	WGPUQueue handle_;
	GPUQueue();  ~GPUQueue();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPURenderBundle
class GPURenderBundle : public node::ObjectWrap {
public:
	WGPURenderBundle handle_;
	GPURenderBundle();  ~GPURenderBundle();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUBindGroup
class GPUBindGroup : public node::ObjectWrap {
public:
	WGPUBindGroup handle_;
	GPUBindGroup();  ~GPUBindGroup();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUPipelineLayout
class GPUPipelineLayout : public node::ObjectWrap {
public:
	WGPUPipelineLayout handle_;
	GPUPipelineLayout();  ~GPUPipelineLayout();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};

#define WGPU_HAVE_GPUShaderModule
class GPUShaderModule : public node::ObjectWrap {
public:
	WGPUShaderModule handle_;
	GPUShaderModule();  ~GPUShaderModule();
	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New( const v8::FunctionCallbackInfo<v8::Value>& );
};


// GPUCanvasContext lives in canvas_context.h/.cc — declared there so the
// implementation file can stay small and the rest of the webgpu module
// doesn't have to recompile when we iterate on the surface plumbing.

// Returns the per-isolate WGPUInstance, or NULL if InitWebGPU hasn't run yet.
// Used by canvas_context.cc when minting a surface for a RenderObject.
extern WGPUInstance webgpu_get_instance( void );

// ---------- Active-device tracking (for native sack-side consumers) ----------
//
// "Active" = the most recently created WGPUDevice via OnDeviceReady. Tracked
// so native code that doesn't see the JS-side GPUDevice handle directly
// (e.g. the imglib-webgpu driver) can pick up a device to render against.
//
// Both return NULL until JS calls sack.gpu.requestAdapter().requestDevice().
// The device is held by-ref while it's the active one — i.e. an extra
// wgpuDeviceAddRef is taken at OnDeviceReady time and released when the
// GPUDevice wrapper is destroyed (or another device displaces it).
//
// extern "C" so the symbol name is stable for non-C++ TUs that may link
// against the binding (and so the matching extern "C" decl in
// imglib-webgpu/driver.cc resolves at link time).
extern "C" WGPUDevice webgpu_get_active_device( void );
extern "C" WGPUQueue  webgpu_get_active_queue( void );
