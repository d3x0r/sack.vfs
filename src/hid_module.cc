#if WIN32

#include "global.h"

//#define LoG(...)
#define LoG lprintf


static uintptr_t InputThread( PTHREAD thread );

class KeyHidObject : public node::ObjectWrap {
public:
	int handle;
	char *name;

	Persistent<Object> this_;
	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //
	//uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	//PLINKQUEUE readQueue;
	LOGICAL blocking = FALSE;
public:

	static void Init( Isolate *isolate, Local<Object> exports );
	KeyHidObject(  );

private:
	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void lock( const v8::FunctionCallbackInfo<Value>& args );
	static void onRead( const v8::FunctionCallbackInfo<Value>& args );
	~KeyHidObject();
};

class MouseObject : public node::ObjectWrap {
public:
	Persistent<Object> this_;
	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //
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
	KBDLLHOOKSTRUCT hookEvent;
	WCHAR ch;
	LOGICAL done;
	LOGICAL used;
	PTHREAD waiter;
};

struct mouse_msgbuf {
	LOGICAL close;
	MSLLHOOKSTRUCT data;
	WPARAM msgid;
};



typedef struct global_tag
{

	PLIST keyEventHandlers;
	uv_async_t keyAsync; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE keyEvents;
	PTHREAD keyThread;

	PLIST mouseEventHandlers;
	uv_async_t mouseAsync; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE mouseEvents;
	PTHREAD mouseThread;
	int buttons;

	//PLIST inputs;
	//HHOOK hookHandle;
	HHOOK hookHandleLL;
	//HHOOK hookHandleM;
	HHOOK hookHandleMLL;
	int skipEvent;
	LOGICAL blocking;
} GLOBAL;

static GLOBAL hidg;


static void getCursor( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	const Local<String> x = String::NewFromUtf8Literal( isolate, "x" );
	const Local<String> y = String::NewFromUtf8Literal( isolate, "y" );
	Local<Object> obj = Object::New( isolate );
	POINT point;
	GetCursorPos( &point );
	obj->Set( isolate->GetCurrentContext(), x, Number::New( isolate, point.x ) );
	obj->Set( isolate->GetCurrentContext(), y, Number::New( isolate, point.y ) );
	args.GetReturnValue().Set( obj );
}
static void setCursor( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	const Local<String> x = String::NewFromUtf8Literal( isolate, "x" );
	const Local<String> y = String::NewFromUtf8Literal( isolate, "y" );
	Local<Object> obj = args[0].As< Object>();
	int64_t xnum = GETV( obj, x )->IntegerValue( context ).FromMaybe( 0 );
	int64_t ynum = GETV( obj, y )->IntegerValue( context ).FromMaybe( 0 );
	SetCursorPos( (int)xnum, (int)ynum );
}

LRESULT WINAPI KeyboardProcLL( int code, WPARAM wParam, LPARAM lParam ) {
	if( code < 0 ) 	return CallNextHookEx( hidg.hookHandleLL, code, wParam, lParam );

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
	msgbuf->waiter = MakeThread();
	msgbuf->done = 0;
	msgbuf->close = FALSE;
	//msgbuf->event = event[0];
	msgbuf->hookEvent = kbhook[0];
	msgbuf->ch = ch;

	//KeyHidObject* com = (KeyHidObject*)psv;
	EnqueLink( &hidg.keyEvents, msgbuf );
	uv_async_send( &hidg.keyAsync );
	while( !msgbuf->done ) Relinquish( 1 );
	if( msgbuf->used )
		return TRUE;
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

		EnqueLink( &hidg.mouseEvents, input );
		uv_async_send( &hidg.mouseAsync );
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
	//hidg.nWriteTimeout = 150; // at 9600 == 144 characters
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
	if( !hidg.keyThread )
		hidg.keyThread = ThreadTo( InputThread, 0 );
}

KeyHidObject::~KeyHidObject() {
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
	Local<Function> mouseInterface = mouseTemplate->GetFunction( isolate->GetCurrentContext() ).ToLocalChecked();
	SET( exports, "Mouse", mouseInterface );

	mouseInterface->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "cursor" )
		, Function::New( context, getCursor ).ToLocalChecked()
		, Function::New( context, setCursor ).ToLocalChecked()
	);

	//----------------------------------------------------------------

}

void KeyHidObjectInit( Isolate *isolate, Local<Object> exports ) {
	KeyHidObject::Init( isolate, exports );

}

static void uv_key_closed( uv_handle_t* handle ) {
	PostThreadMessage( GetThreadID( hidg.keyThread ) & 0xFFFFFFFF, WM_QUIT, 0, 0 );
	/*
	KeyHidObject* myself;// = (KeyHidObject*)handle->data;
	INDEX idx;
	LIST_FORALL( hidg.keyEventHandlers, idx, KeyHidObject*, myself ) {
		myself->readCallback.Reset();
		myself->this_.Reset();
	}
	*/
}

static void uv_closed( uv_handle_t* handle ) {
	PostThreadMessage( GetThreadID( hidg.mouseThread ) & 0xFFFFFFFF, WM_QUIT, 0, 0 );

}

void asyncmsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();

	KeyHidObject* myself;// = (KeyHidObject*)handle->data;
	{
		struct msgbuf *msg;
        while( msg = (struct msgbuf *)DequeLink( &hidg.keyEvents ) ) {
            if( msg->close ) {
				//lprintf( "Key async close" );
                uv_close( (uv_handle_t*)&hidg.keyAsync, uv_key_closed );
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
			SET( eventObj, "down", (msg->hookEvent.flags& LLKHF_UP )?False(isolate):True(isolate) );
			SET( eventObj, "scan", Integer::New( isolate, msg->hookEvent.scanCode ) );
			SET( eventObj, "vkey", Integer::New( isolate, msg->hookEvent.vkCode ) );

			SET( eventObj, "char", String::NewFromTwoByte( isolate, (const uint16_t*)&msg->ch, NewStringType::kNormal, 1 ).ToLocalChecked() );
			//SET( eventObj, "id", Number::New( isolate,(double)idx ) );
			/*
			if( indev )
				SET( eventObj, "device", indev->v8name.Get(isolate) );
			else
				SET( eventObj, "device", String::NewFromUtf8( isolate, "?" ).ToLocalChecked() );
				*/

			Local<Value> argv[] = { eventObj };
			INDEX idx;

			LIST_FORALL( hidg.keyEventHandlers, idx, class KeyHidObject*, myself ) {
				if( !myself->readCallback.IsEmpty() ) {
					MaybeLocal<Value> result = myself->readCallback.Get( isolate )->Call( context, isolate->GetCurrentContext()->Global(), 1, argv );
					if( result.IsEmpty() ) {
						//Deallocate( struct msgbuf*, msg );
						msg->done = TRUE;
						msg->used = FALSE;
						WakeThread( msg->waiter );
						return;
					}
					else {
						msg->used = result.ToLocalChecked()-> TOBOOL( isolate );
						if( msg->used ) {
							msg->done = TRUE;
							WakeThread( msg->waiter );
							break;
						}
					}
				}
			}
			if( !msg->done ) {
				msg->done = TRUE;
				msg->used = FALSE;
				WakeThread( msg->waiter );
			}
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
			if( !hidg.keyEvents ) {
				hidg.keyEvents = CreateLinkQueue();
				uv_async_init( c->loop, &hidg.keyAsync, asyncmsg );
			}
			AddLink( &hidg.keyEventHandlers, obj );

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



void KeyHidObject::close( const v8::FunctionCallbackInfo<Value>& args ) {
	KeyHidObject* com = ObjectWrap::Unwrap<KeyHidObject>( args.This() );
	com->this_.Reset();
	com->readCallback.Reset();

	DeleteLink( &hidg.keyEventHandlers, com );
	if( !GetLinkCount( hidg.keyEventHandlers ) ) {
		struct msgbuf* msg = NewArray( struct msgbuf, 1 );
		msg->close = TRUE;
		EnqueLink( &hidg.keyEvents, msg );
	}
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
	/*
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
	else
	*/
	if( args[0]->IsFunction() ) {
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

	MouseObject* myself;
	INDEX idx;
	{
		struct mouse_msgbuf *msg;
        while( msg = (struct mouse_msgbuf *)DequeLink( &hidg.mouseEvents ) ) {
            if( msg->close ) {
				uv_close( (uv_handle_t*)&hidg.mouseAsync, uv_closed );
				DeleteLinkQueue( &hidg.mouseEvents );
                break;
            }
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
					hidg.buttons |= MK_XBUTTON1;
				if( hi_mdata & XBUTTON2 )
					hidg.buttons |= MK_XBUTTON2;
				break;
			case WM_XBUTTONUP:
			case WM_NCXBUTTONUP:
				if( hi_mdata & XBUTTON1 )
					hidg.buttons &= ~MK_XBUTTON1;
				if( hi_mdata & XBUTTON2 )
					hidg.buttons &= ~MK_XBUTTON2;
				break;

			case WM_NCLBUTTONDOWN:
			case WM_LBUTTONDOWN: hidg.buttons |= MK_LBUTTON; break;
			case WM_NCMBUTTONDOWN:
			case WM_MBUTTONDOWN: hidg.buttons |= MK_MBUTTON; break;
			case WM_NCRBUTTONDOWN:
			case WM_RBUTTONDOWN: hidg.buttons |= MK_RBUTTON; break;
			case WM_NCLBUTTONUP:
			case WM_LBUTTONUP:   hidg.buttons &= ~MK_LBUTTON; break;
			case WM_NCMBUTTONUP:
			case WM_MBUTTONUP:   hidg.buttons &= ~MK_MBUTTON; break;
			case WM_NCRBUTTONUP:
			case WM_RBUTTONUP:   hidg.buttons &= ~MK_RBUTTON; break;

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
			SET( eventObj, "buttons", Integer::New( isolate, hidg.buttons ) );
			SET( eventObj, "x", Integer::New( isolate, msg->data.pt.x ) );
			SET( eventObj, "y", Integer::New( isolate, msg->data.pt.y ) );

			Local<Value> argv[] = { eventObj };

			LIST_FORALL( hidg.mouseEventHandlers, idx, MouseObject*, myself ) {
				if( !myself->readCallback.IsEmpty() ) {
					MaybeLocal<Value> result = myself->readCallback.Get( isolate )->Call( context, myself->this_.Get( isolate ), 1, argv );
					if( result.IsEmpty() ) {
						// don't handle further events- allow processing the thrown exception.
						Deallocate( struct mouse_msgbuf*, msg );
						return;
					}
				}
			}
			Deallocate( struct mouse_msgbuf *, msg );
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
	if( !hidg.mouseThread )
		hidg.mouseThread = ThreadTo( MouseInputThread, 0 );
}

MouseObject::~MouseObject() {
	//lprintf( "need to cleanup mouse object async handler." );
	//hidg.mouseEventHandler = NULL; // no longer have a handler.
}

void MouseObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		MouseObject* obj = new MouseObject( );
		{
			class constructorSet *c = getConstructors( isolate );
			if( !hidg.mouseEvents ) {
				hidg.mouseEvents = CreateLinkQueue();
				uv_async_init( c->loop, &hidg.mouseAsync, mouse_asyncmsg );
			}
			AddLink( &hidg.mouseEventHandlers, obj );
			//obj->async.data = obj;

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
	com->readCallback.Reset();

	DeleteLink( &hidg.mouseEventHandlers, com );
	if( !GetLinkCount( hidg.mouseEventHandlers ) ) {
		struct mouse_msgbuf* msg = NewArray( struct mouse_msgbuf, 1 );
		msg->close = TRUE;
		msg->msgid = 0;
		EnqueLink( &hidg.mouseEvents, msg );
		uv_async_send( &hidg.mouseAsync );
		//hidg.mouseThread = NULL;
	}

	/*
	struct mouse_msgbuf* msgbuf = NewPlus( struct mouse_msgbuf, 1 );
	msgbuf->close = TRUE;
	EnqueLink( &com->readQueue, msgbuf );
	uv_async_send( &hidg.mouseAsync );
	*/
}


#endif
