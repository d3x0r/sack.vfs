#if defined( _MSC_VER )
#  pragma warning( disable: 4251 )
#endif

#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>
#include <uv.h>

#if defined( _MSC_VER )
#  pragma warning( default: 4251 )
#endif

//#include <nan.h>
//#include <varargs.h>
#include <stdarg.h>
#define NO_AUTO_VECTLIB_NAMES
#ifndef NO_FILEOP_ALIAS
#define NO_FILEOP_ALIAS
#endif

#define USE_RENDER_INTERFACE g.pdi
#define USE_IMAGE_INTERFACE g.pii
#ifdef SACK_CORE
#include <stdhdrs.h>
#include <filesys.h>
#include <sack_vfs.h>
#include <pssql.h>
#include <deadstart.h>
#include <translation.h>

#include <image.h>
#include <render.h>
#include <render3d.h>
#include <psi.h>
#else
#include "../../../deps/sack/sack.h"
#include "../../../deps/sack/sack_psi.h"
#include "../../../deps/sack/sack_imglib.h"
#include "../../../deps/sack/sack_vidlib.h"
#endif


#undef New

using namespace v8;

#include "sack_image_module.h"
#include "sack_render_module.h"

#include "sack_psi_module.h" // root sources, does Node root init entry point

#include "sack_intershell_module.h"

#include "sack_vulkan_module.h"


enum GUI_eventType {
	Event_Init,
	Event_Mouse,
	Event_Render_Mouse,
	Event_Render_Key,
	Event_Render_Draw,

	Event_Intershell_CreateControl,
	Event_Intershell_CreateButton,
	Event_Intershell_CreateCustomControl,
	Event_Intershell_ButtonClick,
	Event_Intershell_ButtonSave,
	Event_Intershell_ButtonLoad,
	Event_Intershell_ControlSave,
	Event_Intershell_ControlLoad,
	Event_Intershell_Quit,

	Event_Intershell_Control_Destroy,
	Event_Intershell_Control_Mouse,
	Event_Intershell_Control_Key,
	Event_Intershell_Control_Draw,


	Event_Control_Create,
	Event_Control_Mouse,
	Event_Control_Key,
	Event_Control_Draw,
	Event_Control_Move,
	Event_Control_Resize,
	Event_Control_Load,
	Event_Control_Destroy,

	Event_Frame_Ok,
	Event_Frame_Cancel,
	Event_Frame_Abort,

	/* button event*/
	Event_Control_ButtonClick,
	/* console events*/
	Event_Control_ConsoleInput,
	Event_Control_Close_Loop,
	/* listbox Events */
	Event_Listbox_Selected,
	Event_Listbox_DoubleClick,
	/* listbox item Events */
	Event_Listbox_Item_Opened,
	/* menu events */
	Event_Menu_Item_Selected,
};

struct event {
	GUI_eventType type;
	union {
		PSI_CONTROL pc;
		struct {
			int32_t x, y;
			uint32_t b;
		}mouse;
		struct {
			uint32_t w,h;
			LOGICAL start;
		}size;
		struct {
			int32_t x, y;
			LOGICAL start;
		}move;
		struct {
			RegistrationObject* type;
			struct {
				int32_t x, y;
				uint32_t b;
			}mouse;
		} xcontrol;
		struct {
			uint32_t code;
		}key;
		struct {
			PTEXT text;
		}console;
		struct {
			uintptr_t pli;
			LOGICAL opened;
		}listbox;
		struct {
			MenuItemObject* pmi;
		} popup;
		struct {
			is_control *control;
		}InterShell;
		struct {
			is_control *control;
			int32_t x, y;
			uint32_t w, h;
		}createCustomControl;
	}data;
	//RegistrationObject* registration;
	ControlObject* control;
	PTHREAD waiter;
	struct {
		BIT_FIELD complete : 1;
	}flags;
	uintptr_t success;
};
typedef struct event IS_EVENT;
#define MAXIS_EVENTSPERSET 64
DeclareSet( IS_EVENT );

#ifndef DEFINE_GLOBAL
extern
#endif
struct global {
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	PRENDER3D_INTERFACE p3di;
	struct {
		int32_t x, y;
		uint32_t w, h;
		is_control *control;
	} nextControlCreatePosition;
} g;

void InitInterfaces( int opengl, int vulkan );
uintptr_t MakeEvent( uv_async_t *async, PLINKQUEUE *queue, enum GUI_eventType type, ... );

void InitSystray( Isolate* isolate, Local<Object> _exports );
