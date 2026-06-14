// sack-gui imglib-webgpu — op-list recording and render-bundle encoding.
//
// PSI draws ops into a per-subimage webgpu_op_list (CPU-side vertex pools
// + a batch list). The frame walker encodes the op-list into a
// WGPURenderBundle that's then replayed each frame until the subimage
// gets dirtied again.
//
// Vertex formats — packed to match shaders.wgsl exactly:
//
//   SOLID:     pos[2] (8) + color (4)              = 12 B / vert
//   TEXTURED:  pos[2] (8) + uv[2]  (8) + tint (4)  = 20 B / vert
//   MULTI:     pos[2] (8) + uv[2]  (8) + 3×u32(12) = 28 B / vert
//
// Tristrip topology, 4 verts per quad. We don't deduplicate or strip-merge
// across quads — degenerate-bridge merging is a later optimization.

#include "local.h"

IMAGE_NAMESPACE

namespace {

// Tiny dynamic byte buffer. Mirrors shader_buffer from puregl2 but with
// untyped storage so each pipeline can keep its own packed format.
struct vert_pool
{
	uint8_t *data;
	size_t   used;
	size_t   avail;
};

static void vert_pool_init( struct vert_pool *p )
{
	p->data = NULL;
	p->used = 0;
	p->avail = 0;
}

static void vert_pool_reserve( struct vert_pool *p, size_t need )
{
	if( p->used + need <= p->avail )
		return;
	size_t new_size = p->avail ? p->avail * 2 : 256;
	while( p->used + need > new_size )
		new_size *= 2;
	uint8_t *nb = (uint8_t *)AllocateEx( new_size DBG_SRC );
	if( p->data ) {
		MemCpy( nb, p->data, p->used );
		Release( p->data );
	}
	p->data  = nb;
	p->avail = new_size;
}

static uint32_t vert_pool_append( struct vert_pool *p, const void *src, size_t bytes )
{
	vert_pool_reserve( p, bytes );
	uint32_t offset = (uint32_t)p->used;
	MemCpy( p->data + p->used, src, bytes );
	p->used += bytes;
	return offset;
}

static void vert_pool_free( struct vert_pool *p )
{
	if( p->data ) Release( p->data );
	p->data = NULL; p->used = 0; p->avail = 0;
}

enum batch_kind { BATCH_SOLID, BATCH_TEXTURED, BATCH_MULTI, BATCH_NORMAL };

struct draw_batch
{
	enum batch_kind kind;
	uint32_t        first_vert;    // index (not byte offset) into pool
	uint32_t        vert_count;
	Image           source_image;  // for batches that need per-source bind groups
};

} // anon

// Public opaque type.
struct webgpu_op_list
{
	struct vert_pool solid;
	struct vert_pool textured;   // shared by textured + normal — same vertex layout
	struct vert_pool multi;

	struct draw_batch *batches;
	size_t             batch_count;
	size_t             batch_avail;
};

// ----------------------------------------------------------------------
// Lifecycle
// ----------------------------------------------------------------------

struct webgpu_op_list *webgpu_op_list_new( void )
{
	struct webgpu_op_list *ops = (struct webgpu_op_list *)AllocateEx( sizeof( *ops ) DBG_SRC );
	MemSet( ops, 0, sizeof( *ops ) );
	return ops;
}

void webgpu_op_list_reset( struct webgpu_op_list *ops )
{
	if( !ops ) return;
	ops->solid.used    = 0;
	ops->textured.used = 0;
	ops->multi.used    = 0;
	ops->batch_count   = 0;
}

void webgpu_op_list_release( struct webgpu_op_list *ops )
{
	if( !ops ) return;
	vert_pool_free( &ops->solid );
	vert_pool_free( &ops->textured );
	vert_pool_free( &ops->multi );
	if( ops->batches ) Release( ops->batches );
	Release( ops );
}

// ----------------------------------------------------------------------
// Batch management
// ----------------------------------------------------------------------

static struct draw_batch *get_or_extend_batch( struct webgpu_op_list *ops,
                                                enum batch_kind kind,
                                                Image source,
                                                uint32_t first_vert,
                                                uint32_t new_verts )
{
	// If the last batch matches (same kind, same source), extend it.
	if( ops->batch_count > 0 ) {
		struct draw_batch *last = &ops->batches[ ops->batch_count - 1 ];
		if( last->kind == kind && last->source_image == source ) {
			last->vert_count += new_verts;
			return last;
		}
	}
	// New batch.
	if( ops->batch_count == ops->batch_avail ) {
		size_t na = ops->batch_avail ? ops->batch_avail * 2 : 8;
		struct draw_batch *nb = (struct draw_batch *)AllocateEx( na * sizeof( *nb ) DBG_SRC );
		if( ops->batches ) {
			MemCpy( nb, ops->batches, ops->batch_count * sizeof( *nb ) );
			Release( ops->batches );
		}
		ops->batches     = nb;
		ops->batch_avail = na;
	}
	struct draw_batch *b = &ops->batches[ ops->batch_count++ ];
	b->kind         = kind;
	b->first_vert   = first_vert;
	b->vert_count   = new_verts;
	b->source_image = source;
	return b;
}

// ----------------------------------------------------------------------
// Append API
// ----------------------------------------------------------------------

// Vertex emission: pipelines use TriangleList (NOT TriangleStrip), so each
// quad becomes 6 verts forming two triangles. Input is in tristrip-style
// corner order (TL, TR, BL, BR); we expand to triangle-list:
//   Triangle 1: TL, TR, BL
//   Triangle 2: TR, BR, BL
// This avoids the strip-bridging artefact that draws spurious triangles
// between adjacent quads sharing a batch.

// Internal: pack a textured-format vertex (used by both textured and
// normal batches — they share vertex layout). 6 verts total per quad.
static void append_textured_verts( struct vert_pool *pool,
                                    const float verts[4][2], const float uvs[4][2],
                                    uint32_t tint, uint32_t *out_first_vert )
{
	struct V { float pos[2]; float uv[2]; uint32_t tint; };
	V src[4];
	for( int i = 0; i < 4; i++ ) {
		src[i].pos[0] = verts[i][0];
		src[i].pos[1] = verts[i][1];
		src[i].uv[0]  = uvs[i][0];
		src[i].uv[1]  = uvs[i][1];
		src[i].tint   = tint;
	}
	// TL=0, TR=1, BL=2, BR=3.
	V tri[6] = { src[0], src[1], src[2],   src[1], src[3], src[2] };
	*out_first_vert = (uint32_t)( pool->used / sizeof( V ) );
	vert_pool_append( pool, tri, sizeof( tri ) );
}

void webgpu_op_list_solid_quad( struct webgpu_op_list *ops,
                                 float x0, float y0, float x1, float y1,
                                 uint32_t color )
{
	if( !ops ) return;
	struct V { float pos[2]; uint32_t color; };
	// TL, TR, BL, BR
	V tl = { { x0, y0 }, color };
	V tr = { { x1, y0 }, color };
	V bl = { { x0, y1 }, color };
	V br = { { x1, y1 }, color };
	V tri[6] = { tl, tr, bl,   tr, br, bl };
	const size_t bytes_per_vert = sizeof( V );  // 12
	uint32_t first_vert = (uint32_t)( ops->solid.used / bytes_per_vert );
	vert_pool_append( &ops->solid, tri, sizeof( tri ) );
	get_or_extend_batch( ops, BATCH_SOLID, NULL, first_vert, 6 );
}

void webgpu_op_list_solid_tristrip( struct webgpu_op_list *ops,
                                     const float verts_in[4][2],
                                     uint32_t color )
{
	if( !ops ) return;
	struct V { float pos[2]; uint32_t color; };
	V src[4] = {
		{ { verts_in[0][0], verts_in[0][1] }, color },
		{ { verts_in[1][0], verts_in[1][1] }, color },
		{ { verts_in[2][0], verts_in[2][1] }, color },
		{ { verts_in[3][0], verts_in[3][1] }, color },
	};
	V tri[6] = { src[0], src[1], src[2],   src[1], src[3], src[2] };
	const size_t bytes_per_vert = sizeof( V );
	uint32_t first_vert = (uint32_t)( ops->solid.used / bytes_per_vert );
	vert_pool_append( &ops->solid, tri, sizeof( tri ) );
	get_or_extend_batch( ops, BATCH_SOLID, NULL, first_vert, 6 );
}

void webgpu_op_list_textured_quad( struct webgpu_op_list *ops,
                                    const float verts[4][2],
                                    const float uvs[4][2],
                                    uint32_t tint,
                                    Image source )
{
	if( !ops || !source ) return;
	uint32_t first_vert;
	append_textured_verts( &ops->textured, verts, uvs, tint, &first_vert );
	get_or_extend_batch( ops, BATCH_TEXTURED, source, first_vert, 6 );
}

void webgpu_op_list_normal_quad( struct webgpu_op_list *ops,
                                  const float verts[4][2],
                                  const float uvs[4][2],
                                  uint32_t tint,
                                  Image source )
{
	if( !ops || !source ) return;
	uint32_t first_vert;
	append_textured_verts( &ops->textured, verts, uvs, tint, &first_vert );
	get_or_extend_batch( ops, BATCH_NORMAL, source, first_vert, 6 );
}

void webgpu_op_list_multi_quad( struct webgpu_op_list *ops,
                                 const float verts[4][2],
                                 const float uvs[4][2],
                                 uint32_t shadeR, uint32_t shadeG, uint32_t shadeB,
                                 Image source )
{
	if( !ops || !source ) return;
	struct V { float pos[2]; float uv[2]; uint32_t shadeR, shadeG, shadeB; };
	V src[4];
	for( int i = 0; i < 4; i++ ) {
		src[i].pos[0] = verts[i][0];
		src[i].pos[1] = verts[i][1];
		src[i].uv[0]  = uvs[i][0];
		src[i].uv[1]  = uvs[i][1];
		src[i].shadeR = shadeR;
		src[i].shadeG = shadeG;
		src[i].shadeB = shadeB;
	}
	V tri[6] = { src[0], src[1], src[2],   src[1], src[3], src[2] };
	const size_t bytes_per_vert = sizeof( V );  // 28
	uint32_t first_vert = (uint32_t)( ops->multi.used / bytes_per_vert );
	vert_pool_append( &ops->multi, tri, sizeof( tri ) );
	get_or_extend_batch( ops, BATCH_MULTI, source, first_vert, 6 );
}

// ----------------------------------------------------------------------
// Encode to render bundle.
// ----------------------------------------------------------------------
//
// Steps:
//   1. Upload each populated vertex pool into a WGPUBuffer.
//   2. Create a WGPURenderBundleEncoder compatible with the surface format.
//   3. For each batch:
//        - set the matching pipeline if changed
//        - set the texture/sampler bind group if changed
//        - set the vertex buffer if pipeline changed
//        - draw(vert_count, 1, first_vert, 0) — tristrip topology
//      The pipeline objects are owned by the driver (see driver.cc).
//   4. wgpuRenderBundleEncoderFinish — return.
//
// First cut: the buffer-upload + encode loop is structured, but the
// pipeline-getter calls in driver.cc still return NULL until
// driver_pipelines.cc lands. That means this function currently exits
// early at the pipeline check and returns NULL — the dirty walker logs
// a one-shot warning and the bundle slot stays empty.

WGPURenderBundle webgpu_op_list_record( struct webgpu_op_list *ops,
                                         WGPUTextureFormat color_format )
{
	if( !ops || ops->batch_count == 0 )
		return NULL;
	if( !webgpu_image_ensure_init() )
		return NULL;

	WGPURenderPipeline pipe_solid = webgpu_image_get_pipe_solid();
	if( !pipe_solid )
		return NULL;
	WGPURenderPipeline pipe_textured = webgpu_image_get_pipe_textured();
	WGPURenderPipeline pipe_multi    = webgpu_image_get_pipe_multi();
	WGPURenderPipeline pipe_normal   = webgpu_image_get_pipe_normal();

	// Upload vert pools. Each becomes a transient WGPUBuffer that lives
	// for the lifetime of the bundle (Dawn keeps referenced resources
	// alive through bundle replay).
	auto make_vb = []( const struct vert_pool *p ) -> WGPUBuffer {
		if( p->used == 0 ) return NULL;
		WGPUBufferDescriptor bd = {};
		bd.size  = p->used;
		bd.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
		WGPUBuffer buf = wgpuDeviceCreateBuffer( l.device_, &bd );
		wgpuQueueWriteBuffer( l.queue_, buf, 0, p->data, p->used );
		return buf;
	};
	WGPUBuffer vb_solid    = make_vb( &ops->solid );
	WGPUBuffer vb_textured = make_vb( &ops->textured );  // shared by textured+normal
	WGPUBuffer vb_multi    = make_vb( &ops->multi );

	WGPURenderBundleEncoderDescriptor bed = {};
	bed.colorFormatCount   = 1;
	bed.colorFormats       = &color_format;
	// Bundle must match the executing pass's attachment state. If the
	// binding has told us there's a depth-stencil attachment on the
	// surface pass (e.g. three.js 3D content sharing the pass with our
	// HUD), we record the bundle with that format and promise to not
	// write depth — pure 2D content has no depth-touching ops.
	bed.depthStencilFormat = l.surface_depth_format_;
	if( l.surface_depth_format_ != WGPUTextureFormat_Undefined ) {
		bed.depthReadOnly   = true;
	}
	bed.sampleCount        = 1;
	WGPURenderBundleEncoder enc = wgpuDeviceCreateRenderBundleEncoder( l.device_, &bed );

	// Globals bind group at @group(0) is shared by every pipeline; bind once.
	wgpuRenderBundleEncoderSetBindGroup( enc, 0, l.globals_group_, 0, NULL );

	enum batch_kind last_kind = BATCH_SOLID;
	Image           last_src  = NULL;
	int             pipeline_set = 0;

	for( size_t i = 0; i < ops->batch_count; i++ ) {
		struct draw_batch *b = &ops->batches[i];
		if( !pipeline_set || b->kind != last_kind ) {
			WGPURenderPipeline p = NULL;
			WGPUBuffer         vb = NULL;
			switch( b->kind ) {
			case BATCH_SOLID:    p = pipe_solid;    vb = vb_solid;    break;
			case BATCH_TEXTURED: p = pipe_textured; vb = vb_textured; break;
			case BATCH_MULTI:    p = pipe_multi;    vb = vb_multi;    break;
			case BATCH_NORMAL:   p = pipe_normal;   vb = vb_textured; break;
			}
			if( !p ) continue;
			wgpuRenderBundleEncoderSetPipeline( enc, p );
			if( vb )
				wgpuRenderBundleEncoderSetVertexBuffer( enc, 0, vb, 0, WGPU_WHOLE_SIZE );
			last_kind    = b->kind;
			last_src     = NULL;   // force per-source bind group rebind
			pipeline_set = 1;
		}
		if( b->kind != BATCH_SOLID && b->source_image != last_src ) {
			WGPUBindGroup bg = ( b->kind == BATCH_NORMAL )
				? webgpu_image_get_normal_bg ( b->source_image )
				: webgpu_image_get_diffuse_bg( b->source_image );
			if( bg )
				wgpuRenderBundleEncoderSetBindGroup( enc, 1, bg, 0, NULL );
			last_src = b->source_image;
		}
		wgpuRenderBundleEncoderDraw( enc, b->vert_count, 1, b->first_vert, 0 );
	}

	WGPURenderBundleDescriptor bdsc = {};
	WGPURenderBundle bundle = wgpuRenderBundleEncoderFinish( enc, &bdsc );
	wgpuRenderBundleEncoderRelease( enc );

	if( vb_solid )    wgpuBufferRelease( vb_solid );
	if( vb_textured ) wgpuBufferRelease( vb_textured );
	if( vb_multi )    wgpuBufferRelease( vb_multi );

	// Reset so the caller can append the next frame's ops fresh. The
	// bundle retains its own copies of everything it needs.
	webgpu_op_list_reset( ops );

	return bundle;
}

IMAGE_NAMESPACE_END
