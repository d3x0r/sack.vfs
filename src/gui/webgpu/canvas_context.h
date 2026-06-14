// GPUCanvasContext — wraps a Dawn WGPUSurface for a vidlib RenderObject.
//
// JS-side: `renderer.getContext('webgpu')` calls webgpu_canvas_context_for_renderer
// in this file and returns the resulting GPUCanvasContext. The wrapper exposes
// configure / unconfigure / getCurrentTexture per WebGPU spec.

#pragma once

#include "../../global.h"
#include "webgpu/webgpu.h"

class GPUCanvasContext : public node::ObjectWrap {
public:
	WGPUSurface surface_;     // owned: released in dtor
	PRENDERER   renderer_;    // borrowed back-pointer for size queries
	bool        configured_;  // tracks wgpuSurfaceConfigure state
	WGPUDevice  configuredDevice_;  // last device passed to configure;
	                          // used to detect device switches, which
	                          // require recreating the underlying surface
	                          // on Vulkan (the swapchain can't switch).

	GPUCanvasContext();
	~GPUCanvasContext();

	static void Init( v8::Isolate*, v8::Local<v8::Object> );
	static void New ( const v8::FunctionCallbackInfo<v8::Value>& );

	// configure({ device, format, usage?, alphaMode?, viewFormats?,
	//             width?, height? })
	static void configure        ( const v8::FunctionCallbackInfo<v8::Value>& );
	static void unconfigure      ( const v8::FunctionCallbackInfo<v8::Value>& );
	static void getCurrentTexture( const v8::FunctionCallbackInfo<v8::Value>& );

	// Tell the imglib-webgpu driver what depth-stencil format the render
	// pass that executes our bundles will use. Required when our HUD
	// bundles need to share a pass with depth-enabled content (e.g.
	// three.js 3D scenes). Pass a WebGPU format string ('depth24plus',
	// 'depth24plus-stencil8', 'depth32float', ...) or null/undefined to
	// reset to no-depth-attachment mode.
	//
	// Not in the spec — sack-specific binding glue. Worth keeping on
	// the canvas context object because callers reach it from the same
	// place they configure the surface itself.
	static void setSurfaceDepthFormat( const v8::FunctionCallbackInfo<v8::Value>& );

	// Re-pull PSI ControlColorTypes (HIGHLIGHT..SCROLLBAR_BACK) into the
	// imglib-webgpu driver's palette uniform (slots 1..14). Useful after
	// PSI's preload has run (its base colours are zeros until then) or
	// after any SetBaseColor / theme swap. One queue write, no bundle
	// invalidation — CDATA palette references stay correct.
	static void refreshPSIPalette( const v8::FunctionCallbackInfo<v8::Value>& );

	// Not in the WebGPU spec (browsers auto-present), but required for
	// native runners — call after each frame's queue submission.
	static void present          ( const v8::FunctionCallbackInfo<v8::Value>& );
};

// Build a JS GPUCanvasContext for the given renderer's window. Returns an
// empty Local if WebGPU init has not happened yet or the renderer has no
// native handle. Caller is expected to throw on empty.
extern v8::Local<v8::Object> webgpu_canvas_context_for_renderer(
	v8::Isolate* isolate, PRENDERER r );
