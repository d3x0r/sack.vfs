// sack-gui imglib-webgpu — webgpu.image interface registration.
//
// Strategy: at first GetImageInterface() call (which fires once the
// sack interface system asks for our driver), we memcpy the resolved
// CPU sack.image interface into our local struct, then overwrite the
// entries we actually GPU-accelerate. This means every slot has a
// working implementation without 100+ trivial dispatch stubs, and
// the entries we never need to touch (font cache, color-channel
// accessors, sprite, sliced image, transfer-data, etc.) just pass
// through to the CPU implementation transparently.

#include "local.h"

IMAGE_NAMESPACE

// Forward decls of the entries we override — bodies in image_ops.cc /
// line_ops.cc / font_ops.cc.
extern void  CPROC wgpu_BlatColor              ( Image, int32_t, int32_t, uint32_t, uint32_t, CDATA );
extern void  CPROC wgpu_BlatColorAlpha         ( Image, int32_t, int32_t, uint32_t, uint32_t, CDATA );
extern void  CPROC wgpu_BlotImageEx            ( Image, Image, int32_t, int32_t, uint32_t, uint32_t, ... );
extern void  CPROC wgpu_BlotImageSizedEx       ( Image, Image, int32_t, int32_t, int32_t, int32_t,
                                                  uint32_t, uint32_t, uint32_t, uint32_t, ... );
extern void  CPROC wgpu_BlotScaledImageSizedEx ( Image, Image, int32_t, int32_t, uint32_t, uint32_t,
                                                  int32_t, int32_t, uint32_t, uint32_t,
                                                  uint32_t, uint32_t, ... );
extern void  CPROC wgpu_do_line                ( Image, int32_t, int32_t, int32_t, int32_t, CDATA );
extern void  CPROC wgpu_do_lineAlpha           ( Image, int32_t, int32_t, int32_t, int32_t, CDATA );
extern void  CPROC wgpu_do_hline               ( Image, int32_t, int32_t, int32_t, CDATA );
extern void  CPROC wgpu_do_vline               ( Image, int32_t, int32_t, int32_t, CDATA );
extern void  CPROC wgpu_do_hlineAlpha          ( Image, int32_t, int32_t, int32_t, CDATA );
extern void  CPROC wgpu_do_vlineAlpha          ( Image, int32_t, int32_t, int32_t, CDATA );
extern void  CPROC wgpu_PutCharacterFont       ( Image, int32_t, int32_t, int32_t, CDATA, CDATA, TEXTCHAR, SFTFont );
extern void  CPROC wgpu_PutStringFontEx        ( Image, int32_t, int32_t, int32_t, CDATA, CDATA, CTEXTSTR, size_t, SFTFont );
extern void  CPROC wgpu_PutStringFontExx       ( Image, int32_t, int32_t, int32_t, CDATA, CDATA, CTEXTSTR, size_t, SFTFont, int, uint32_t );
extern void  CPROC wgpu_MarkImageDirty         ( Image );
extern int   CPROC wgpu_ReloadTexture          ( Image, int );
extern int   CPROC wgpu_ReloadShadedTexture    ( Image, int, CDATA );
extern int   CPROC wgpu_ReloadMultiShadedTexture( Image, int, CDATA, CDATA, CDATA );
extern LOGICAL CPROC wgpu_IsImageTargetFinal   ( Image );
extern void  CPROC wgpu_UnmakeImageFileEx      ( Image pif DBG_PASS );  // hook to free wgpuPerImage
extern void  CPROC wgpu_ResetImageBuffers      ( Image img, LOGICAL image_only );

// Our interface table. Starts all-NULL. webgpu_image_install_overrides()
// memcpy's CPU defaults in, then overwrites the slots above.
static IMAGE_INTERFACE WebgpuImageInterface;
static int  install_done = 0;

static void install_overrides( void )
{
	if( install_done )
		return;

	webgpu_image_ensure_init();

	PIMAGE_INTERFACE cpu = webgpu_image_get_cpu();
	if( !cpu )
	{
		// CPU interface not yet registered. Leave the table NULL — caller
		// will likely retry. (Should only happen if our preload fires
		// before sack.image's, and the consumer doesn't use lazy GetInterface.)
		return;
	}

	MemCpy( &WebgpuImageInterface, cpu, sizeof( IMAGE_INTERFACE ) );

	// Override the entries we have GPU paths for. These functions all
	// internally do "if !FINAL_RENDER, delegate to CPU; else GPU path."
	WebgpuImageInterface._BlatColor               = wgpu_BlatColor;
	WebgpuImageInterface._BlatColorAlpha          = wgpu_BlatColorAlpha;
	WebgpuImageInterface._BlotImageEx             = wgpu_BlotImageEx;
	WebgpuImageInterface._BlotImageSizedEx        = wgpu_BlotImageSizedEx;
	WebgpuImageInterface._BlotScaledImageSizedEx  = wgpu_BlotScaledImageSizedEx;
	WebgpuImageInterface._do_line                 = wgpu_do_line;
	WebgpuImageInterface._do_lineAlpha            = wgpu_do_lineAlpha;
	WebgpuImageInterface._do_hline                = wgpu_do_hline;
	WebgpuImageInterface._do_vline                = wgpu_do_vline;
	WebgpuImageInterface._do_hlineAlpha           = wgpu_do_hlineAlpha;
	WebgpuImageInterface._do_vlineAlpha           = wgpu_do_vlineAlpha;
	WebgpuImageInterface._PutCharacterFont        = wgpu_PutCharacterFont;
	WebgpuImageInterface._PutStringFontEx         = wgpu_PutStringFontEx;
	WebgpuImageInterface._PutStringFontExx        = wgpu_PutStringFontExx;
	WebgpuImageInterface._MarkImageDirty          = wgpu_MarkImageDirty;
	WebgpuImageInterface._ReloadTexture           = wgpu_ReloadTexture;
	WebgpuImageInterface._ReloadShadedTexture     = wgpu_ReloadShadedTexture;
	WebgpuImageInterface._ReloadMultiShadedTexture = wgpu_ReloadMultiShadedTexture;
	WebgpuImageInterface._IsImageTargetFinal      = wgpu_IsImageTargetFinal;
	WebgpuImageInterface._UnmakeImageFileEx       = wgpu_UnmakeImageFileEx;
	WebgpuImageInterface._ResetImageBuffers       = wgpu_ResetImageBuffers;

	install_done = 1;
}

static PIMAGE_INTERFACE CPROC wgpu_GetImageInterface( void )
{
	install_overrides();
	return &WebgpuImageInterface;
}

static void CPROC wgpu_DropImageInterface( PIMAGE_INTERFACE p )
{
	(void)p;
}

PRIORITY_PRELOAD( WebgpuImageRegisterInterface, IMAGE_PRELOAD_PRIORITY )
{
	RegisterInterface( "webgpu.image",
	                   (void *(CPROC*)(void))wgpu_GetImageInterface,
	                   (void  (CPROC*)(void*))wgpu_DropImageInterface );
#if defined( STATIC_RENDER_INTERFACE )
	RegisterClassAlias( "system/interfaces/webgpu.image", "system/interfaces/image" );
#endif
}

// Returns the (lazily-populated) PIMAGE_INTERFACE for our driver. Used by
// the renderer's getContext('webgpu') to set image->reverse_interface on
// the surface — that's how sack's font code (and any other reverse-aware
// code in sack imglib) is routed back through us when it would otherwise
// hit the global "image" interface (= CPU sack.image) for atlas blits.
PIMAGE_INTERFACE webgpu_image_get_interface_for_reverse( void )
{
	install_overrides();
	return install_done ? &WebgpuImageInterface : NULL;
}

IMAGE_NAMESPACE_END

// C-linkage shim outside any namespace so non-imglib TUs (e.g.
// sack_render_module) can fetch the pointer without dealing with the
// sack::image::Interface namespace gymnastics. Returns void* — caller
// casts to the struct image_interface_tag * type that
// ImageFile_tag::reverse_interface uses.
extern "C" void *sack_image_get_webgpu_reverse_for_render( void )
{
	return (void *)sack::image::webgpu_image_get_interface_for_reverse();
}
