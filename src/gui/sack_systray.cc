

#include "../global.h"


class callbackWrapper {
public:
	Persistent<Function> cb;
};

enum systrayEvents {
	Event_Systray_DoubleClick,
	Event_Systray_MenuFunction,
	Event_Systray_Close_Loop,
	Event_Systray_,
};

struct systrayEvent {
	systrayEvents type;
	uintptr_t param;
};

typedef struct systrayEvent SS_EVENT;
#define MAXSS_EVENTSPERSET 64
DeclareSet( SS_EVENT );


static struct sytrayLocal {
	LOGICAL set;
	PTEXT icon;
	Persistent<Function> doubleClick;
	PLINKQUEUE events;
	LOGICAL eventLoopRegistered;
	int eventLoopEnables;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback

	SS_EVENTSET event_pool;

} systrayLocal;

static uintptr_t MakeSystrayEvent( enum systrayEvents type, ... );


static void asyncmsg( uv_async_t* handle ) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	systrayEvent* evt;
	while( evt = (struct systrayEvent*)DequeLink( &systrayLocal.events ) ) {
		switch( evt->type ) {
		case Event_Systray_DoubleClick:
			if( !systrayLocal.doubleClick.IsEmpty() ) {
				Local<Function> f = systrayLocal.doubleClick.Get( isolate );
				f->Call( context, context->Global(), 0, NULL );
			}
			break;
		case Event_Systray_MenuFunction:
			class callbackWrapper* cb = (class callbackWrapper*)evt->param;
			Local<Function> f = cb->cb.Get( isolate );
			f->Call( context, context->Global(), 0, NULL );
			break;
		}
	}
	DeleteFromSet( SS_EVENT, &systrayLocal.event_pool, evt );
}

static void enableEventLoop( void ) {
	if( !systrayLocal.eventLoopRegistered ) {
		systrayLocal.eventLoopRegistered = TRUE;
		MemSet( &systrayLocal.async, 0, sizeof( &systrayLocal.async ) );
		uv_async_init( uv_default_loop(), &systrayLocal.async, asyncmsg );
	}
	systrayLocal.eventLoopEnables++;
}

static void disableEventLoop( void ) {
	if( systrayLocal.eventLoopRegistered ) {
		if( !(--systrayLocal.eventLoopEnables) ) {
			systrayLocal.eventLoopRegistered = FALSE;
			MakeSystrayEvent( Event_Systray_Close_Loop );
		}
	}
}


static uintptr_t MakeSystrayEvent( enum systrayEvents type, ... ) {
	systrayEvent* pe;
#define e (*pe)
	va_list args;
	va_start( args, type );
	if( type != Event_Control_Close_Loop )
		enableEventLoop();
	pe = GetFromSet( SS_EVENT, &systrayLocal.event_pool );
	e.type = type;
	//e.registration = control->reg
	switch( type ) {
	case Event_Systray_DoubleClick:
	case Event_Systray_Close_Loop:
		break;
	case Event_Systray_MenuFunction:
		pe->param = va_arg( args, uintptr_t );
		break;
	}
	EnqueLink( &systrayLocal.events, pe );
	uv_async_send( &systrayLocal.async );
	return 0;
}


static void CPROC menuCallback( uintptr_t p) {
	MakeSystrayEvent( Event_Systray_MenuFunction, p );
}

static void CPROC doubleClickCallback( void ) {
	MakeSystrayEvent( Event_Systray_DoubleClick );
}



void setSystrayIcon( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() > 0 ) {
		if( args.Length() > 1 ) {
			Local<Function> cb = Local<Function>::Cast( args[1] );
			systrayLocal.doubleClick.Reset( isolate, cb );
		}
		String::Utf8Value fName( USE_ISOLATE( isolate ) args[0] );
		if( systrayLocal.set )
			ChangeIcon( *fName );
		else {
			enableEventLoop();
			RegisterIcon( *fName );
			SetIconDoubleClick( doubleClickCallback );
		}
		systrayLocal.set = 1;
	}
}


void setSystrayIconMenu( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() > 1 ) {
		Local<Function> cb = Local<Function>::Cast( args[1] );
		String::Utf8Value fName( USE_ISOLATE( isolate ) args[0] );
		class callbackWrapper* wrapper = new class callbackWrapper;
		wrapper->cb.Reset( isolate, cb );
		AddSystrayMenuFunction_v2( *fName, menuCallback, (uintptr_t)wrapper );
	}
}

void InitSystray( Isolate* isolate, Local<Object> _exports ) {
	Local<Context> context = isolate->GetCurrentContext();

	Local<Object> systrayObject = Object::New( isolate );
	SET_READONLY_METHOD( systrayObject, "set", setSystrayIcon );
	SET_READONLY_METHOD( systrayObject, "on", setSystrayIconMenu );

	SET( _exports, "Systray", systrayObject );

}
