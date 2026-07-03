// sack-gui imglib-webgpu — shared types and globals.
//
// This driver registers three sack interfaces against the WebGPU/Dawn layer
// already linked into sack-gui:
//
//   webgpu.image      (IMAGE_INTERFACE)     — interface_image.cc
//   webgpu.image.3d   (IMAGE_3D_INTERFACE)  — interface_image_3d.cc
//   webgpu.render.3d  (RENDER3D_INTERFACE)  — interface_render_3d.cc
//
// Each interface_*.cc is the registration TU — it owns the struct of
// function pointers and the PRIORITY_PRELOAD that calls RegisterInterface.
// Function bodies live in the *_ops.cc TUs.
//
// Per-image GPU state is held in a driver-side PTREEROOT keyed by Image*
// rather than embedded in ImageFile_tag. Embedding would have required
// -D_WEBGPU_DRIVER propagated to every TU that allocates an ImageFile —
// including sack's own CPU imglib (the sack.image fallback the driver
// delegates to). Side-table costs one tree lookup per access, which is
// rounding error next to the actual draw work.
//
// CPU fallback: at first-use we resolve sack.image / sack.image++ and copy
// its function pointer table into ours; entries we want to GPU-accelerate
// are then overwritten. Result: every IMAGE_INTERFACE slot has a working
// implementation without 100+ trivial dispatch stubs.

#ifndef SACK_GUI_IMGLIB_WEBGPU_LOCAL_H
#define SACK_GUI_IMGLIB_WEBGPU_LOCAL_H

#ifndef _WEBGPU_DRIVER
#define _WEBGPU_DRIVER
#endif

#ifndef __3D__
#define __3D__
#endif

#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
#include <node.h>

#include <stdhdrs.h>
#include <procreg.h>
#include <imglib/imagestruct.h>
#include <imglib/fontstruct.h>
#include <image.h>
#include <image3d.h>
#include <render.h>
#include <render3d.h>


#undef New
#undef Reallocate
#undef Allocate

#include "webgpu/webgpu.h"

IMAGE_NAMESPACE

// ----------------------------------------------------------------------
// Per-driver global state.
// ----------------------------------------------------------------------
//
// device_ / queue_ are borrowed from a WGPUDevice created on the JS side
// via sack.gpu.requestAdapter().requestDevice(). The webgpu_module exposes
// webgpu_get_active_device() / webgpu_get_active_queue() (added alongside
// the existing webgpu_get_instance() accessor) so we can pick them up
// lazily on first draw.
//
struct webgpu_image_driver_state
{
	// Cached CPU image interface — used as the fallback for every entry
	// we don't GPU-accelerate. Resolved lazily on first use to dodge
	// PRIORITY_PRELOAD ordering issues.
	PIMAGE_INTERFACE     cpu_image_interface;
	PIMAGE_3D_INTERFACE  cpu_image_3d_interface;

	// Borrowed device/queue. NULL until JS produces a device.
	WGPUDevice device_;
	WGPUQueue  queue_;

	// Four internal pipelines, lazily created on first draw that needs them.
	//
	//   pipe_solid    : pos(vec2) + color(u8x4)                — lines, rects, BlatColor*
	//   pipe_textured : pos(vec2) + uv(vec2) + tint(u8x4)      — BlotImage COPY / SHADED
	//   pipe_multi    : pos(vec2) + uv(vec2) + 3×u8x4 shade    — BlotImage MULTISHADE
	//   pipe_normal   : pos+uv+tint, diffuse + normal map      — lit BlotImage with normal-map
	//
	WGPURenderPipeline pipe_solid_;
	WGPURenderPipeline pipe_textured_;
	WGPURenderPipeline pipe_multi_;
	WGPURenderPipeline pipe_normal_;

	WGPUBindGroupLayout bgl_globals_;        // group(0): palette + lighting uniforms
	WGPUBindGroupLayout bgl_texture_;        // group(1): sampler + diffuse  (solid/textured/multi)
	WGPUBindGroupLayout bgl_texture_normal_; // group(1): sampler + diffuse + normal (normal pipe)
	WGPUSampler        sampler_linear_;
	WGPUSampler        sampler_nearest_;

	// Theme palette. Late-resolved colors: vertices carry a base-color index
	// (or literal RGBA when the high bit is set); the fragment shader looks
	// up palette_[index] when the index path is taken. Theme swap rewrites
	// this buffer; no bundle invalidation needed.
	WGPUBuffer    palette_buffer_;
	WGPUBuffer    lighting_buffer_;          // Lighting uniform — see shaders.wgsl
	WGPUBindGroup globals_group_;            // bound at group(0) — palette + lighting
	uint32_t      palette_generation_;

	// Color format of the swapchain attachment our bundles target. Set once
	// by the binding (after surface configure) via webgpu_image_set_surface_format,
	// or defaults to BGRA8Unorm on Windows / RGBA8Unorm elsewhere — the
	// format is a platform constant in practice (not even GL had this vary).
	WGPUTextureFormat surface_format_;

	// Depth-stencil format of the render pass that will execute our bundles.
	// Undefined = pure 2D pass (no depth attachment) — bundles record with
	// depthStencilFormat=Undefined.  Non-Undefined = mixed-mode pass (e.g.
	// three.js 3D content + our HUD bundles on top); bundles record with
	// depthStencilFormat=this + depthReadOnly=true so they don't write depth
	// but pass attachment compatibility checks. Caller (canvas binding /
	// three.js integration) sets this via webgpu_image_set_surface_depth_format
	// before any drawing.
	WGPUTextureFormat surface_depth_format_;

	// Bumped when any font's atlas resizes (so per-image bundles can compare
	// against bundle_atlas_generation_ and re-record).
	uint32_t   font_atlas_generation_;

	// Bumped whenever surface_depth_format_ changes. Each per-image entry
	// records the value it last encoded its bundle with; mismatch on a
	// frame walk forces a re-record so the bundle's attachment state
	// catches up.
	uint32_t   depth_format_generation_;

	RCOORD scale;

	// Surface dimensions. Used to convert PSI pixel coords to NDC at
	// op-record time. The binding/sack-side hook calls
	// webgpu_image_set_surface_size on canvas configure/resize.
	uint32_t surface_w_;
	uint32_t surface_h_;

	uint32_t flags_initialized_ : 1;
};

#ifndef IMAGE_LIBRARY_SOURCE_MAIN
extern
#endif
struct webgpu_image_driver_state webgpu_image_driver;
#define l webgpu_image_driver

// First-use initializer. Idempotent. Resolves the CPU interface(s) and
// (if the JS device is ready) the device/queue. Returns TRUE if both the
// CPU fallback AND the device are now usable, FALSE if the device is not
// yet available (CPU fallback can still be used regardless).
LOGICAL webgpu_image_ensure_init( void );

// Returns our driver's IMAGE_INTERFACE pointer, populated and ready to
// be installed as an image's reverse_interface. NULL if the CPU fallback
// hasn't resolved yet (very early init).
PIMAGE_INTERFACE webgpu_image_get_interface_for_reverse( void );

// Returns the resolved CPU image interface, or NULL if it hasn't been
// registered yet. The override functions use this for non-FINAL_RENDER
// targets.
PIMAGE_INTERFACE webgpu_image_get_cpu( void );

// Pipeline lazy-creation. Returns NULL if the device isn't ready.
WGPURenderPipeline webgpu_image_get_pipe_solid( void );
WGPURenderPipeline webgpu_image_get_pipe_textured( void );
WGPURenderPipeline webgpu_image_get_pipe_multi( void );
WGPURenderPipeline webgpu_image_get_pipe_normal( void );

// Setters wired up by sack-side / JS-side code as appropriate.
void webgpu_image_set_surface_format       ( WGPUTextureFormat fmt );
void webgpu_image_set_surface_depth_format ( WGPUTextureFormat fmt );
void webgpu_image_set_surface_size         ( uint32_t w, uint32_t h );
void webgpu_image_set_light_direction( float x, float y, float z );
void webgpu_image_set_light_color    ( float r, float g, float b );
void webgpu_image_set_ambient        ( float r, float g, float b, float intensity );
void webgpu_image_set_palette_entry  ( int idx, float r, float g, float b, float a );
void webgpu_image_refresh_psi_palette( void );  // re-pulls slots 1..14 from PSI

// Bind a normal map to an image. When a BlotImage* targets this image as
// source, the driver routes the draw through pipe_normal_ instead of
// pipe_textured_. Pass NULL normal to clear.
void webgpu_image_set_normal_map( Image diffuse, Image normal );

// Ensure src has an up-to-date GPU texture mirroring its CPU pixmap.
// Returns NULL if device not ready or src has no data. Idempotent.
WGPUTexture webgpu_image_ensure_src_texture( Image src );

// Per-source bind groups (sampler + texture(s)). Lazy-created and cached
// on the source's webgpu_per_image; invalidated when the texture changes.
WGPUBindGroup webgpu_image_get_diffuse_bg( Image src );
WGPUBindGroup webgpu_image_get_normal_bg ( Image src );

// ----------------------------------------------------------------------
// Per-image extra state — hangs off ImageFile::wgpuPerImage.
// ----------------------------------------------------------------------
struct webgpu_per_image
{
	// Source-side: the GPU texture mirroring the CPU pixmap, refreshed
	// from img->image whenever IF_FLAG_UPDATED is set.
	WGPUTexture     src_texture_;
	WGPUTextureView src_view_;
	uint32_t        src_upload_generation_;

	// Optional normal map paired with this image. When present, BlotImage*
	// sourcing this image routes through pipe_normal_. Set via
	// webgpu_image_set_normal_map(). NOT owned — caller owns the Image
	// lifetime; the per-image entry holds borrowed pointers and is
	// invalidated by the normal image's own UnmakeImageFileEx.
	Image           normal_image_;

	// Cached bind groups for group(1). Recreated on src_texture_ change.
	WGPUBindGroup   diffuse_bg_;          // sampler + diffuse
	WGPUBindGroup   normal_bg_;           // sampler + diffuse + normal
	Image           normal_bg_for_image_; // what normal_image_ was when normal_bg_ was built

	// Target-side: per-subimage retained render bundle. NULL means
	// "needs recording on next frame walk."
	WGPURenderBundle bundle_;
	uint32_t         bundle_lock_;            // simple LockedExchange spinlock
	uint32_t         dirty_;                  // atomic; set by any thread on op
	uint32_t         bundle_atlas_generation_;
	uint32_t         bundle_palette_generation_;
	uint32_t         bundle_depth_generation_;

	// Retained op-list snapshot — what the bundle was recorded from.
	struct webgpu_op_list *ops_;

	// Set after a bundle record. The next draw op on this subimage
	// resets the op-list before appending — that's how we model PSI's
	// "smudged control redraws itself fresh" semantics without needing
	// PSI to explicitly clear anything.
	uint32_t needs_reset_;
};
struct webgpu_op_list;  // opaque — defined in bundle.cc

// ----------------------------------------------------------------------
// Op-list API — called from image_ops.cc when recording a GPU draw, and
// from the frame walker to encode the recorded ops into a WGPURenderBundle.
//
// The list is segregated by pipeline (solid / textured / multi-shaded) so
// the encoder can issue one setPipeline+draw per pipeline-in-use, with
// inner batch breaks only on texture changes for the textured pipelines.
//
// All append calls are caller-locked: ops_list itself is not thread-safe.
// Callers (subimage recording paths) take the per-subimage lock first.
// ----------------------------------------------------------------------

struct webgpu_op_list *webgpu_op_list_new   ( void );
void                   webgpu_op_list_release( struct webgpu_op_list *ops );
void                   webgpu_op_list_reset ( struct webgpu_op_list *ops );

// Solid-colored quad in clip space. Vertex order: TL, TR, BL, BR (tristrip).
// `color` is the packed u32 (palette-index encoding or literal RGBA per the
// resolve_color() convention in shaders.wgsl).
void webgpu_op_list_solid_quad( struct webgpu_op_list *ops,
                                 float x0, float y0, float x1, float y1,
                                 uint32_t color );

// Solid-colored arbitrary tristrip (4 NDC verts in tristrip order).
// Used for diagonal lines — perpendicular-offset quads aren't axis-aligned.
void webgpu_op_list_solid_tristrip( struct webgpu_op_list *ops,
                                     const float verts[4][2],
                                     uint32_t color );

// Textured quad with per-vertex tint. `verts` and `uvs` are 4×vec2 arrays
// (caller fills in any transform). Batch is keyed by `source` so the encoder
// can look up the cached bind group at record time. Texture upload is
// resolved on demand via webgpu_image_ensure_src_texture.
void webgpu_op_list_textured_quad( struct webgpu_op_list *ops,
                                    const float verts[4][2],
                                    const float uvs[4][2],
                                    uint32_t tint,
                                    Image source );

// Multi-shaded textured quad — three per-vertex shade colors (one per
// source channel).
void webgpu_op_list_multi_quad( struct webgpu_op_list *ops,
                                 const float verts[4][2],
                                 const float uvs[4][2],
                                 uint32_t shadeR, uint32_t shadeG, uint32_t shadeB,
                                 Image source );

// Normal-mapped textured quad — same vertex format as textured. The
// source's previously bound normal-map (via webgpu_image_set_normal_map)
// is picked up at encode time when the normal-pipeline bind group is built.
void webgpu_op_list_normal_quad( struct webgpu_op_list *ops,
                                  const float verts[4][2],
                                  const float uvs[4][2],
                                  uint32_t tint,
                                  Image source );

// Encode the op-list into a new WGPURenderBundle. Returns NULL if no ops
// to encode (or device not yet ready). The caller owns the returned bundle
// and must wgpuRenderBundleRelease when done. Resets the op-list as a
// side effect — caller should not retain it.
//
// `color_format` should match the target attachment so the bundle is
// compatible with the executing pass.
WGPURenderBundle webgpu_op_list_record( struct webgpu_op_list *ops,
                                         WGPUTextureFormat color_format );

// Allocate-on-demand for an image's per-image state.
struct webgpu_per_image *webgpu_per_image_get( Image img );
void webgpu_per_image_release( Image img );

// Hook the frame walker calls to compose dirty subimages back into a bundle.
void webgpu_image_mark_dirty( Image img );
void webgpu_image_execute_into_pass( WGPURenderPassEncoder pass, Image root );

// Designate the PSI image that's bound to the active surface — i.e. the
// root of the image tree we should walk when the binding tells us a
// surface-targeting render pass is about to end. Pass NULL to detach.
// Called by sack-side code that connects an Image to a canvas. Parameter
// is void* so the canvas binding TU doesn't have to pull sack imglib
// headers (Image == struct ImageFile_tag *).
void webgpu_image_set_surface_root( void *root );

IMAGE_NAMESPACE_END

// ----------------------------------------------------------------------
// C-linkage hook called from the webgpu_module render path. Declared
// outside the IMAGE_NAMESPACE so the binding can name it without
// namespace gymnastics. Implementation in driver.cc forwards to the
// namespaced webgpu_image_execute_into_pass against the surface-root
// previously set via webgpu_image_set_surface_root().
extern "C" void webgpu_image_dispatch_surface_pass( WGPURenderPassEncoder pass );

#endif // SACK_GUI_IMGLIB_WEBGPU_LOCAL_H
