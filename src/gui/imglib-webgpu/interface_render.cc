// sack-gui imglib-webgpu — webgpu.render interface registration.
//
// Lets PSI's Frame() / Frame.Control() etc. open against the webgpu
// surface. Each "display" we hand back is actually a sub-image on the
// renderer's surface root, with a wrapping virtual_renderer struct that
// stores PSI's mouse/key/redraw/close callbacks for later dispatch.
// Implementations live in render_ops.cc.
//
// We do NOT register this as the default "render" interface — that would
// hijack OS window creation. PSI / native code that wants frames on the
// webgpu surface explicitly resolves "webgpu.render" via GetInterface,
// then calls SetControlInterface(...) on the PSI side so that subsequent
// frame opens go through us.

#include "local.h"

IMAGE_NAMESPACE

// Forward decls — bodies in render_ops.cc.
extern PRENDERER CPROC wgpu_OpenDisplaySizedAt     ( uint32_t attr, uint32_t w, uint32_t h, int32_t x, int32_t y );
extern PRENDERER CPROC wgpu_OpenDisplayAboveSizedAt( uint32_t attr, uint32_t w, uint32_t h, int32_t x, int32_t y, PRENDERER above );
extern void      CPROC wgpu_CloseDisplay           ( PRENDERER );
extern void      CPROC wgpu_UpdateDisplayPortionEx ( PRENDERER, int32_t x, int32_t y, uint32_t w, uint32_t h DBG_PASS );
extern void      CPROC wgpu_UpdateDisplayEx        ( PRENDERER DBG_PASS );
extern void      CPROC wgpu_GetDisplayPosition     ( PRENDERER, int32_t *x, int32_t *y, uint32_t *w, uint32_t *h );
extern void      CPROC wgpu_MoveDisplay            ( PRENDERER, int32_t x, int32_t y );
extern void      CPROC wgpu_MoveDisplayRel         ( PRENDERER, int32_t dx, int32_t dy );
extern void      CPROC wgpu_SizeDisplay            ( PRENDERER, uint32_t w, uint32_t h );
extern void      CPROC wgpu_MoveSizeDisplay        ( PRENDERER, int32_t x, int32_t y, int32_t w, int32_t h );
extern Image     CPROC wgpu_GetDisplayImage        ( PRENDERER );
extern void      CPROC wgpu_SetCloseHandler        ( PRENDERER, CloseCallback,  uintptr_t );
extern void      CPROC wgpu_SetMouseHandler        ( PRENDERER, MouseCallback,  uintptr_t );
extern void      CPROC wgpu_SetRedrawHandler       ( PRENDERER, RedrawCallback, uintptr_t );
extern void      CPROC wgpu_SetKeyboardHandler     ( PRENDERER, KeyProc,        uintptr_t );
extern void      CPROC wgpu_SetLoseFocusHandler    ( PRENDERER, LoseFocusCallback, uintptr_t );
extern LOGICAL   CPROC wgpu_HasFocus               ( PRENDERER );
extern LOGICAL   CPROC wgpu_DisplayIsValid         ( PRENDERER );
extern void      CPROC wgpu_HideDisplay            ( PRENDERER );
extern void      CPROC wgpu_RestoreDisplay         ( PRENDERER );
extern void      CPROC wgpu_GetMousePosition       ( int32_t *x, int32_t *y );
extern void      CPROC wgpu_GetDisplaySize         ( uint32_t *w, uint32_t *h );
extern void      CPROC wgpu_MarkDisplayUpdated     ( PRENDERER );

// Most other RENDER_INTERFACE slots are NULL — they cover things like OS
// window styles, drag-and-drop, sprite methods, touch / pen / focus
// management, calibration etc. PSI frame open + control draw + mouse
// dispatch shouldn't need them. NULL slots that turn out to be called
// will surface as crashes; fill in on demand.
//
// Memcpy-from-CPU-render strategy isn't useful here — CPU sack.render
// creates real OS windows, which is the opposite of what we want.
// Init the struct field-by-field instead.
static RENDER_INTERFACE WebgpuRenderInterface;
static int install_done = 0;

static void install_overrides( void )
{
	if( install_done )
		return;

	MemSet( &WebgpuRenderInterface, 0, sizeof( RENDER_INTERFACE ) );

	WebgpuRenderInterface._OpenDisplaySizedAt      = wgpu_OpenDisplaySizedAt;
	WebgpuRenderInterface._OpenDisplayAboveSizedAt = wgpu_OpenDisplayAboveSizedAt;
	WebgpuRenderInterface._CloseDisplay            = wgpu_CloseDisplay;
	WebgpuRenderInterface._UpdateDisplayPortionEx  = wgpu_UpdateDisplayPortionEx;
	WebgpuRenderInterface._UpdateDisplayEx         = wgpu_UpdateDisplayEx;
	WebgpuRenderInterface._GetDisplayPosition      = wgpu_GetDisplayPosition;
	WebgpuRenderInterface._MoveDisplay             = wgpu_MoveDisplay;
	WebgpuRenderInterface._MoveDisplayRel          = wgpu_MoveDisplayRel;
	WebgpuRenderInterface._SizeDisplay             = wgpu_SizeDisplay;
	WebgpuRenderInterface._MoveSizeDisplay         = wgpu_MoveSizeDisplay;
	WebgpuRenderInterface._GetDisplayImage         = wgpu_GetDisplayImage;
	WebgpuRenderInterface._SetCloseHandler         = wgpu_SetCloseHandler;
	WebgpuRenderInterface._SetMouseHandler         = wgpu_SetMouseHandler;
	WebgpuRenderInterface._SetRedrawHandler        = wgpu_SetRedrawHandler;
	WebgpuRenderInterface._SetKeyboardHandler      = wgpu_SetKeyboardHandler;
	WebgpuRenderInterface._SetLoseFocusHandler     = wgpu_SetLoseFocusHandler;
	WebgpuRenderInterface._HasFocus                = wgpu_HasFocus;
	WebgpuRenderInterface._DisplayIsValid          = wgpu_DisplayIsValid;
	WebgpuRenderInterface._HideDisplay             = wgpu_HideDisplay;
	WebgpuRenderInterface._RestoreDisplay          = wgpu_RestoreDisplay;
	WebgpuRenderInterface._GetMousePosition        = wgpu_GetMousePosition;
	WebgpuRenderInterface._GetDisplaySize          = wgpu_GetDisplaySize;
	WebgpuRenderInterface._MarkDisplayUpdated      = wgpu_MarkDisplayUpdated;

	install_done = 1;
}

static POINTER CPROC wgpu_GetRenderInterface( void )
{
	install_overrides();
	return &WebgpuRenderInterface;
}

static void CPROC wgpu_DropRenderInterface( POINTER p )
{
	(void)p;
}

PRIORITY_PRELOAD( WebgpuRenderRegisterInterface, IMAGE_PRELOAD_PRIORITY )
{
	RegisterInterface( "webgpu.render", wgpu_GetRenderInterface, wgpu_DropRenderInterface );
}

IMAGE_NAMESPACE_END
