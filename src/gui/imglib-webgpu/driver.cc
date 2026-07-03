// sack-gui imglib-webgpu — driver global state + lazy initialization.
//
// Lazy first-use init resolves:
//   1. The CPU sack.image / sack.image++ interface (always available
//      once the imglib has registered, but order vs. our PRIORITY_PRELOAD
//      is not guaranteed — hence lazy).
//   2. The WebGPU device/queue from the JS binding (only available after
//      JS has called sack.gpu.requestAdapter().requestDevice()).
//
// Pipelines, samplers, palette buffer are created on first need.

#define IMAGE_LIBRARY_SOURCE_MAIN
#include "local.h"
#include "../webgpu/webgpu_module.h"   // webgpu_get_instance, future accessors

IMAGE_NAMESPACE

// Module global. Static-zero initialized, so cpu_image_interface starts NULL
// and we treat that as "not yet resolved."
//struct webgpu_image_driver_state webgpu_image_driver;

// External accessors we expect the webgpu_module to expose. Declared here
// so we can use them; the implementation slot in webgpu_module.cc returns
// the most-recently-created GPUDevice's WGPUDevice handle (and its queue).
//
// If those accessors aren't wired up yet at link time, this TU still
// builds — the symbols are weak references via the explicit checks below.
extern "C" WGPUDevice webgpu_get_active_device( void );
extern "C" WGPUQueue  webgpu_get_active_queue( void );

PIMAGE_INTERFACE webgpu_image_get_cpu( void )
{
	if( !l.cpu_image_interface )
	{
		// Try C++ name first (we're in a C++ TU); fall back to the C name.
		l.cpu_image_interface = (PIMAGE_INTERFACE)GetInterface( "sack.image++" );
		if( !l.cpu_image_interface )
			l.cpu_image_interface = (PIMAGE_INTERFACE)GetInterface( "sack.image" );
	}
	return l.cpu_image_interface;
}

static PIMAGE_3D_INTERFACE webgpu_image_get_cpu_3d( void )
{
	if( !l.cpu_image_3d_interface )
	{
		l.cpu_image_3d_interface = (PIMAGE_3D_INTERFACE)GetInterface( "sack.image.3d++" );
		if( !l.cpu_image_3d_interface )
			l.cpu_image_3d_interface = (PIMAGE_3D_INTERFACE)GetInterface( "sack.image.3d" );
		if( !l.cpu_image_3d_interface )
			l.cpu_image_3d_interface = (PIMAGE_3D_INTERFACE)GetInterface( "image.3d" );
	}
	return l.cpu_image_3d_interface;
}

LOGICAL webgpu_image_ensure_init( void )
{
	if( !l.flags_initialized_ )
	{
		webgpu_image_get_cpu();
		webgpu_image_get_cpu_3d();
		// scale mirrors puregl2 driver's profile-driven scale (defaults 0.1).
		l.scale = (RCOORD)SACK_GetProfileInt( GetProgramName(), "SACK/Image Library/Scale", 10 );
		if( l.scale == 0.0 )
			l.scale = 1.0;
		else
			l.scale = 1.0f / l.scale;
		l.flags_initialized_ = 1;
	}

	// Device acquisition is independent and can become available later.
	if( !l.device_ )
	{
		l.device_ = webgpu_get_active_device();
		l.queue_  = webgpu_get_active_queue();
	}

	return ( l.cpu_image_interface != NULL ) && ( l.device_ != NULL );
}

// Pipeline lazy creation lives in driver_pipelines.cc — implementations
// of webgpu_image_get_pipe_solid/textured/multi/normal and the setter
// API (light direction, palette entries, normal-map binding, etc.).

// ----------------------------------------------------------------------
// Per-image state allocator.
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Per-image state — side-table.
//
// Keyed by Image* (cast to uintptr_t). BT_OPT_NODUPLICATES with the
// default key compare (unsigned integer) is exactly what we want.
// Tree is process-global; entries are removed by webgpu_per_image_release,
// which the wgpu_UnmakeImageFileEx override invokes before the CPU side
// frees the ImageFile.
// ----------------------------------------------------------------------

static PTREEROOT g_per_image_tree;

static void ensure_per_image_tree( void )
{
	if( !g_per_image_tree )
		g_per_image_tree = CreateBinaryTree();
}

struct webgpu_per_image *webgpu_per_image_get( Image img )
{
	if( !img )
		return NULL;
	ensure_per_image_tree();
	uintptr_t key = (uintptr_t)img;
	struct webgpu_per_image *pi =
		(struct webgpu_per_image *)FindInBinaryTree( g_per_image_tree, key );
	if( !pi )
	{			
		pi = (struct webgpu_per_image *)AllocateEx( sizeof( *pi ) DBG_SRC );
		MemSet( pi, 0, sizeof( *pi ) );
		AddBinaryNode( g_per_image_tree, pi, key );
	}
	return pi;
}

void webgpu_per_image_release( Image img )
{
	if( !img || !g_per_image_tree )
		return;
	uintptr_t key = (uintptr_t)img;
	struct webgpu_per_image *pi =
		(struct webgpu_per_image *)FindInBinaryTree( g_per_image_tree, key );
	if( !pi )
		return;
	if( pi->bundle_ )      wgpuRenderBundleRelease( pi->bundle_ );
	if( pi->diffuse_bg_ )  wgpuBindGroupRelease( pi->diffuse_bg_ );
	if( pi->normal_bg_ )   wgpuBindGroupRelease( pi->normal_bg_ );
	if( pi->src_view_ )    wgpuTextureViewRelease( pi->src_view_ );
	if( pi->src_texture_ ) wgpuTextureRelease( pi->src_texture_ );
	if( pi->ops_ )         webgpu_op_list_release( pi->ops_ );
	RemoveBinaryNode( g_per_image_tree, pi, key );
	Release( pi );
}

void webgpu_image_mark_dirty( Image img )
{
	struct webgpu_per_image *pi = webgpu_per_image_get( img );
	if( pi )
		pi->dirty_ = 1;
}

// Recursive tree walker. Walks PSI image tree from `img`, accumulating
// absolute surface-space position via parent_abs_x/y. For each FINAL_RENDER
// subimage:
//   - if dirty, re-record bundle from current op-list under the per-image lock
//   - if a bundle exists, set the pass viewport to the subimage's absolute
//     rect and executeBundles
//
// Z-order is "youngest first" by walking pChild → pElder, which matches PSI's
// older-behind-newer convention. (puregl2 does the same.)
static void draw_image_recursive( WGPURenderPassEncoder pass, Image img,
                                   int32_t parent_abs_x, int32_t parent_abs_y )
{
	if( !img )
		return;
	if( img->flags & IF_FLAG_HIDDEN )
		return;

	int32_t abs_x = parent_abs_x + img->real_x;
	int32_t abs_y = parent_abs_y + img->real_y;

	if( ( img->flags & IF_FLAG_FINAL_RENDER ) && !( img->flags & IF_FLAG_IN_MEMORY ) )
	{
		struct webgpu_per_image *pi = webgpu_per_image_get( img );
		if( pi )
		{
			// Re-record bundle when dirty, OR when global generation
			// counters (depth format, atlas, palette) have moved since
			// this bundle was recorded — bundles need to be encoded
			// against the *current* attachment state and uniform layout.
			LOGICAL gen_stale = pi->bundle_
				&& ( pi->bundle_depth_generation_ != l.depth_format_generation_ );
			if( ( pi->dirty_ || gen_stale ) && pi->ops_ ) {
				while( LockedExchange( &pi->bundle_lock_, 1 ) )
					Relinquish();
				if( pi->bundle_ ) {
					wgpuRenderBundleRelease( pi->bundle_ );
					pi->bundle_ = NULL;
				}
				pi->bundle_ = webgpu_op_list_record( pi->ops_, l.surface_format_ );
				pi->bundle_atlas_generation_   = l.font_atlas_generation_;
				pi->bundle_palette_generation_ = l.palette_generation_;
				pi->bundle_depth_generation_   = l.depth_format_generation_;
				pi->dirty_       = 0;
				pi->needs_reset_ = 1;   // next draw op clears ops_ before append
				pi->bundle_lock_ = 0;
			}
			if( pi->bundle_ ) {
				wgpuRenderPassEncoderSetViewport( pass,
				    (float)abs_x, (float)abs_y,
				    (float)img->real_width, (float)img->real_height,
				    0.0f, 1.0f );
				wgpuRenderPassEncoderExecuteBundles( pass, 1, &pi->bundle_ );
			}
		}
	}

	// Recurse children — pChild is the youngest. Walking the pElder chain
	// from pChild visits youngest → oldest, which matches the puregl2
	// draw order (oldest sub-images get the youngest's overlay last in
	// the pass already; render bundles don't reorder).
	for( Image c = img->pChild; c; c = c->pElder )
		draw_image_recursive( pass, c, abs_x, abs_y );
}

void webgpu_image_execute_into_pass( WGPURenderPassEncoder pass, Image root )
{
	if( !pass || !root )
		return;
	if( !webgpu_image_ensure_init() )
		return;
	if( !l.surface_w_ || !l.surface_h_ )
		return;   // can't viewport without surface dimensions
	// Subtract the root's own real_x/real_y from the starting parent abs
	// so the root viewport always lands at (0,0) of the surface. On Win32
	// sack updates the display image's real_x/real_y to track the window's
	// screen position; without this neutralization the rendered content
	// would translate every time the user drags the window.
	draw_image_recursive( pass, root, -root->real_x, -root->real_y );
}

// Surface-root tracking. The binding's render-path hook
// (GPURenderPassEncoder.end on a surface-targeting pass) calls our
// extern "C" dispatcher; the dispatcher walks whichever root has been
// designated by sack-side code that connects an Image to a canvas.
static Image g_surface_root = NULL;

void webgpu_image_set_surface_root( void *root )
{
	g_surface_root = (Image)root;
}

// Read-only accessor for code outside this TU (render_ops.cc) that needs
// to know what the surface root is.
Image webgpu_image_get_surface_root( void )
{
	return g_surface_root;
}

IMAGE_NAMESPACE_END

// C-linkage shim. Lives outside IMAGE_NAMESPACE so the symbol name is
// `webgpu_image_dispatch_surface_pass` flat — easy for the binding to
// resolve. NULL root means no Image has been bound to a canvas yet;
// silently no-op.
//
// IMAGE_NAMESPACE expands to `namespace sack { namespace image {`, so
// our internals live in sack::image and we pull them in here.
extern "C" void webgpu_image_dispatch_surface_pass( WGPURenderPassEncoder pass )
{
	if( !pass )
		return;
	sack::image::Image root = sack::image::g_surface_root;
	if( !root )
		return;
	sack::image::webgpu_image_execute_into_pass( pass, root );
}
