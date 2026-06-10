// sack-gui imglib-webgpu — webgpu.image.3d interface registration.
//
// Mirror of interface_image.cc: at first use we memcpy the resolved
// CPU image.3d interface in, then overwrite slots we GPU-handle.
//
// The image.3d interface is the puregl2 shader-buffer model
// (PImageShaderTracker, CompileShader, BeginShaderOp, AppendTristrip,
// shader_buffer, uniform setters). Our overrides will eventually
// translate it onto WGPUShaderModule + WGPURenderPipeline + a CPU
// vertex pool feeding WGPUBuffer at flush time. For now we keep all
// the bookkeeping entries (CreateShaderBuffer, SetShader* setters)
// as CPU delegates — they don't touch GL state — and only the
// compile/enable/output entries get GPU-aware overrides as we wire
// them up.

#include "local.h"

IMAGE_NAMESPACE

// Forward decls — bodies in image_3d_ops.cc.
extern int  CPROC wgpu_ImageCompileShader   ( PImageShaderTracker, char const*const*, int,
                                              char const*const*, int );
extern int  CPROC wgpu_ImageCompileShaderEx ( PImageShaderTracker, char const*const*, int,
                                              char const*const*, int,
                                              struct image_shader_attribute_order *, int );
extern void CPROC wgpu_ImageEnableShader    ( PImageShaderTracker, ... );

static IMAGE_3D_INTERFACE WebgpuImage3dInterface;
static int install_done = 0;

static void install_overrides( void )
{
	if( install_done )
		return;

	webgpu_image_ensure_init();

	PIMAGE_3D_INTERFACE cpu = l.cpu_image_3d_interface;
	if( !cpu )
		return;

	MemCpy( &WebgpuImage3dInterface, cpu, sizeof( IMAGE_3D_INTERFACE ) );

	WebgpuImage3dInterface._ImageCompileShader   = wgpu_ImageCompileShader;
	WebgpuImage3dInterface._ImageCompileShaderEx = wgpu_ImageCompileShaderEx;
	WebgpuImage3dInterface._ImageEnableShader    = wgpu_ImageEnableShader;

	install_done = 1;
}

static POINTER CPROC wgpu_GetImage3dInterface( void )
{
	install_overrides();
	return &WebgpuImage3dInterface;
}

static void CPROC wgpu_DropImage3dInterface( POINTER p )
{
	(void)p;
}

PRIORITY_PRELOAD( WebgpuImage3dRegisterInterface, IMAGE_PRELOAD_PRIORITY )
{
	RegisterInterface( "webgpu.image.3d", wgpu_GetImage3dInterface, wgpu_DropImage3dInterface );
#if defined( STATIC_RENDER_INTERFACE )
	RegisterClassAlias( "system/interfaces/webgpu.image.3d", "system/interfaces/image.3d" );
#endif
}

IMAGE_NAMESPACE_END
