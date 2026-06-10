// sack-gui imglib-webgpu — webgpu.render.3d interface registration.
//
// Provides RENDER3D_INTERFACE (see render3d.h). The interface is small —
// 6 entries, plus 3 Vulkan-only entries we deliberately omit since we
// don't define _VULKAN_DRIVER (we go through the WebGPU abstraction).
//
// This TU references ONLY the symbols it registers — function bodies
// live in render3d_ops.cc.

#include "local.h"

IMAGE_NAMESPACE

// Forward decls — bodies in render3d_ops.cc.
extern PTRANSFORM     CPROC wgpu_GetRenderTransform( PRENDERER );
extern LOGICAL        CPROC wgpu_ClipPoints( P_POINT points, int nPoints );
extern void           CPROC wgpu_GetViewVolume( PRAY *planes );
extern void           CPROC wgpu_SetRendererAnchorSpace( PRENDERER display, int anchor );
extern RenderPipeline CPROC wgpu_GetRenderPipe( RenderContext context );
extern RenderShader   CPROC wgpu_AddShader( RenderPipeline context, ... );

static RENDER3D_INTERFACE Render3dInterface = {
	  wgpu_GetRenderTransform
	, wgpu_ClipPoints
	, wgpu_GetViewVolume
	, wgpu_SetRendererAnchorSpace
	, wgpu_GetRenderPipe
	, wgpu_AddShader
	// _VULKAN_DRIVER conditional trio (getCommandBuffer/Geometry/Vertex)
	// is intentionally absent — _VULKAN_DRIVER is not defined for this
	// driver; we go through Dawn's webgpu.h, not the raw Vk handles.
};

static POINTER CPROC wgpu_GetRender3dInterface( void )
{
	return &Render3dInterface;
}

static void CPROC wgpu_DropRender3dInterface( POINTER p )
{
	(void)p;
}

PRIORITY_PRELOAD( WebgpuRender3dRegisterInterface, IMAGE_PRELOAD_PRIORITY )
{
	RegisterInterface( "webgpu.render.3d", wgpu_GetRender3dInterface, wgpu_DropRender3dInterface );
#if defined( STATIC_RENDER_INTERFACE )
	RegisterClassAlias( "system/interfaces/webgpu.render.3d", "system/interfaces/render.3d" );
#endif
}

IMAGE_NAMESPACE_END
