// sack-gui imglib-webgpu — webgpu.render virtual renderer impl.
//
// A "virtual renderer" is a sub-image on the active webgpu surface plus
// the callbacks PSI installed via SetMouseHandler / SetRedrawHandler etc.
// PSI treats it as if it were a regular display; we treat it as a node in
// the surface image tree that our frame walker draws naturally.
//
// Mouse / key / redraw events arrive at the OS-level renderer (handled in
// sack_render_module.cc). For those to reach PSI's controls hosted on a
// virtual renderer, sack_render_module needs to call
// webgpu_render_dispatch_{mouse,key,redraw,close} before firing its own
// JS-side callbacks. Those dispatch helpers walk the virtual renderer
// list in z-order and fire each one's stored callback whose surface
// area contains the cursor (mouse), holds focus (key), is dirty (redraw),
// etc. A non-zero return from the dispatcher means "consumed — don't
// also fire the OS renderer's JS callback."
//
// This is the absolute-minimum subset. Many RENDER_INTERFACE entries are
// NULL on registration; PSI's frame path may surface NULL crashes as we
// exercise it. Fill in incrementally as needed.

#include "local.h"

IMAGE_NAMESPACE

// ----------------------------------------------------------------------
// Virtual renderer structure.
// ----------------------------------------------------------------------
struct webgpu_virtual_renderer
{
	struct webgpu_virtual_renderer *next;   // simple linked list for now;
	                                        // upgrade to z-order PLIST later

	Image surface;        // sub-image on the host surface root
	int32_t x, y;         // position on host
	uint32_t w, h;        // size

	LOGICAL valid;        // FALSE once CloseDisplay has been called
	LOGICAL visible;      // hide/show

	// PSI-installed event callbacks. uintptr_t psv accompanies each.
	CloseCallback     close_cb;     uintptr_t close_psv;
	MouseCallback     mouse_cb;     uintptr_t mouse_psv;
	RedrawCallback    redraw_cb;    uintptr_t redraw_psv;
	KeyProc           key_cb;       uintptr_t key_psv;
	LoseFocusCallback losefocus_cb; uintptr_t losefocus_psv;
};

static struct webgpu_virtual_renderer *g_virtual_list = NULL;
static int32_t  g_last_mouse_x = 0;
static int32_t  g_last_mouse_y = 0;
static uint32_t g_last_mouse_b = 0;

#define V(prend)  ((struct webgpu_virtual_renderer *)(prend))

// ----------------------------------------------------------------------
// Open / close.
// ----------------------------------------------------------------------

// The surface root is owned by driver.cc; we get it via the accessor.
extern Image webgpu_image_get_surface_root( void );

PRENDERER CPROC wgpu_OpenDisplaySizedAt( uint32_t attr, uint32_t w, uint32_t h,
                                         int32_t x, int32_t y )
{
	(void)attr;
	Image host = webgpu_image_get_surface_root();
	if( !host )
		return NULL;

	PIMAGE_INTERFACE pii = webgpu_image_get_cpu();
	if( !pii || !pii->_MakeSubImageEx )
		return NULL;

	struct webgpu_virtual_renderer *v =
		(struct webgpu_virtual_renderer *)AllocateEx( sizeof( *v ) DBG_SRC );
	MemSet( v, 0, sizeof( *v ) );

	v->x = x;  v->y = y;  v->w = w;  v->h = h;
	v->valid   = TRUE;
	v->visible = TRUE;
	v->surface = pii->_MakeSubImageEx( host, x, y, w, h DBG_SRC );
	if( v->surface ) {
		// Mark it as a GPU target so wgpu_BlatColor / wgpu_BlotImage etc.
		// record into bundles when PSI's controls draw onto it.
		v->surface->flags |=  IF_FLAG_FINAL_RENDER;
		v->surface->flags &= ~IF_FLAG_IN_MEMORY;
	}

	v->next = g_virtual_list;
	g_virtual_list = v;
	return (PRENDERER)v;
}

PRENDERER CPROC wgpu_OpenDisplayAboveSizedAt( uint32_t attr, uint32_t w, uint32_t h,
                                               int32_t x, int32_t y, PRENDERER above )
{
	(void)above;   // z-order plumbing deferred — we keep insertion order
	               // for now. When a real ListBox or modal frame surfaces
	               // the need we'll switch g_virtual_list to a PLIST and
	               // honor the "above" argument.
	return wgpu_OpenDisplaySizedAt( attr, w, h, x, y );
}

void CPROC wgpu_CloseDisplay( PRENDERER r )
{
	struct webgpu_virtual_renderer *v = V( r );
	if( !v ) return;
	if( v->close_cb )
		v->close_cb( v->close_psv );
	// Unlink from list.
	struct webgpu_virtual_renderer **pp = &g_virtual_list;
	while( *pp && *pp != v ) pp = &(*pp)->next;
	if( *pp ) *pp = v->next;
	// Release the sub-image via the CPU interface; that fires our
	// wgpu_UnmakeImageFileEx which cleans the GPU per-image side-table
	// entry.
	if( v->surface ) {
		PIMAGE_INTERFACE pii = webgpu_image_get_cpu();
		if( pii && pii->_UnmakeImageFileEx )
			pii->_UnmakeImageFileEx( v->surface DBG_SRC );
		v->surface = NULL;
	}
	v->valid = FALSE;
	Release( v );
}

// ----------------------------------------------------------------------
// Geometry.
// ----------------------------------------------------------------------

void CPROC wgpu_GetDisplayPosition( PRENDERER r, int32_t *x, int32_t *y,
                                     uint32_t *w, uint32_t *h )
{
	struct webgpu_virtual_renderer *v = V( r );
	if( !v ) return;
	if( x ) *x = v->x;
	if( y ) *y = v->y;
	if( w ) *w = v->w;
	if( h ) *h = v->h;
}

void CPROC wgpu_MoveDisplay( PRENDERER r, int32_t x, int32_t y )
{
	struct webgpu_virtual_renderer *v = V( r );
	if( !v || !v->surface ) return;
	v->x = x; v->y = y;
	v->surface->real_x = x; v->surface->real_y = y;
	// Frame walker will re-viewport on the next pass; mark dirty so the
	// re-record kicks in for any layout-relative content.
	webgpu_image_mark_dirty( v->surface );
}

void CPROC wgpu_MoveDisplayRel( PRENDERER r, int32_t dx, int32_t dy )
{
	struct webgpu_virtual_renderer *v = V( r );
	if( !v ) return;
	wgpu_MoveDisplay( r, v->x + dx, v->y + dy );
}

void CPROC wgpu_SizeDisplay( PRENDERER r, uint32_t w, uint32_t h )
{
	struct webgpu_virtual_renderer *v = V( r );
	if( !v || !v->surface ) return;
	v->w = w; v->h = h;
	// The sub-image's effective dimensions follow.
	v->surface->real_width  = w;
	v->surface->real_height = h;
	webgpu_image_mark_dirty( v->surface );
}

void CPROC wgpu_MoveSizeDisplay( PRENDERER r, int32_t x, int32_t y, int32_t w, int32_t h )
{
	wgpu_MoveDisplay( r, x, y );
	wgpu_SizeDisplay( r, (uint32_t)w, (uint32_t)h );
}

Image CPROC wgpu_GetDisplayImage( PRENDERER r )
{
	struct webgpu_virtual_renderer *v = V( r );
	return v ? v->surface : NULL;
}

// ----------------------------------------------------------------------
// Update / redraw bookkeeping. PSI calls these to request a redraw of
// (a portion of) its display; on our path that translates to "mark the
// retained bundle dirty so the walker re-records on next frame."
// ----------------------------------------------------------------------

void CPROC wgpu_UpdateDisplayPortionEx( PRENDERER r, int32_t x, int32_t y,
                                         uint32_t w, uint32_t h DBG_PASS )
{
	(void)x; (void)y; (void)w; (void)h;
	struct webgpu_virtual_renderer *v = V( r );
	if( v && v->surface )
		webgpu_image_mark_dirty( v->surface );
}

void CPROC wgpu_UpdateDisplayEx( PRENDERER r DBG_PASS )
{
	struct webgpu_virtual_renderer *v = V( r );
	if( v && v->surface )
		webgpu_image_mark_dirty( v->surface );
}

void CPROC wgpu_MarkDisplayUpdated( PRENDERER r )
{
	struct webgpu_virtual_renderer *v = V( r );
	if( v && v->surface )
		webgpu_image_mark_dirty( v->surface );
}

// ----------------------------------------------------------------------
// Event callback storage.
// ----------------------------------------------------------------------

void CPROC wgpu_SetCloseHandler   ( PRENDERER r, CloseCallback     cb, uintptr_t psv )
{ struct webgpu_virtual_renderer *v = V( r ); if( v ) { v->close_cb     = cb; v->close_psv     = psv; } }

void CPROC wgpu_SetMouseHandler   ( PRENDERER r, MouseCallback     cb, uintptr_t psv )
{ struct webgpu_virtual_renderer *v = V( r ); if( v ) { v->mouse_cb     = cb; v->mouse_psv     = psv; } }

void CPROC wgpu_SetRedrawHandler  ( PRENDERER r, RedrawCallback    cb, uintptr_t psv )
{ struct webgpu_virtual_renderer *v = V( r ); if( v ) { v->redraw_cb    = cb; v->redraw_psv    = psv; } }

void CPROC wgpu_SetKeyboardHandler( PRENDERER r, KeyProc           cb, uintptr_t psv )
{ struct webgpu_virtual_renderer *v = V( r ); if( v ) { v->key_cb       = cb; v->key_psv       = psv; } }

void CPROC wgpu_SetLoseFocusHandler( PRENDERER r, LoseFocusCallback cb, uintptr_t psv )
{ struct webgpu_virtual_renderer *v = V( r ); if( v ) { v->losefocus_cb = cb; v->losefocus_psv = psv; } }

// ----------------------------------------------------------------------
// State queries / visibility.
// ----------------------------------------------------------------------

LOGICAL CPROC wgpu_HasFocus( PRENDERER r )
{
	// Simplification: report focus if the host renderer has focus. Per-
	// virtual-renderer focus tracking comes later when we add a focus
	// chain mechanism similar to OS window focus.
	(void)r;
	return TRUE;
}

LOGICAL CPROC wgpu_DisplayIsValid( PRENDERER r )
{
	struct webgpu_virtual_renderer *v = V( r );
	return v && v->valid;
}

void CPROC wgpu_HideDisplay( PRENDERER r )
{
	struct webgpu_virtual_renderer *v = V( r );
	if( v ) {
		v->visible = FALSE;
		if( v->surface ) {
			v->surface->flags |= IF_FLAG_HIDDEN;
			webgpu_image_mark_dirty( v->surface );
		}
	}
}

void CPROC wgpu_RestoreDisplay( PRENDERER r )
{
	struct webgpu_virtual_renderer *v = V( r );
	if( v ) {
		v->visible = TRUE;
		if( v->surface ) {
			v->surface->flags &= ~IF_FLAG_HIDDEN;
			webgpu_image_mark_dirty( v->surface );
		}
	}
}

// ----------------------------------------------------------------------
// Mouse pointer.
// ----------------------------------------------------------------------

void CPROC wgpu_GetMousePosition( int32_t *x, int32_t *y )
{
	if( x ) *x = g_last_mouse_x;
	if( y ) *y = g_last_mouse_y;
}

void CPROC wgpu_GetDisplaySize( uint32_t *w, uint32_t *h )
{
	if( w ) *w = l.surface_w_;
	if( h ) *h = l.surface_h_;
}

// ----------------------------------------------------------------------
// External dispatchers — called by sack_render_module.cc's doMouse /
// doKey / doRedraw before its own JS-side callback fires. Returns
// non-zero if a virtual renderer consumed the event.
// ----------------------------------------------------------------------

IMAGE_NAMESPACE_END

extern "C" int webgpu_render_dispatch_mouse( int32_t x, int32_t y, uint32_t b )
{
	using namespace sack::image;
	g_last_mouse_x = x; g_last_mouse_y = y; g_last_mouse_b = b;
	int consumed = 0;
	for( struct webgpu_virtual_renderer *v = g_virtual_list; v; v = v->next ) {
		if( !v->valid || !v->visible || !v->mouse_cb ) continue;
		// Hit test: cursor inside the virtual renderer's host-space rect.
		if( x < v->x || y < v->y ) continue;
		if( x >= v->x + (int32_t)v->w || y >= v->y + (int32_t)v->h ) continue;
		// Translate to renderer-local coords for the callback.
		int32_t lx = x - v->x;
		int32_t ly = y - v->y;
		int r = v->mouse_cb( v->mouse_psv, lx, ly, b );
		if( r ) { consumed = 1; break; }
	}
	return consumed;
}

extern "C" int webgpu_render_dispatch_key( uint32_t key )
{
	using namespace sack::image;
	for( struct webgpu_virtual_renderer *v = g_virtual_list; v; v = v->next ) {
		if( !v->valid || !v->visible || !v->key_cb ) continue;
		int r = v->key_cb( v->key_psv, key );
		if( r ) return 1;
	}
	return 0;
}

extern "C" void webgpu_render_dispatch_redraw( void )
{
	using namespace sack::image;
	for( struct webgpu_virtual_renderer *v = g_virtual_list; v; v = v->next ) {
		if( !v->valid || !v->visible || !v->redraw_cb ) continue;
		v->redraw_cb( v->redraw_psv, (PRENDERER)v );
	}
}
