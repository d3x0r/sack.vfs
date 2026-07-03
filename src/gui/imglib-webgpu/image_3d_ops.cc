// sack-gui imglib-webgpu — IMAGE_3D_INTERFACE overrides.
//
// Compile callers' WGSL into a WGPUShaderModule, and stash the resulting
// module on PImageShaderTracker via the existing psvInit slot. For now
// these are stubs that print and fall through to CPU; full impl lands
// once driver.cc has the device wired.

#include "local.h"

IMAGE_NAMESPACE

int CPROC wgpu_ImageCompileShader( PImageShaderTracker tracker,
                                    char const*const* vertex_code, int vert_blocks,
                                    char const*const* frag_code,   int frag_blocks )
{
	// Convention: vertex_code / frag_code are WGSL source strings (not
	// GLSL). The puregl2-era GLSL is not translated; callers porting
	// from puregl2 are expected to re-author their shaders in WGSL.
	(void)tracker; (void)vertex_code; (void)vert_blocks;
	(void)frag_code; (void)frag_blocks;
	// TODO: concat blocks, call wgpuDeviceCreateShaderModule with a
	// WGPUShaderSourceWGSL chain, build a render pipeline against
	// driver-known vertex layouts, stash pipeline+module on tracker.
	return 0;
}

int CPROC wgpu_ImageCompileShaderEx( PImageShaderTracker tracker,
                                      char const*const* vertex_code, int vert_blocks,
                                      char const*const* frag_code,   int frag_blocks,
                                      struct image_shader_attribute_order *attribs, int nAttribs )
{
	(void)attribs; (void)nAttribs;
	return wgpu_ImageCompileShader( tracker, vertex_code, vert_blocks, frag_code, frag_blocks );
}

void CPROC wgpu_ImageEnableShader( PImageShaderTracker tracker, ... )
{
	// TODO: set the active pipeline on the current pass encoder; bind
	// the palette + sampler/texture bind groups. Variadic args are
	// shader-specific uniforms — defer to the tracker's Enable callback.
	(void)tracker;
}

IMAGE_NAMESPACE_END
