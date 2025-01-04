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
	Persistent<Function> readCallback; //
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
	static void sendKey( const v8::FunctionCallbackInfo<Value>& args );
	
	~KeyHidObject();
};

class MouseObject : public node::ObjectWrap {
public:
	Persistent<Object> this_;
	Persistent<Function> readCallback; //
public:

	static void Init( Isolate *isolate, Local<Object> exports );
	MouseObject(  );

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void onRead( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void clickAt( const v8::FunctionCallbackInfo<Value>& args );
	static void event( const v8::FunctionCallbackInfo<Value>& args );
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
	int mouseX, mouseY;

	//PLIST inputs;
	//HHOOK hookHandle;
	HHOOK hookHandleLL;
	//HHOOK hookHandleM;
	HHOOK hookHandleMLL;
	int skipEvent;
	LOGICAL blocking;
} GLOBAL;

static GLOBAL hidg;

PRIORITY_ATEXIT( removeHooks, 100 ) {
	UnhookWindowsHookEx( hidg.hookHandleLL );
	UnhookWindowsHookEx( hidg.hookHandleMLL );
}

static void getCursor( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	const Local<String> x = String::NewFromUtf8Literal( isolate, "x" );
	const Local<String> y = String::NewFromUtf8Literal( isolate, "y" );
	Local<Object> obj = Object::New( isolate );
	POINT point;
	if( hidg.hookHandleMLL ) {
		obj->Set( isolate->GetCurrentContext(), x, Number::New( isolate, hidg.mouseX ) );
		obj->Set( isolate->GetCurrentContext(), y, Number::New( isolate, hidg.mouseY ) );
	} else {
		GetCursorPos( &point );
		obj->Set( isolate->GetCurrentContext(), x, Number::New( isolate, point.x ) );
		obj->Set( isolate->GetCurrentContext(), y, Number::New( isolate, point.y ) );
	}
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
	if( resending ) {
		return CallNextHookEx( hidg.hookHandleLL, code, wParam, lParam );
	}
	if( hidg.blocking ) {
		hidg.skipEvent = 1;
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
	while( !msgbuf->done ) WakeableSleep( 1 );
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
	// initialize to the best state we can know.
	// after this point, this will track with the low level messages received.
	hidg.buttons = ( GetAsyncKeyState( VK_LBUTTON ) ? MK_LBUTTON : 0 )
		| ( GetAsyncKeyState( VK_RBUTTON ) ? MK_RBUTTON : 0 )
		| ( GetAsyncKeyState( VK_MBUTTON ) ? MK_MBUTTON : 0 )
		| ( GetAsyncKeyState( VK_XBUTTON1 ) ? MK_XBUTTON1 : 0 )
		| ( GetAsyncKeyState( VK_XBUTTON2 ) ? MK_XBUTTON2 : 0 );
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
	NODE_SET_PROTOTYPE_METHOD( keyTemplate, "send", sendKey );

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
	SET_READONLY_METHOD( mouseInterface, "event", MouseObject::event );
	SET_READONLY_METHOD( mouseInterface, "clickAt", MouseObject::clickAt );

	Local<Object> buttonFlags = Object::New( isolate );
	SET_READONLY( buttonFlags, "left", Integer::New( isolate, MK_LBUTTON ) );
	SET_READONLY( buttonFlags, "right", Integer::New( isolate, MK_RBUTTON ) );
	SET_READONLY( buttonFlags, "middle", Integer::New( isolate, MK_MBUTTON ) );
	SET_READONLY( buttonFlags, "x1", Integer::New( isolate, MK_XBUTTON1 ) );
	SET_READONLY( buttonFlags, "x2", Integer::New( isolate, MK_XBUTTON2 ) );
	SET_READONLY( buttonFlags, "relative", Integer::New( isolate, 0x1000000 ) );
	//SET_READONLY( wsWebStatesObject, "", Integer::New( isolate, wsReadyStates::LISTENING ) );
	SET_READONLY( mouseInterface, "buttons", buttonFlags );

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
			SET( eventObj, "time", Number::New( isolate, msg->hookEvent.time ) );
			SET( eventObj, "extended", (msg->hookEvent.flags & LLKHF_EXTENDED)?True(isolate):False(isolate) );

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
						msg->used = result.ToLocalChecked()->TOBOOL( isolate );
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
		delete[] argv;
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

	if( args[0]->IsFunction() ) {
		com->readCallback.Reset( isolate, Local<Function>::Cast(args[0]));
	}
	else {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Unhandled parameter value to keyboard reader." ) ) );
	}
}



struct sendParam {
	PDATALIST pdlInputs;//
};
static PTHREAD pSendKeyThread = NULL;
static PLINKQUEUE plsKeys     = NULL;

static uintptr_t KeySendThread( PTHREAD thread ) {
	while( TRUE ) {
		struct sendParam *param;
		while( param = (struct sendParam *)DequeLink( &plsKeys ) ) {
			struct sendParam *pend;
			while( pend = (struct sendParam *)DequeLink( &plsKeys ) ) {
				INDEX idx;
				PINPUT pInput;
				DATA_FORALL( pend->pdlInputs, idx, PINPUT, pInput ) { AddDataItem( &param->pdlInputs, pInput ); }
			}
			// lprintf( "thread Generating event: %d %d %d", param->used_inputs, param->inputs[ 0 ].ki.wScan
			//        , param->inputs[ 0 ].ki.wVk );
			BOOL b = SendInput( param->pdlInputs->Cnt, (INPUT*)param->pdlInputs->data, sizeof( INPUT ) );
			if( !b ) {
				lprintf( "SendInput failed. %d", GetLastError() );
			}
			DeleteDataList( &param->pdlInputs );
			Release( param );
		}
		WakeableSleep( SLEEP_FOREVER );
	}
}


static void generateKeys( Isolate *isolate, Local<Array> events ) {
#ifdef _WIN32
	Local<Context> context = isolate->GetCurrentContext();
	struct sendParam *param = NewPlus( struct sendParam, 0 );
	param->pdlInputs = CreateDataList( sizeof( INPUT ) );
	INPUT input;
	int event = 0;
	input.type = INPUT_KEYBOARD;
	input.ki.time = 0; // auto time
	input.ki.dwExtraInfo = 0;

	for( event = 0; event < events->Length(); event++ ) {
		Local<Object> e = events->Get( context, event ).ToLocalChecked().As<Object>();
		Local<Value> ex = e->Get( context, String::NewFromUtf8Literal( isolate, "key" ) ).ToLocalChecked();
		Local<Value> ey = e->Get( context, String::NewFromUtf8Literal( isolate, "code" ) ).ToLocalChecked();
		Local<Value> eb = e->Get( context, String::NewFromUtf8Literal( isolate, "down" ) ).ToLocalChecked();
		Local<Value> esh = e->Get( context, String::NewFromUtf8Literal( isolate, "extended" ) ).ToLocalChecked();
		input.ki.wVk = ex->IsNumber() ? ex.As<Number>()->IntegerValue( context ).ToChecked() : 0;
		input.ki.wScan = ey->IsNumber() ? ey.As<Number>()->IntegerValue( context ).ToChecked() : 0;
		input.ki.dwFlags =  eb->TOBOOL( isolate )?0:KEYEVENTF_KEYUP;
		input.ki.dwFlags |=  esh->TOBOOL( isolate )?KEYEVENTF_EXTENDEDKEY:0;
		AddDataItem( &param->pdlInputs, &input );
	}

	
	EnqueLink( &plsKeys, param );
	if( !pSendKeyThread )
		pSendKeyThread = ThreadTo( KeySendThread, 0 );
	else
		WakeThread( pSendKeyThread );

#endif
}


void KeyHidObject::sendKey( const v8::FunctionCallbackInfo<Value>& args ) {
#ifdef WIN32
	//static int old_x; // save old x,y to know whether current is a move or just a click
	//static int old_y; 
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int scan;
	int vKey;
	int down;
	int ext;

	if( args.Length() < 3 ) {
		if( args[0]->IsArray() ) {
			Local<Array> arr = Local<Array>::Cast( args[0] );
			generateKeys( isolate, arr );
			return;
		}
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "At least 3 arguments must be specified for a key event (vKey, scanCode, isDown, extended)." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	} else {
		vKey = (args.Length() > 0)?args[0]->IntegerValue( context ).ToChecked():0;
		scan = (args.Length() > 1)?args[1]->IntegerValue( context ).ToChecked():0;
		down = (args.Length() > 2)?args[2]->TOBOOL( isolate ):0;
		ext = (args.Length() > 3)?args[3]->TOBOOL( isolate ):0;

	}

	struct sendParam *param = NewPlus( struct sendParam, 0 );
	param->pdlInputs        = CreateDataList( sizeof( INPUT ) );
	INPUT input;
	//int used_inputs = 0;

	input.type = INPUT_KEYBOARD;

	{

		input.ki.wVk = vKey;
		input.ki.wScan = scan;
		// normal buttons have no extra data either - only include their states in the first message.
		input.ki.dwFlags |= 
			( ext ? KEYEVENTF_EXTENDEDKEY : 0 ) 
			| ( down ? 0 : KEYEVENTF_KEYUP )
		     //| KEYEVENTF_SCANCODE
		     //| KEYEVENTF_UNICODE
		     ;
		input.ki.time = 0; // auto time
		input.ki.dwExtraInfo = 0;
		AddDataItem( &param->pdlInputs, &input );
	};
	//lprintf( "Generating events: %d %d %d", param->used_inputs, inputs[ 0 ].ki.wScan, inputs[ 0 ].ki.wVk );
	EnqueLink( &plsKeys, param );
	if( !pSendKeyThread )
		pSendKeyThread = ThreadTo( KeySendThread, 0 );
	else
		WakeThread( pSendKeyThread );
	
#endif
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
			hidg.mouseX = msg->data.pt.x;
			hidg.mouseY = msg->data.pt.y;
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
		delete[] argv;
	}
}

void MouseObject::onRead( const v8::FunctionCallbackInfo<Value>& args ) {
	// set read callback
   Isolate *isolate = args.GetIsolate();
	MouseObject* com = ObjectWrap::Unwrap<MouseObject>( args.This() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() ) {
			Local<Function> arg0 = Local<Function>::Cast( args[0] );
			com->readCallback.Reset( isolate, arg0 );
		}
	}
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

static void generateEvents( Isolate *isolate, Local<Array> events ) {
	Local<Context> context = isolate->GetCurrentContext();
	int old_buttons;
	POINT curPos;

	//GetCursorPos( &curPos );
	curPos.x = hidg.mouseX;
	curPos.y = hidg.mouseY;
	old_buttons = hidg.buttons;
	PDATALIST pdlInputs = CreateDataList( sizeof( INPUT ) );
	INPUT input;
	int used_inputs = 0;
	int event = 0;
	input.type = INPUT_MOUSE;
	input.mi.time = 0; // auto time
	input.mi.dwExtraInfo = 0;

	for( event = 0; event < events->Length(); event++ ) {
		Local<Object> e = events->Get( context, event ).ToLocalChecked().As<Object>();
		Local<Value> ex = e->Get( context, String::NewFromUtf8Literal( isolate, "x" ) ).ToLocalChecked();
		Local<Value> ey = e->Get( context, String::NewFromUtf8Literal( isolate, "y" ) ).ToLocalChecked();
		Local<Value> eb = e->Get( context, String::NewFromUtf8Literal( isolate, "buttons" ) ).ToLocalChecked();
		Local<Value> esh = e->Get( context, String::NewFromUtf8Literal( isolate, "vscroll" ) ).ToLocalChecked();
		Local<Value> esw = e->Get( context, String::NewFromUtf8Literal( isolate, "hscroll" ) ).ToLocalChecked();
		int64_t x = ex->IsNumber() ? ex.As<Number>()->IntegerValue( context ).ToChecked() : curPos.x;
		int64_t y = ey->IsNumber() ? ey.As<Number>()->IntegerValue( context ).ToChecked() : curPos.y;
		int64_t b = eb->IsNumber() ? eb.As<Number>()->IntegerValue( context ).ToChecked() : old_buttons;
		//int64_t realB = b; // we destroy b with certain button combinations...

		//lprintf( "data: %d %d %d %d", x, y, b, old_buttons );
		bool hasScroll1 = esh->IsNumber();
		double down_up = hasScroll1 ? esh.As<Number>()->Value() : 0;
		// even if there's a parameter, it might not be a command
		if( !down_up ) hasScroll1 = false;
		bool hasScroll2 = esw->IsNumber();
		double right_left = hasScroll2 ? esw.As<Number>()->Value() : 0;
		// even if there's a parameter, it might not be a command
		if( !right_left ) hasScroll1 = false;
		//lprintf( "has Scrolls:%d %d", hasScroll1, hasScroll2 );

		bool has_x = false;
		if( b & 0x1000000 ) {
			x += curPos.x;
			y += curPos.y;
		}
		used_inputs = 0; // used_inputs is for a subset of events that get genertaed... one move
		do {
			if( used_inputs ) {
				//lprintf( "adding a second part of the event.." );
				// don't NEED to set dx,dy to 0, but it's cleaner if we do
				input.mi.dx = 0;
				input.mi.dy = 0;
				input.mi.dwFlags = 0;
			} else {
				if( x != curPos.x || y != curPos.y ) {
					double fScreenWidth = GetSystemMetrics( SM_CXVIRTUALSCREEN ) - 1;
					double fScreenHeight = GetSystemMetrics( SM_CYVIRTUALSCREEN ) - 1;
					double const sx = 65535.0f / fScreenWidth;
					double const sy = 65535.0f / fScreenHeight;
					input.mi.dx = (LONG)ceil( x * sx );
					input.mi.dy = (LONG)ceil( y * sy );
					/* assume that this event WILL move the mouse */
					//lprintf( "Updating the current mouse position... %d %d", x, y );
					curPos.x = hidg.mouseX = x;
					curPos.y = hidg.mouseY = y;
					input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_MOVE;
				} else {
					//lprintf( "Same position as current?" );
					// don't NEED to set dx,dy to 0, but it's cleaner if we do
					input.mi.dx = 0;
					input.mi.dy = 0;
					input.mi.dwFlags = 0;
					//lprintf( "no motion needed..." );
				}

				// normal buttons have no extra data either - only include their states in the first message.
				input.mi.dwFlags |=
					( ( ( b & MK_LBUTTON ) && !( old_buttons & MK_LBUTTON ) ) ? MOUSEEVENTF_LEFTDOWN : 0 )
					| ( ( ( b & MK_RBUTTON ) && !( old_buttons & MK_RBUTTON ) ) ? MOUSEEVENTF_RIGHTDOWN : 0 )
					| ( ( ( b & MK_MBUTTON ) && !( old_buttons & MK_MBUTTON ) ) ? MOUSEEVENTF_MIDDLEDOWN : 0 )
					| ( ( !( b & MK_LBUTTON ) && ( old_buttons & MK_LBUTTON ) ) ? MOUSEEVENTF_LEFTUP : 0 )
					| ( ( !( b & MK_RBUTTON ) && ( old_buttons & MK_RBUTTON ) ) ? MOUSEEVENTF_RIGHTUP : 0 )
					| ( ( !( b & MK_MBUTTON ) && ( old_buttons & MK_MBUTTON ) ) ? MOUSEEVENTF_MIDDLEUP : 0 )
					;
				// button messages are only on transition, not hold-states; update old_buttons
				// x1 and x2 states are updated as they are built into an event
				old_buttons = ( old_buttons & ( MK_XBUTTON1 | MK_XBUTTON2 ) )
					| ( b & ( MK_LBUTTON | MK_RBUTTON | MK_MBUTTON ) );
			}

			if( b & MK_XBUTTON1 && !( old_buttons & MK_XBUTTON1 ) ) {
				input.mi.dwFlags |= MOUSEEVENTF_XDOWN;
				input.mi.mouseData = XBUTTON1; // scroll wheel
				b &= ~MK_XBUTTON1;
				has_x = true;
			} else if( !( b & MK_XBUTTON1 ) && ( old_buttons & MK_XBUTTON1 ) ) {
				input.mi.dwFlags |= MOUSEEVENTF_XUP;
				input.mi.mouseData = XBUTTON1; // scroll wheel
				old_buttons &= ~MK_XBUTTON1;
				has_x = true;
			} else if( b & MK_XBUTTON2 && !( old_buttons & MK_XBUTTON2 ) ) {
				input.mi.dwFlags |= MOUSEEVENTF_XDOWN;
				input.mi.mouseData = XBUTTON2; // scroll wheel
				b &= ~MK_XBUTTON2;
				has_x = true;
			} else if( !( b & MK_XBUTTON2 ) && ( old_buttons & MK_XBUTTON2 ) ) {
				input.mi.dwFlags |= MOUSEEVENTF_XUP;
				input.mi.mouseData = XBUTTON2; // scroll wheel
				old_buttons &= ~MK_XBUTTON2;
				has_x = true;
			} else {
				has_x = false;
				if( hasScroll1 ) {
					input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
					input.mi.mouseData = (DWORD)( down_up * WHEEL_DELTA );
					hasScroll1 = false;
				} else if( hasScroll2 ) {
					input.mi.dwFlags |= MOUSEEVENTF_HWHEEL;
					input.mi.mouseData = (DWORD)( right_left * WHEEL_DELTA );
					hasScroll2 = false;
				} else {
					input.mi.mouseData = 0;
				}
			}

			hidg.buttons = old_buttons;
			//lprintf( "Adding events: x:%d y:%d f:%d ex:%d md:%d t:%d", input.mi.dx, input.mi.dy, input.mi.dwFlags, input.mi.dwExtraInfo, input.mi.mouseData, input.mi.time );
			AddDataItem( &pdlInputs, &input );
			used_inputs++;
		} while( has_x || hasScroll1 || hasScroll2 );
	}
	if( 0 ) {
		for( int i = 0; i < pdlInputs->Cnt; i++ ) {
			INPUT* input = (INPUT*)GetDataItem( &pdlInputs, i );
			lprintf( "Generating event: x:%d y:%d f:%d ex:%d md:%d t:%d", input->mi.dx, input->mi.dy, input->mi.dwFlags, input->mi.dwExtraInfo, input->mi.mouseData, input->mi.time );
		}
	}
	
	//int n = 
	SendInput( pdlInputs->Cnt, (LPINPUT)pdlInputs->data, sizeof( INPUT ) );
	//lprintf( "Send Inputreplied? %d", n );
	DeleteDataList( &pdlInputs );

}


void MouseObject::event( const v8::FunctionCallbackInfo<Value>& args ) {
	int old_buttons;
	//static int old_x; // save old x,y to know whether current is a move or just a click
	//static int old_y; 
	POINT curPos;
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 3 ) {
		if( args[0]->IsArray() ) {
			Local<Array> arr = Local<Array>::Cast( args[0] );
			generateEvents( isolate, arr );
			return;
		}
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "At least 3 arguments must be specified for a mouse event (x, y, buttons)." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Context> context = isolate->GetCurrentContext();
	GetCursorPos( &curPos );
	/*
	old_buttons = ( GetAsyncKeyState( VK_LBUTTON ) ? MK_LBUTTON : 0 )
		| ( GetAsyncKeyState( VK_RBUTTON ) ? MK_RBUTTON : 0 )
		| ( GetAsyncKeyState( VK_MBUTTON ) ? MK_MBUTTON : 0 )
		| ( GetAsyncKeyState( VK_XBUTTON1 ) ? MK_XBUTTON1 : 0 )
		| ( GetAsyncKeyState( VK_XBUTTON2 ) ? MK_XBUTTON2 : 0 );
	*/
	//lprintf( "current key state buttons: %08x %08x", old_buttons, hidg.buttons );
	old_buttons = hidg.buttons;
	int64_t x = args[0]->IsNumber() ? args[0].As<Number>()->IntegerValue( context ).ToChecked():curPos.x;
	int64_t y = args[1]->IsNumber() ? args[1].As<Number>()->IntegerValue( context ).ToChecked():curPos.y;
	int64_t b = args[2]->IsNumber() ? args[2].As<Number>()->IntegerValue( context ).ToChecked():old_buttons;
	//int64_t realB = b; // we destroy b with certain button combinations...

	bool hasScroll1 = (args.Length() > 3)?args[3]->IsNumber():false;
	double down_up = hasScroll1?args[3].As<Number>()->Value():0;
	// even if there's a parameter, it might not be a command
	if( !down_up ) hasScroll1 = false;
	bool hasScroll2 = ( args.Length() > 4 ) ? args[4]->IsNumber():false;
	double right_left = hasScroll2?args[4].As<Number>()->Value():0;
	// even if there's a parameter, it might not be a command
	if( !right_left ) hasScroll1 = false;
	//lprintf( "has Scrolls:%d %d", hasScroll1, hasScroll2 );

	INPUT inputs[4];
	int used_inputs = 0;
	bool has_x = false;
	if( b & 0x1000000 ) {
		x += curPos.x;
		y += curPos.y;
	}

	do {
		inputs[used_inputs].type = INPUT_MOUSE;
		if( used_inputs ) {
			// don't NEED to set dx,dy to 0, but it's cleaner if we do
			inputs[used_inputs].mi.dx = 0;
			inputs[used_inputs].mi.dy = 0;
			inputs[used_inputs].mi.dwFlags = 0;
		} else {
			if( x != curPos.x || y != curPos.y ) {
				double fScreenWidth = GetSystemMetrics( SM_CXVIRTUALSCREEN )-1;
				double fScreenHeight = GetSystemMetrics( SM_CYVIRTUALSCREEN )-1;
				double const sx = 65535.0f / fScreenWidth;
				double const sy = 65535.0f / fScreenHeight;
				inputs[used_inputs].mi.dx = (LONG)ceil( x * sx );
				inputs[used_inputs].mi.dy = (LONG)ceil( y * sy );
				inputs[used_inputs].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_MOVE;
			} else {
				// don't NEED to set dx,dy to 0, but it's cleaner if we do
				inputs[used_inputs].mi.dx = 0;
				inputs[used_inputs].mi.dy = 0;
				inputs[used_inputs].mi.dwFlags = 0;
				//lprintf( "no motion needed..." );
			}

			// normal buttons have no extra data either - only include their states in the first message.
			inputs[used_inputs].mi.dwFlags |=
				( ( ( b & MK_LBUTTON ) && !( old_buttons & MK_LBUTTON ) ) ? MOUSEEVENTF_LEFTDOWN : 0 )
				| ( ( ( b & MK_RBUTTON ) && !( old_buttons & MK_RBUTTON ) ) ? MOUSEEVENTF_RIGHTDOWN : 0 )
				| ( ( ( b & MK_MBUTTON ) && !( old_buttons & MK_MBUTTON ) ) ? MOUSEEVENTF_MIDDLEDOWN : 0 )
				| ( ( !( b & MK_LBUTTON ) && ( old_buttons & MK_LBUTTON ) ) ? MOUSEEVENTF_LEFTUP : 0 )
				| ( ( !( b & MK_RBUTTON ) && ( old_buttons & MK_RBUTTON ) ) ? MOUSEEVENTF_RIGHTUP : 0 )
				| ( ( !( b & MK_MBUTTON ) && ( old_buttons & MK_MBUTTON ) ) ? MOUSEEVENTF_MIDDLEUP : 0 )
				;
			// button messages are only on transition, not hold-states; update old_buttons
			// x1 and x2 states are updated as they are built into an event
			old_buttons = ( old_buttons & ( MK_XBUTTON1 | MK_XBUTTON2 ) )
				| ( b & ( MK_LBUTTON | MK_RBUTTON | MK_MBUTTON ) );
		}

		if( b & MK_XBUTTON1 && !( old_buttons & MK_XBUTTON1 ) ) {
			inputs[used_inputs].mi.dwFlags |= MOUSEEVENTF_XDOWN;
			inputs[used_inputs].mi.mouseData = XBUTTON1; // scroll wheel
			b &= ~MK_XBUTTON1;
			has_x = true;
		} else if( !(b & MK_XBUTTON1) && ( old_buttons & MK_XBUTTON1 ) ) {
			inputs[used_inputs].mi.dwFlags |= MOUSEEVENTF_XUP;
			inputs[used_inputs].mi.mouseData = XBUTTON1; // scroll wheel
			old_buttons &= ~MK_XBUTTON1;
			has_x = true;
		} else if( b & MK_XBUTTON2 && !(old_buttons&MK_XBUTTON2) ) {
			inputs[used_inputs].mi.dwFlags |= MOUSEEVENTF_XDOWN;
			inputs[used_inputs].mi.mouseData = XBUTTON2; // scroll wheel
			b &= ~MK_XBUTTON2;
			has_x = true;
		} else if( !(b& MK_XBUTTON2) && ( old_buttons& MK_XBUTTON2 ) ) {
			inputs[used_inputs].mi.dwFlags |= MOUSEEVENTF_XUP;
			inputs[used_inputs].mi.mouseData = XBUTTON2; // scroll wheel
			old_buttons &= ~MK_XBUTTON2;
			has_x = true;
		} else {
			has_x = false;
			if( hasScroll1 ) {
				inputs[used_inputs].mi.dwFlags |= MOUSEEVENTF_WHEEL;
				inputs[used_inputs].mi.mouseData = (DWORD)(down_up * WHEEL_DELTA );
				hasScroll1 = false;
			} else if( hasScroll2 ) {
				inputs[used_inputs].mi.dwFlags |= MOUSEEVENTF_HWHEEL;
				inputs[used_inputs].mi.mouseData = (DWORD)(right_left * WHEEL_DELTA );
				hasScroll2 = false;
			} else {
				inputs[used_inputs].mi.mouseData = 0;
			}
		}

		inputs[used_inputs].mi.time = 0; // auto time
		inputs[used_inputs].mi.dwExtraInfo = 0;
		hidg.buttons = old_buttons;
		used_inputs++;
	} while( has_x || hasScroll1 || hasScroll2 );
	//lprintf( "Generating events: %d %d %d", used_inputs, inputs[0].mi.dx, inputs[0].mi.dy  );
	SendInput( used_inputs, inputs, sizeof( INPUT ) );
}

void MouseObject::clickAt( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	POINT pt;
	GetCursorPos( &pt );
	double fScreenWidth = GetSystemMetrics( SM_CXVIRTUALSCREEN );
	double fScreenHeight = GetSystemMetrics( SM_CYVIRTUALSCREEN );

	int64_t x = args[0].As<Number>()->IntegerValue( context ).ToChecked();
	int64_t y = args[1].As<Number>()->IntegerValue( context ).ToChecked();

	double const sx = 65535.0f / fScreenWidth;
	double const sy = 65535.0f / fScreenHeight;
	double fx = x * sx;
	double fy = y * sy;
	//lprintf( "Clicking at %d %d   %1.4g %1.4g", x, y, fx, fy );
	INPUT inputs[3];

	inputs[0].type = INPUT_MOUSE;
	inputs[0].mi.dx = (LONG)ceil(fx);
	inputs[0].mi.dy = (LONG)ceil(fy);
	inputs[0].mi.dwFlags = MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN;
	inputs[0].mi.mouseData = 0; // scroll wheel
	inputs[0].mi.time = 0; // auto time
	inputs[0].mi.dwExtraInfo = 0;
	
	inputs[1].type = INPUT_MOUSE;
	inputs[1].mi.dx = 0;
	inputs[1].mi.dy = 0;
	inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
	inputs[1].mi.mouseData = 0; // scroll wheel
	inputs[1].mi.time = 0; // auto time
	inputs[1].mi.dwExtraInfo = 0;

	inputs[2].type = INPUT_MOUSE;
	inputs[2].mi.dx = (LONG)ceil(sx * pt.x);
	inputs[2].mi.dy = (LONG)ceil(sy * pt.y);
	inputs[2].mi.dwFlags = MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
	inputs[2].mi.mouseData = 0; // scroll wheel
	inputs[2].mi.time = 0; // auto time
	inputs[2].mi.dwExtraInfo = 0;
	SendInput( 3, inputs, sizeof( INPUT ) );


}

#endif
