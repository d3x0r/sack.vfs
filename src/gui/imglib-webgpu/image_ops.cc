// sack-gui imglib-webgpu — IMAGE_INTERFACE overrides.
//
// Each function:
//   1. If the target isn't a FINAL_RENDER non-IN_MEMORY image, delegate
//      to the CPU sack.image implementation — that path mutates the
//      pixmap directly and there's nothing GPU to do.
//   2. Otherwise: record into the per-subimage op list (under lock),
//      mark dirty, return. The frame walker will re-encode the
//      WGPURenderBundle on the next frame tick.
//
// Bodies are stubs for the moment — the dispatch shape is wired up so
// the driver builds and registers; recording will be filled in once
// bundle.cc lands.

#include "local.h"

IMAGE_NAMESPACE

static inline LOGICAL is_gpu_target( Image img )
{
	if( !img ) return FALSE;
	return ( img->flags & IF_FLAG_FINAL_RENDER ) && !( img->flags & IF_FLAG_IN_MEMORY );
}

#define CPU_DELEGATE_VOID( name, args ) \
	do { PIMAGE_INTERFACE cpu = webgpu_image_get_cpu(); \
	     if( cpu && cpu->_##name ) cpu->_##name args; } while( 0 )

// Convert sack pixel coords on `dest` into NDC (clip space). Y-flip because
// WebGPU NDC is Y-up while sack pixels are Y-down. dest->real_width/height
// give the subimage's own dimensions — the frame walker sets the viewport
// to position the subimage on the surface at execute time.
static inline void px_to_ndc( Image dest, float x, float y, float *nx, float *ny )
{
	float W = (float)dest->real_width;
	float H = (float)dest->real_height;
	if( W <= 0 ) W = 1;
	if( H <= 0 ) H = 1;
	*nx = (x / W) * 2.0f - 1.0f;
	*ny = 1.0f - (y / H) * 2.0f;
}

// First call after a bundle record clears the op-list so the next round of
// PSI draw ops on this subimage starts fresh — PSI's "smudged" model means
// any new draw op kicks off a complete re-render of the subimage.
static inline struct webgpu_op_list *get_fresh_ops( struct webgpu_per_image *pi )
{
	if( pi->needs_reset_ ) {
		if( pi->ops_ ) webgpu_op_list_reset( pi->ops_ );
		pi->needs_reset_ = 0;
	}
	if( !pi->ops_ )
		pi->ops_ = webgpu_op_list_new();
	return pi->ops_;
}

// CDATA is sack's 32-bit packed ARGB (alpha in the high byte). Our shaders
// resolve_color() treats alpha == 0 as "palette index" mode and any nonzero
// alpha as "literal RGBA". So a fully transparent CDATA(0) would round-trip
// through palette[0]; intentional but worth knowing.

// Forward decl — wgpu_BlotImageEx forwards through this; defined below.
void CPROC wgpu_BlotImageSizedEx( Image dest, Image src,
                                   int32_t xd, int32_t yd,
                                   int32_t xs, int32_t ys,
                                   uint32_t ws, uint32_t hs,
                                   uint32_t nTransparent, uint32_t method, ... );

// ----------------------------------------------------------------------
// Color / blit
// ----------------------------------------------------------------------

void CPROC wgpu_BlatColor( Image dest, int32_t x, int32_t y, uint32_t w, uint32_t h, CDATA color )
{
	if( !is_gpu_target( dest ) ) {
		CPU_DELEGATE_VOID( BlatColor, ( dest, x, y, w, h, color ) );
		return;
	}
	struct webgpu_per_image *pi = webgpu_per_image_get( dest );
	if( !pi ) return;
	struct webgpu_op_list *ops = get_fresh_ops( pi );
	float x0, y0, x1, y1;
	px_to_ndc( dest, (float)x,         (float)y,         &x0, &y0 );
	px_to_ndc( dest, (float)(x + (int32_t)w), (float)(y + (int32_t)h), &x1, &y1 );
	webgpu_op_list_solid_quad( ops, x0, y0, x1, y1, (uint32_t)color );
	pi->dirty_ = 1;
}

void CPROC wgpu_BlatColorAlpha( Image dest, int32_t x, int32_t y, uint32_t w, uint32_t h, CDATA color )
{
	// For our pipeline, alpha-blended draw is the default — BlatColorAlpha
	// is the same path as BlatColor. (CPU split exists because the CPU
	// loop has separate fast paths.)
	wgpu_BlatColor( dest, x, y, w, h, color );
}

void CPROC wgpu_BlotImageEx( Image dest, Image src, int32_t x, int32_t y,
                              uint32_t nTransparent, uint32_t method, ... )
{
	if( !src ) return;
	va_list ap;
	va_start( ap, method );
	CDATA r = 0, g = 0, b = 0;
	if( method == BLOT_SHADED ) {
		r = va_arg( ap, CDATA );
	} else if( method == BLOT_MULTISHADE ) {
		r = va_arg( ap, CDATA );
		g = va_arg( ap, CDATA );
		b = va_arg( ap, CDATA );
	}
	va_end( ap );
	wgpu_BlotImageSizedEx( dest, src, x, y, 0, 0,
	                       src->real_width, src->real_height,
	                       nTransparent, method,
	                       r, g, b );
}

// Internal — both Sized and Scaled variants flow through here. Sized passes
// wd=ws and hd=hs (no scaling); Scaled passes whatever dest dims the caller
// asked for. The GPU handles the scale implicitly via the textured quad.
static void blot_record( Image dest, Image src,
                          int32_t xd, int32_t yd, uint32_t wd, uint32_t hd,
                          int32_t xs, int32_t ys, uint32_t ws, uint32_t hs,
                          uint32_t method, va_list *ap )
{
	struct webgpu_per_image *pi = webgpu_per_image_get( dest );
	if( !pi ) return;
	if( !webgpu_image_ensure_src_texture( src ) )
		return;
	struct webgpu_op_list *ops = get_fresh_ops( pi );

	// Dest quad in NDC — uses the *dest* dimensions (wd, hd).
	float verts[4][2];
	px_to_ndc( dest, (float)xd,                 (float)yd,                 &verts[0][0], &verts[0][1] );
	px_to_ndc( dest, (float)(xd + (int32_t)wd), (float)yd,                 &verts[1][0], &verts[1][1] );
	px_to_ndc( dest, (float)xd,                 (float)(yd + (int32_t)hd), &verts[2][0], &verts[2][1] );
	px_to_ndc( dest, (float)(xd + (int32_t)wd), (float)(yd + (int32_t)hd), &verts[3][0], &verts[3][1] );

	// Source UVs. If src is a subimage (e.g. a font cell on an atlas),
	// the GPU texture covers the entire parent atlas, so we have to
	// translate the cell-local (xs, ys) into atlas-relative coords and
	// normalize against the atlas's dimensions — not the subimage's.
	Image src_root = src;
	int32_t atlas_xs = xs;
	int32_t atlas_ys = ys;
	{
		Image walk = src;
		while( walk->pParent ) {
			atlas_xs += walk->real_x;
			atlas_ys += walk->real_y;
			walk = walk->pParent;
		}
		src_root = walk;
	}
	float SW = (float)src_root->real_width;  if( SW <= 0 ) SW = 1;
	float SH = (float)src_root->real_height; if( SH <= 0 ) SH = 1;
	float uvs[4][2] = {
		{  atlas_xs                  / SW,  atlas_ys                  / SH },
		{ (atlas_xs + (int32_t)ws)   / SW,  atlas_ys                  / SH },
		{  atlas_xs                  / SW, (atlas_ys + (int32_t)hs)   / SH },
		{ (atlas_xs + (int32_t)ws)   / SW, (atlas_ys + (int32_t)hs)   / SH },
	};

	// IF_FLAG_INVERTED means the CPU storage is bottom-up — row 0 in
	// memory = visual bottom. The GPU texture got those bytes as-is, so
	// to display right-side-up we sample with V flipped. The flag is
	// a property of the root pixmap, so check it there.
	if( src_root->flags & IF_FLAG_INVERTED ) {
		uvs[0][1] = 1.0f - uvs[0][1];
		uvs[1][1] = 1.0f - uvs[1][1];
		uvs[2][1] = 1.0f - uvs[2][1];
		uvs[3][1] = 1.0f - uvs[3][1];
	}

	uint32_t tint = 0xFFFFFFFFu;
	// Normal map / per-image override is keyed at the root level too —
	// a subimage inherits the parent atlas's normal-map binding.
	struct webgpu_per_image *src_pi = webgpu_per_image_get( src_root );
	int has_normal = ( src_pi && src_pi->normal_image_ ) ? 1 : 0;

	// Batches are keyed by the *root* image — different cells of the
	// same atlas share a bind group and should land in the same batch.
	Image batch_src = src_root;

	switch( method ) {
	case BLOT_SHADED: {
		CDATA shade = va_arg( *ap, CDATA );
		if( has_normal )
			webgpu_op_list_normal_quad  ( ops, verts, uvs, (uint32_t)shade, batch_src );
		else
			webgpu_op_list_textured_quad( ops, verts, uvs, (uint32_t)shade, batch_src );
		break;
	}
	case BLOT_MULTISHADE: {
		CDATA r = va_arg( *ap, CDATA );
		CDATA g = va_arg( *ap, CDATA );
		CDATA b = va_arg( *ap, CDATA );
		webgpu_op_list_multi_quad( ops, verts, uvs,
		                           (uint32_t)r, (uint32_t)g, (uint32_t)b, batch_src );
		break;
	}
	default: // BLOT_COPY or anything else
		if( has_normal )
			webgpu_op_list_normal_quad  ( ops, verts, uvs, tint, batch_src );
		else
			webgpu_op_list_textured_quad( ops, verts, uvs, tint, batch_src );
		break;
	}
	pi->dirty_ = 1;
}

void CPROC wgpu_BlotImageSizedEx( Image dest, Image src,
                                   int32_t xd, int32_t yd,
                                   int32_t xs, int32_t ys,
                                   uint32_t ws, uint32_t hs,
                                   uint32_t nTransparent, uint32_t method, ... )
{
	if( !src ) return;
	if( !is_gpu_target( dest ) ) {
		PIMAGE_INTERFACE cpu = webgpu_image_get_cpu();
		if( cpu && cpu->_BlotImageSizedEx )
			cpu->_BlotImageSizedEx( dest, src, xd, yd, xs, ys, ws, hs, nTransparent, method );
		return;
	}
	va_list ap;
	va_start( ap, method );
	// Sized = no scaling: dest dims == source dims.
	blot_record( dest, src, xd, yd, ws, hs, xs, ys, ws, hs, method, &ap );
	va_end( ap );
	(void)nTransparent;  // alpha-blend pipeline handles transparency uniformly
}

void CPROC wgpu_BlotScaledImageSizedEx( Image dest, Image src,
                                         int32_t xd, int32_t yd, uint32_t wd, uint32_t hd,
                                         int32_t xs, int32_t ys, uint32_t ws, uint32_t hs,
                                         uint32_t nTransparent, uint32_t method, ... )
{
	if( !src ) return;
	if( !is_gpu_target( dest ) ) {
		PIMAGE_INTERFACE cpu = webgpu_image_get_cpu();
		if( cpu && cpu->_BlotScaledImageSizedEx )
			cpu->_BlotScaledImageSizedEx( dest, src, xd, yd, wd, hd, xs, ys, ws, hs, nTransparent, method );
		return;
	}
	va_list ap;
	va_start( ap, method );
	blot_record( dest, src, xd, yd, wd, hd, xs, ys, ws, hs, method, &ap );
	va_end( ap );
	(void)nTransparent;
}

// ----------------------------------------------------------------------
// Lifecycle hooks (only the ones we need to intercept)
// ----------------------------------------------------------------------

void CPROC wgpu_MarkImageDirty( Image img )
{
	// Mark our per-image dirty flag; the frame walker re-records.
	webgpu_image_mark_dirty( img );
	// Then let the CPU side run its own bookkeeping (peers, parent flags).
	PIMAGE_INTERFACE cpu = webgpu_image_get_cpu();
	if( cpu && cpu->_MarkImageDirty ) cpu->_MarkImageDirty( img );
}

void CPROC wgpu_UnmakeImageFileEx( Image pif DBG_PASS )
{
	// Free GPU-side state before the CPU side releases the ImageFile.
	webgpu_per_image_release( pif );
	PIMAGE_INTERFACE cpu = webgpu_image_get_cpu();
	if( cpu && cpu->_UnmakeImageFileEx ) cpu->_UnmakeImageFileEx( pif DBG_RELAY );
}

// Reset clears retained GPU state so the next draw cycle starts from a
// clean canvas. PSI's "smudged control redraws fresh" semantics already
// handle the per-op-list reset on the next draw, but reset is the
// explicit "invalidate everything *now*" signal — drop the cached bundle
// and the recorded ops outright so the surface goes blank until something
// new is drawn into it.
void CPROC wgpu_ResetImageBuffers( Image img, LOGICAL image_only )
{
	(void)image_only;   // CPU pixmap reset semantics; irrelevant on GPU side
	if( !img ) return;
	struct webgpu_per_image *pi = webgpu_per_image_get( img );
	if( !pi ) return;

	// Take the bundle lock so the frame walker can't grab a half-released
	// bundle out from under us.
	while( LockedExchange( &pi->bundle_lock_, 1 ) )
		Relinquish();
	if( pi->bundle_ ) {
		wgpuRenderBundleRelease( pi->bundle_ );
		pi->bundle_ = NULL;
	}
	if( pi->ops_ )
		webgpu_op_list_reset( pi->ops_ );
	pi->needs_reset_ = 0;   // ops_ is now empty; no pending reset to do
	pi->dirty_       = 1;   // signal walker to re-attempt — produces a
	                        // NULL bundle until next draw, which is the
	                        // visible "blank" state we want.
	pi->bundle_lock_ = 0;
}

// ----------------------------------------------------------------------
// Texture reloads — the puregl2 / D3D drivers do CPU→GPU texture upload
// here; we do the same with wgpuQueueWriteTexture.
// ----------------------------------------------------------------------

int CPROC wgpu_ReloadTexture( Image img, int option )
{
	(void)option;
	if( !webgpu_image_ensure_init() ) return 0;
	// TODO: if src_texture_ doesn't exist or size mismatch, (re)create it.
	//       wgpuQueueWriteTexture from img->image when IF_FLAG_UPDATED.
	(void)img;
	return 0;
}

int CPROC wgpu_ReloadShadedTexture( Image img, int option, CDATA color )
{
	(void)img; (void)option; (void)color;
	// Same as Reload but the texture is shaded by `color` — for our
	// renderer the shade is applied at draw time (per-vertex tint), so
	// this reduces to a regular reload.
	return wgpu_ReloadTexture( img, option );
}

int CPROC wgpu_ReloadMultiShadedTexture( Image img, int option, CDATA r, CDATA g, CDATA b )
{
	(void)r; (void)g; (void)b;
	return wgpu_ReloadTexture( img, option );
}

LOGICAL CPROC wgpu_IsImageTargetFinal( Image img )
{
	return is_gpu_target( img );
}

// Line ops are in line_ops.cc.

// ----------------------------------------------------------------------
// Font / text — delegate to CPU. Our atlas-on-GPU path replaces these
// in font_ops.cc once that lands.
// ----------------------------------------------------------------------

void CPROC wgpu_PutCharacterFont( Image img, int32_t x, int32_t y, int32_t h,
                                   CDATA color, CDATA bg, TEXTCHAR ch, SFTFont font ) {
	CPU_DELEGATE_VOID( PutCharacterFont, ( img, x, y, h, color, bg, ch, font ) );
}

void CPROC wgpu_PutStringFontEx( Image img, int32_t x, int32_t y, int32_t h,
                                  CDATA color, CDATA bg, CTEXTSTR pc, size_t nLen, SFTFont font ) {
	CPU_DELEGATE_VOID( PutStringFontEx, ( img, x, y, h, color, bg, pc, nLen, font ) );
}

void CPROC wgpu_PutStringFontExx( Image img, int32_t x, int32_t y, int32_t h,
                                   CDATA color, CDATA bg, CTEXTSTR pc, size_t nLen, SFTFont font,
                                   int justification, uint32_t width ) {
	CPU_DELEGATE_VOID( PutStringFontExx, ( img, x, y, h, color, bg, pc, nLen, font, justification, width ) );
}

IMAGE_NAMESPACE_END
