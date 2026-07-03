// sack-gui imglib-webgpu — line draw ops.
//
// Lines are emitted as solid-pipeline tristrips. Axis-aligned lines
// (do_hline / do_vline) become 1-px-thick rectangles. do_line for a
// diagonal computes a perpendicular offset of half a pixel and emits
// a thin parallelogram.
//
// Per-image bundle dirty / op-list freshness semantics live in image_ops.cc;
// the helpers below assume an already-acquired per-image entry.

#include "local.h"

IMAGE_NAMESPACE

// The image_ops.cc equivalents are static; we just keep our own copies
// here rather than refactoring them into local.h.

static inline LOGICAL line_is_gpu_target( Image img )
{
	if( !img ) return FALSE;
	return ( img->flags & IF_FLAG_FINAL_RENDER ) && !( img->flags & IF_FLAG_IN_MEMORY );
}

static inline void line_px_to_ndc( Image dest, float x, float y, float *nx, float *ny )
{
	float W = (float)dest->real_width;
	float H = (float)dest->real_height;
	if( W <= 0 ) W = 1;
	if( H <= 0 ) H = 1;
	*nx = (x / W) * 2.0f - 1.0f;
	*ny = 1.0f - (y / H) * 2.0f;
}

static inline struct webgpu_op_list *line_fresh_ops( struct webgpu_per_image *pi )
{
	if( pi->needs_reset_ ) {
		if( pi->ops_ ) webgpu_op_list_reset( pi->ops_ );
		pi->needs_reset_ = 0;
	}
	if( !pi->ops_ )
		pi->ops_ = webgpu_op_list_new();
	return pi->ops_;
}

// ----------------------------------------------------------------------
// hline / vline — axis aligned, 1 pixel thick.
// ----------------------------------------------------------------------

void CPROC wgpu_do_hline( Image img, int32_t y, int32_t xfrom, int32_t xto, CDATA c )
{
	if( !line_is_gpu_target( img ) ) {
		PIMAGE_INTERFACE cpu = webgpu_image_get_cpu();
		if( cpu && cpu->_do_hline ) cpu->_do_hline( img, y, xfrom, xto, c );
		return;
	}
	if( xto < xfrom ) { int32_t t = xfrom; xfrom = xto; xto = t; }
	struct webgpu_per_image *pi = webgpu_per_image_get( img );
	if( !pi ) return;
	struct webgpu_op_list *ops = line_fresh_ops( pi );
	float x0, y0, x1, y1;
	line_px_to_ndc( img, (float)xfrom,       (float)y,       &x0, &y0 );
	line_px_to_ndc( img, (float)(xto + 1),   (float)(y + 1), &x1, &y1 );
	webgpu_op_list_solid_quad( ops, x0, y0, x1, y1, (uint32_t)c );
	pi->dirty_ = 1;
}

void CPROC wgpu_do_vline( Image img, int32_t x, int32_t yfrom, int32_t yto, CDATA c )
{
	if( !line_is_gpu_target( img ) ) {
		PIMAGE_INTERFACE cpu = webgpu_image_get_cpu();
		if( cpu && cpu->_do_vline ) cpu->_do_vline( img, x, yfrom, yto, c );
		return;
	}
	if( yto < yfrom ) { int32_t t = yfrom; yfrom = yto; yto = t; }
	struct webgpu_per_image *pi = webgpu_per_image_get( img );
	if( !pi ) return;
	struct webgpu_op_list *ops = line_fresh_ops( pi );
	float x0, y0, x1, y1;
	line_px_to_ndc( img, (float)x,        (float)yfrom,      &x0, &y0 );
	line_px_to_ndc( img, (float)(x + 1),  (float)(yto + 1),  &x1, &y1 );
	webgpu_op_list_solid_quad( ops, x0, y0, x1, y1, (uint32_t)c );
	pi->dirty_ = 1;
}

void CPROC wgpu_do_hlineAlpha( Image img, int32_t y, int32_t xfrom, int32_t xto, CDATA c )
{
	// Same path; the pipeline always alpha-blends.
	wgpu_do_hline( img, y, xfrom, xto, c );
}

void CPROC wgpu_do_vlineAlpha( Image img, int32_t x, int32_t yfrom, int32_t yto, CDATA c )
{
	wgpu_do_vline( img, x, yfrom, yto, c );
}

// ----------------------------------------------------------------------
// do_line — arbitrary slope. Emits a 1-px-wide oriented quad along the
// line vector, with vertices offset by (±dy/L, ∓dx/L) for the
// perpendicular direction. For purely axis-aligned cases this collapses
// to the same shape do_hline/do_vline would emit, so we don't special-case.
// ----------------------------------------------------------------------

void CPROC wgpu_do_line( Image img, int32_t x1, int32_t y1, int32_t x2, int32_t y2, CDATA c )
{
	if( !line_is_gpu_target( img ) ) {
		PIMAGE_INTERFACE cpu = webgpu_image_get_cpu();
		if( cpu && cpu->_do_line ) cpu->_do_line( img, x1, y1, x2, y2, c );
		return;
	}
	struct webgpu_per_image *pi = webgpu_per_image_get( img );
	if( !pi ) return;
	struct webgpu_op_list *ops = line_fresh_ops( pi );

	// Compute perpendicular pixel offset of half a pixel each side
	// → 1-pixel total thickness. Operates in pixel space; conversion
	// to NDC happens per-vertex below.
	float fx1 = (float)x1, fy1 = (float)y1;
	float fx2 = (float)x2, fy2 = (float)y2;
	float dx  = fx2 - fx1;
	float dy  = fy2 - fy1;
	float len = sqrtf( dx*dx + dy*dy );
	if( len < 1e-3f ) return;   // zero-length
	float nx = -dy / len * 0.5f; // perpendicular, half-pixel offset
	float ny =  dx / len * 0.5f;

	float corners_px[4][2] = {
		{ fx1 + nx, fy1 + ny },
		{ fx2 + nx, fy2 + ny },
		{ fx1 - nx, fy1 - ny },
		{ fx2 - nx, fy2 - ny },
	};
	float corners_ndc[4][2];
	for( int i = 0; i < 4; i++ )
		line_px_to_ndc( img, corners_px[i][0], corners_px[i][1],
		                &corners_ndc[i][0], &corners_ndc[i][1] );

	webgpu_op_list_solid_tristrip( ops, corners_ndc, (uint32_t)c );
	pi->dirty_ = 1;
}

void CPROC wgpu_do_lineAlpha( Image img, int32_t x1, int32_t y1, int32_t x2, int32_t y2, CDATA c )
{
	wgpu_do_line( img, x1, y1, x2, y2, c );
}

IMAGE_NAMESPACE_END
