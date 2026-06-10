// sack-gui imglib-webgpu — webgpu.render.3d function bodies.
//
// All stubs at the moment. PSI's native 3D path is the main consumer of
// this interface; for the initial bring-up most callers will be the
// three.js/Dawn JS path which doesn't reach through here at all.

#include "local.h"

IMAGE_NAMESPACE

PTRANSFORM CPROC wgpu_GetRenderTransform( PRENDERER renderer )
{
	(void)renderer;
	// TODO: return the active camera transform for the renderer. Until
	// we have a per-renderer state struct hooked up, callers get NULL —
	// which most paths defensively check.
	return NULL;
}

LOGICAL CPROC wgpu_ClipPoints( P_POINT points, int nPoints )
{
	(void)points; (void)nPoints;
	// TODO: real frustum test against the active camera view volume.
	// Permissive default — never report "all points outside" so callers
	// don't accidentally skip draws while we're a stub.
	return TRUE;
}

void CPROC wgpu_GetViewVolume( PRAY *planes )
{
	(void)planes;
	// TODO: write 6 PRAY planes (left/right/bottom/top/near/far) of the
	// active camera frustum into planes[0..5].
}

void CPROC wgpu_SetRendererAnchorSpace( PRENDERER display, int anchor )
{
	(void)display; (void)anchor;
	// TODO: track the anchor (0=world / 1=local / 2=view) on the renderer
	// so subsequent draws use the matching base transform.
}

RenderPipeline CPROC wgpu_GetRenderPipe( RenderContext context )
{
	(void)context;
	// TODO: return a handle to a WebGPU pipeline-state object once we
	// have one to return. Pipelines in WebGPU are concrete (already
	// baked vertex+fragment+state); the puregl2 notion of "pipe" was
	// looser. For now NULL.
	return NULL;
}

RenderShader CPROC wgpu_AddShader( RenderPipeline context, ... )
{
	(void)context;
	// TODO: variadic shader-attachment add. Defer until image_3d's
	// CompileShader is implemented — they share the WGSL compile path.
	return NULL;
}

IMAGE_NAMESPACE_END
