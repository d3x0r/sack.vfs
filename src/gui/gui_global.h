
#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>
#include <uv.h>
//#include <nan.h>
//#include <varargs.h>
#include <stdarg.h>
#define NO_AUTO_VECTLIB_NAMES
#ifndef NO_FILEOP_ALIAS
#define NO_FILEOP_ALIAS
#endif

#include <stdhdrs.h>
#include <filesys.h>
#include <sack_vfs.h>
#include <pssql.h>
#include <deadstart.h>
#include <translation.h>

#define USE_RENDER_INTERFACE g.pdi
#define USE_IMAGE_INTERFACE g.pii
#include <image.h>
#include <render.h>
#include <psi.h>


#undef New

using namespace v8;

#include "sack_image_module.h"
#include "sack_render_module.h"

#include "sack_psi_module.h" // root sources, does Node root init entry point

#include "sack_intershell_module.h"


enum eventType {
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

	Event_Control_Create,
	Event_Control_Mouse,
	Event_Control_Key,
	Event_Control_Draw,
	Event_Control_Load,
};

struct event {
	eventType type;
	union {
		struct {
			int32_t x, y;
			uint32_t b;
		}mouse;
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
	int success;
};

#ifndef DEFINE_GLOBAL
extern
#endif
struct global {
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	struct {
		int32_t x, y;
		uint32_t w, h;
		is_control *control;
		PSI_CONTROL resultControl;
	} nextControlCreatePosition;
} g;


int MakeEvent( uv_async_t *async, PLINKQUEUE *queue, enum eventType type, ... );

