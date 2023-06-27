#if WIN32

#include "global.h"

//#define LoG(...)
#define LoG lprintf


static uintptr_t InputThread( PTHREAD thread );
static void CPROC dispatchKey( uintptr_t psv, RAWINPUT *event, WCHAR ch, int len );

class KeyHidObject : public node::ObjectWrap {
public:
	int handle;
	char *name;

	Persistent<Object> this_;
	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE readQueue;
	LOGICAL blocking = FALSE;
public:

	static void Init( Isolate *isolate, Local<Object> exports );
	KeyHidObject(  );

private:
	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void lock( const v8::FunctionCallbackInfo<Value>& args );
	static void onRead( const v8::FunctionCallbackInfo<Value>& args );
	static void writeCom( const v8::FunctionCallbackInfo<Value>& args );
	static void closeCom( const v8::FunctionCallbackInfo<Value>& args );
	~KeyHidObject();
};

class MouseObject : public node::ObjectWrap {
public:
	int handle;
	char *name;
	int buttons;
	Persistent<Object> this_;
	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE readQueue;
	LOGICAL blocking = FALSE;
public:

	static void Init( Isolate *isolate, Local<Object> exports );
	MouseObject(  );

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void onRead( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	~MouseObject();
};

struct input_data {
	HANDLE hDevice;
	char *name;
	Eternal<String> v8name;
};

struct msgbuf {
	LOGICAL close;
	RAWINPUT event;
	KBDLLHOOKSTRUCT hookEvent;
	WCHAR ch;
};

struct mouse_msgbuf {
	LOGICAL close;
	MSLLHOOKSTRUCT data;
	WPARAM msgid;
	//RAWINPUT event;
	//WCHAR ch;
};



typedef struct global_tag
{
	HWND hWnd;
	uint32_t nWriteTimeout;
	uv_loop_t* loop;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	KeyHidObject *eventHandler;
	MouseObject* mouseEventHandler;
	PTHREAD mouseThread;
	PLIST inputs;
	HHOOK hookHandle;
	HHOOK hookHandleLL;
	HHOOK hookHandleM;
	HHOOK hookHandleMLL;
	int skipEvent;
	LOGICAL blocking;
} GLOBAL;

static GLOBAL hidg;


LRESULT WINAPI KeyboardProcLL( int code, WPARAM wParam, LPARAM lParam ) {
	KBDLLHOOKSTRUCT *kbhook = (KBDLLHOOKSTRUCT*)lParam;
	static int resending;
	static struct states {
		int state;
		int tick;
		INPUT events[12];
		INPUT eventsUp[12];
		struct {
			int code;
			int wParam;
			KBDLLHOOKSTRUCT lParam;
		} pending[12];
	} States[10];
	static int lastDownSkip;
	//static 
	int n = 10;
	int up = (kbhook->flags & LLKHF_UP) != 0;

	if( resending ) {
		return CallNextHookEx( hidg.hookHandleLL, code, wParam, lParam );
	}
	if( hidg.blocking ) {
		hidg.skipEvent = 1;
		if( 0 ) {
			LoG( "Drop key." );
			lastDownSkip++;
		}
		return 1;
	}

	char ch = MapVirtualKey( kbhook->vkCode, MAPVK_VK_TO_CHAR );
	struct msgbuf* msgbuf = NewPlus( struct msgbuf, 0 );
	msgbuf->close = FALSE;
	//msgbuf->event = event[0];
	msgbuf->hookEvent = kbhook[0];
	msgbuf->ch = ch;

	//KeyHidObject* com = (KeyHidObject*)psv;
	EnqueLink( &hidg.eventHandler->readQueue, msgbuf );
	uv_async_send( &hidg.eventHandler->async );

	return CallNextHookEx( hidg.hookHandleLL, code, wParam, lParam );

}

LRESULT WINAPI MouseProcLL( int code, WPARAM wParam, LPARAM lParam ) {
	MSLLHOOKSTRUCT *mhook = (MSLLHOOKSTRUCT *)lParam;
	struct mouse_msgbuf* input = NewArray( struct mouse_msgbuf, 1 );
	/* wParam =  WM_LBUTTONDOWN, WM_LBUTTONUP, WM_MOUSEMOVE, WM_MOUSEWHEEL, WM_MOUSEHWHEEL, WM_RBUTTONDOWN, or WM_RBUTTONUP. */
	//lprintf( "LL Mouse: %d %d %d %d %d %d %d %d", code, wParam
	//			, mhook->pt.x, mhook->pt.y
	//			, mhook->mouseData, mhook->dwExtraInfo, mhook->flags, mhook->time );
	if( code == HC_ACTION ) {
		input->close = FALSE;
		input->msgid = wParam;
		input->data = mhook[0];

		EnqueLink( &hidg.mouseEventHandler->readQueue, input );
		uv_async_send( &hidg.mouseEventHandler->async );
	}
	return CallNextHookEx( hidg.hookHandleMLL, code, wParam, lParam );
}

uintptr_t InputThread( PTHREAD thread )
{
	// --- InstallHook (HookingRawInputDemoDLL.cpp) ---
	// Register keyboard Hook
	//WH_KEYBOARD_LL
	HMODULE xx;
	xx = GetModuleHandle( "sack_vfs.node" );
	hidg.hookHandleLL = SetWindowsHookEx( WH_KEYBOARD_LL, (HOOKPROC)KeyboardProcLL, xx, 0 );
	hidg.nWriteTimeout = 150; // at 9600 == 144 characters
	{
		MSG msg;
		//lprintf( "Err:%d",GetLastError());
		while( GetMessage( &msg, NULL, 0, 0 ) )
			DispatchMessage( &msg );
		return msg.wParam;
	}
	return 0;
}


uintptr_t MouseInputThread( PTHREAD thread )
{
	HMODULE xx = GetModuleHandle( "sack_vfs.node" );
	//lprintf( "%p", xx );
	hidg.hookHandleMLL = SetWindowsHookEx( WH_MOUSE_LL, MouseProcLL, xx, 0 );
	//hidg.hookHandleM = SetWindowsHookEx( WH_MOUSE, MouseProc, xx, 0 );
	{
		MSG msg;
		while( GetMessage( &msg, NULL, 0, 0 ) )
			DispatchMessage( &msg );
		return msg.wParam;
	}
	return 0;
}

static void asyncmsg( uv_async_t* handle );

KeyHidObject::KeyHidObject(  ) {
	readQueue = CreateLinkQueue();
	hidg.eventHandler = this;
	//MSG msg;
	// create message queue on main thread.
	//PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );
	ThreadTo( InputThread, 0 );
}

KeyHidObject::~KeyHidObject() {
    hidg.eventHandler = NULL; // no longer have a handler.
	DeleteLinkQueue( &this->readQueue );

}


void KeyHidObject::Init( Isolate *isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> keyTemplate;
	class constructorSet* c = getConstructors( isolate );

	//----------------------------------------------------------------
	keyTemplate = FunctionTemplate::New( isolate, New );
	keyTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.KeyHidEvents" ) );
	keyTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

																 // Prototype
	NODE_SET_PROTOTYPE_METHOD( keyTemplate, "onKey", onRead );
	NODE_SET_PROTOTYPE_METHOD( keyTemplate, "lock", lock );

	c->KeyHidObject_constructor.Reset( isolate, keyTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	SET( exports, "Keyboard"
		, keyTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

	//----------------------------------------------------------------
	Local<FunctionTemplate> mouseTemplate;

	mouseTemplate = FunctionTemplate::New( isolate, MouseObject::New );
	mouseTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.MouseHidEvents" ) );
	mouseTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

																 // Prototype
	NODE_SET_PROTOTYPE_METHOD( mouseTemplate, "onMouse", MouseObject::onRead );
	NODE_SET_PROTOTYPE_METHOD( mouseTemplate, "close", MouseObject::close );
	//NODE_SET_PROTOTYPE_METHOD( mouseTemplate, "lock", lock );

	c->MouseHidObject_constructor.Reset( isolate, mouseTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	SET( exports, "Mouse"
		, mouseTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

	//----------------------------------------------------------------

}

void KeyHidObjectInit( Isolate *isolate, Local<Object> exports ) {
	KeyHidObject::Init( isolate, exports );

}

static void uv_closed( uv_handle_t* handle ) {
    KeyHidObject* myself = (KeyHidObject*)handle->data;

    myself->readCallback.Reset();
    myself->this_.Reset();
}

void asyncmsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();

	KeyHidObject* myself = (KeyHidObject*)handle->data;
	{
		struct msgbuf *msg;
        while( msg = (struct msgbuf *)DequeLink( &myself->readQueue ) ) {
            if( msg->close ) {
				//lprintf( "Key async close" );
                uv_close( (uv_handle_t*)&myself->async, uv_closed );
				break;
            }

			Local<Object> eventObj = Object::New( isolate );
			//struct input_data *indev;
			//INDEX idx;
			/*
			if(0)
			LIST_FORALL( hidg.inputs, idx, struct input_data *, indev ) {
				if( indev->hDevice == msg->event.header.hDevice ) {
					lprintf( "Found indev device? %s", indev->name );
					if( indev->name ) {
						lprintf( "Set name of indev to:%s", indev->name );
						indev->v8name.Set( isolate, String::NewFromUtf8( isolate, indev->name, v8::NewStringType::kNormal ).ToLocalChecked() );
						Release( indev->name );
						indev->name = NULL;
					}
					break;
				}
			}
			*/
			SET( eventObj, "down", (msg->event.data.keyboard.Flags&RI_KEY_BREAK)?False(isolate):True(isolate) );
			SET( eventObj, "scan", Integer::New( isolate, msg->event.data.keyboard.MakeCode ) );
			SET( eventObj, "vkey", Integer::New( isolate, msg->event.data.keyboard.VKey ) );

			SET( eventObj, "char", String::NewFromTwoByte( isolate, (const uint16_t*)&msg->ch, NewStringType::kNormal, 1 ).ToLocalChecked() );
			//SET( eventObj, "id", Number::New( isolate,(double)idx ) );
			/*
			if( indev )
				SET( eventObj, "device", indev->v8name.Get(isolate) );
			else
				SET( eventObj, "device", String::NewFromUtf8( isolate, "?" ).ToLocalChecked() );
				*/

			Local<Value> argv[] = { eventObj };
			if( !myself->readCallback.IsEmpty() ) {
				MaybeLocal<Value> result = myself->readCallback.Get(isolate)->Call(context, isolate->GetCurrentContext()->Global(), 1, argv);
				if( result.IsEmpty() ) {
					Deallocate( struct msgbuf*, msg );
					return;
				}
			}
			Deallocate( struct msgbuf *, msg );
		}
	}
	{
		// This is hook into Node to dispatch Promises() that are created... all event loops should have this.
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}

void KeyHidObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		KeyHidObject* obj = new KeyHidObject( );
		{
			//MemSet( &obj->async, 0, sizeof( obj->async ) );

			class constructorSet *c = getConstructors( isolate );
			uv_async_init( c->loop, &obj->async, asyncmsg );
			obj->async.data = obj;

			obj->Wrap( args.This() );
			obj->this_.Reset( isolate, args.This() );
			args.GetReturnValue().Set( args.This() );

			if( args.Length() > 0 ) {
				if( args[0]->IsFunction() ) {
					Local<Function> arg0 = Local<Function>::Cast( args[0] );
					obj->readCallback.Reset( isolate, arg0 );
				}
			}

		}
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Value>* argv = new Local<Value>[args.Length()];
		for( int n = 0; n < args.Length(); n++ ) argv[n] = args[n];
		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->KeyHidObject_constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), args.Length(), argv ).ToLocalChecked() );
		delete argv;
	}
}



void CPROC dispatchKey( uintptr_t psv, RAWINPUT *event, WCHAR ch, int len ) {
    struct msgbuf *msgbuf = NewPlus( struct msgbuf, len );
    msgbuf->close = FALSE;
    msgbuf->event = event[0];
    msgbuf->ch = ch;

    KeyHidObject *com = (KeyHidObject*)psv;
    EnqueLink( &com->readQueue, msgbuf );
    uv_async_send( &com->async );
}

void KeyHidObject::lock( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	KeyHidObject* com = ObjectWrap::Unwrap<KeyHidObject>( args.This() );
	int argc = args.Length();
	if( argc ) {
		if( args[0].As<Boolean>()->IsTrue() ) {
			hidg.blocking = TRUE;
		}
		else
			hidg.blocking = FALSE;
	}
	else
		hidg.blocking = !hidg.blocking;
}

void KeyHidObject::onRead( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc < 1 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Must pass callback(or null) to onRead handler" ) ) );
		return;
	}

	KeyHidObject *com = ObjectWrap::Unwrap<KeyHidObject>( args.This() );
        if( args[0]->IsNull() ) {
            if( argc > 1 ) {
                if( args[1]->IsTrue() ) {
                    struct msgbuf *msgbuf = NewPlus( struct msgbuf, 1 );
                    msgbuf->close = TRUE;
                    EnqueLink( &com->readQueue, msgbuf );
                    uv_async_send( &com->async );
					return;
                } 
            }
			com->readCallback.Reset();
		}
	else if( args[0]->IsFunction() ) {
		Local<Function> arg0 = Local<Function>::Cast( args[0] );
		com->readCallback = Persistent<Function, CopyablePersistentTraits<Function>>( isolate, arg0 );
	}
	else {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Unhandled parameter value to keyboard reader." ) ) );
	}
}


void mouse_asyncmsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();

	MouseObject* myself = (MouseObject*)handle->data;
	{
		struct mouse_msgbuf *msg;
        while( msg = (struct mouse_msgbuf *)DequeLink( &myself->readQueue ) ) {
            if( msg->close ) {
				uv_close( (uv_handle_t*)&myself->async, uv_closed );
				DeleteLinkQueue( &myself->readQueue );
                break;
            }

			if( !myself->readCallback.IsEmpty() ) {
				Local<Object> eventObj = Object::New( isolate );
				WPARAM hi_mdata = HIWORD( msg->data.mouseData );
				int wheel = 0;
				SET( eventObj, "op", Integer::New( isolate, (int)msg->msgid ) );
				switch( msg->msgid ) {
				//case WM_MOUSEHWHEEL:
				//case WM_MOUSEVWHEEL:
				case WM_MOUSEWHEEL:
					wheel = HIWORD( msg->data.mouseData );
					break;
				case WM_XBUTTONDOWN:
				case WM_NCXBUTTONDOWN:
					if( hi_mdata & XBUTTON1 )
						myself->buttons |= MK_XBUTTON1;
					if( hi_mdata & XBUTTON2 )
						myself->buttons |= MK_XBUTTON2;
					break;
				case WM_XBUTTONUP:
				case WM_NCXBUTTONUP:
					if( hi_mdata & XBUTTON1 )
						myself->buttons &= ~MK_XBUTTON1;
					if( hi_mdata & XBUTTON2 )
						myself->buttons &= ~MK_XBUTTON2;
					break;

				case WM_NCLBUTTONDOWN:
				case WM_LBUTTONDOWN: myself->buttons |= MK_LBUTTON; break;
				case WM_NCMBUTTONDOWN:
				case WM_MBUTTONDOWN: myself->buttons |= MK_MBUTTON; break;
				case WM_NCRBUTTONDOWN:
				case WM_RBUTTONDOWN: myself->buttons |= MK_RBUTTON; break;
				case WM_NCLBUTTONUP:
				case WM_LBUTTONUP:   myself->buttons &= ~MK_LBUTTON; break;
				case WM_NCMBUTTONUP:
				case WM_MBUTTONUP:   myself->buttons &= ~MK_MBUTTON; break;
				case WM_NCRBUTTONUP:
				case WM_RBUTTONUP:   myself->buttons &= ~MK_RBUTTON; break;

				case WM_MOUSEMOVE:
					break;
				//case WM_
				default:
					lprintf( "Unhandled op:%d", msg->msgid );
					break;
				/*
				case WM_LBUTTONDBLCLK:
				case WM_MBUTTONDBLCLK:
				case WM_RBUTTONDBLCLK:

				case WM_NCLBUTTONDBLCLK:
				case WM_NCMBUTTONDBLCLK:
				case WM_NCRBUTTONDBLCLK:

				case WM_XBUTTONDBLCLK:
				case WM_NCXBUTTONDBLCLK:
					break;
				*/
				}
				SET( eventObj, "wheel", Integer::New( isolate, wheel ) );
				SET( eventObj, "buttons", Integer::New( isolate, myself->buttons ) );
				SET( eventObj, "x", Integer::New( isolate, msg->data.pt.x ) );
				SET( eventObj, "y", Integer::New( isolate, msg->data.pt.y ) );

				Local<Value> argv[] = { eventObj };
				MaybeLocal<Value> result = myself->readCallback.Get( isolate )->Call( context, myself->this_.Get(isolate), 1, argv);
				if( result.IsEmpty() ) {
					// don't handle further events- allow processing the thrown exception.
					Deallocate( struct msgbuf*, msg );
					return;
				}
			}
			Deallocate( struct msgbuf *, msg );
		}
	}
	{
		// This is hook into Node to dispatch Promises() that are created... all event loops should have this.
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}


MouseObject::MouseObject() {
	readQueue = CreateLinkQueue();
	buttons = 0;
	hidg.mouseEventHandler = this;
	hidg.mouseThread = ThreadTo( MouseInputThread, 0 );
}

MouseObject::~MouseObject() {
	//lprintf( "need to cleanup mouse object async handler." );
	PostThreadMessage( GetThreadID( hidg.mouseThread ) & 0xFFFFFFFF, WM_QUIT, 0, 0 );
	hidg.mouseEventHandler = NULL; // no longer have a handler.
	hidg.mouseThread = NULL;
	DeleteLinkQueue( &this->readQueue );

}

void MouseObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		MouseObject* obj = new MouseObject( );
		{
			class constructorSet *c = getConstructors( isolate );
			uv_async_init( c->loop, &obj->async, mouse_asyncmsg );
			obj->async.data = obj;

			obj->Wrap( args.This() );
			obj->this_.Reset( isolate, args.This() );
			args.GetReturnValue().Set( args.This() );

			if( args.Length() > 0 ) {
				if( args[0]->IsFunction() ) {
					Local<Function> arg0 = Local<Function>::Cast( args[0] );
					obj->readCallback.Reset( isolate, arg0 );
				}
			}
		}
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		class constructorSet* c = getConstructors( isolate );
		Local<Value> *argv = new Local<Value>[args.Length()];
		for( int n = 0; n < args.Length(); n++ ) argv[n] = args[n];
		Local<Function> cons = Local<Function>::New( isolate, c->MouseHidObject_constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), args.Length(), argv ).ToLocalChecked() );
		delete argv;
	}
}

void MouseObject::onRead( const v8::FunctionCallbackInfo<Value>& args ) {
	// set read callback
	MouseObject* com = ObjectWrap::Unwrap<MouseObject>( args.This() );
}

void MouseObject::close( const v8::FunctionCallbackInfo<Value>& args ) {
	MouseObject* com = ObjectWrap::Unwrap<MouseObject>( args.This() );
	com->this_.Reset();
	struct mouse_msgbuf* msgbuf = NewPlus( struct mouse_msgbuf, 1 );
	msgbuf->close = TRUE;
	EnqueLink( &com->readQueue, msgbuf );
	uv_async_send( &com->async );
}

#endif
