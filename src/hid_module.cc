
#include "global.h"

//#define LoG(...)
#define LoG lprintf


static uintptr_t InputThread( PTHREAD thread );
static void CPROC dispatchKey( uintptr_t psv, RAWINPUT *event, WCHAR ch, int len );
#define WM_HOOK2 WM_USER+512

class KeyHidObject : public node::ObjectWrap {
public:
	int handle;
	char *name;

	Persistent<Function, CopyablePersistentTraits<Function>> *readCallback; //
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE readQueue;

public:

	static void Init( Isolate *isolate, Local<Object> exports );
	KeyHidObject(  );

private:
	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void onRead( const v8::FunctionCallbackInfo<Value>& args );
	static void writeCom( const v8::FunctionCallbackInfo<Value>& args );
	static void closeCom( const v8::FunctionCallbackInfo<Value>& args );
	~KeyHidObject();
};


struct input_data {
	HANDLE hDevice;
	char *name;
	Eternal<String> v8name;
};

typedef struct global_tag
{
	HWND hWnd;
	uint32_t nWriteTimeout;
	uv_loop_t* loop;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	KeyHidObject *eventHandler;
	PLIST inputs;
	HHOOK hookHandle;
	HHOOK hookHandleLL;
	int skipEvent;
} GLOBAL;

static GLOBAL hidg;

static RAWINPUTDEVICE devices[] = {
	{ 1, 6, RIDEV_NOLEGACY | RIDEV_INPUTSINK,  0 }
};

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	UINT uSize;

	switch( uMsg ) {
	case WM_CREATE:
		/* 1 - generic desktop controls // we use this
		2 - simulation controls
		3 - vr
		4 - sport
		5 - game 
		6 - generic device
		7 - keyboard
		8 - LEDs
		9 - button
		*/
		/*
		// for page 1 (above)
		0 - undefined
		1 - pointer
		2 - mouse
		3 - reserved
		4 - joystick
		5 - game pad
		6 - keyboard // we use this
		7 - keypad
		8 - multi-axis controller
		9 - Tablet PC controls
		*/
		devices[0].hwndTarget = hWnd;
		int rc;rc = RegisterRawInputDevices( devices, 1, sizeof( RAWINPUTDEVICE ) );
#if 0
		RAWINPUTDEVICELIST *devices;
		UINT uSize; uSize = sizeof( devices );
		UINT nDev;
		UINT n;
		nDev = 0;
		/*
		RAWINPUTDEVICE *devices2;
		GetRegisteredRawInputDevices( NULL, &nDev, sizeof( RAWINPUTDEVICE ) );
		devices2 = NewArray( RAWINPUTDEVICE, nDev );
		GetRegisteredRawInputDevices( devices2, &nDev, sizeof( RAWINPUTDEVICE ) );
		*/

		GetRawInputDeviceList( NULL, &nDev, sizeof( RAWINPUTDEVICELIST ) );
		lprintf( "%d", GetLastError());
		devices = NewArray( RAWINPUTDEVICELIST, nDev );
		GetRawInputDeviceList( devices, &nDev, sizeof( RAWINPUTDEVICELIST ) );
		lprintf( "%d", GetLastError() );
		for( n = 0; n < nDev; n++ ) {
			struct input_data *indev;
			UINT uSize;
			indev = NewArray( struct input_data, 1 );
			indev->hDevice = devices[n].hDevice;
			GetRawInputDeviceInfo(
				devices[n].hDevice,
				RIDI_DEVICENAME,
				NULL,
				&uSize
			);
			indev->name = NewArray( char, uSize );
			GetRawInputDeviceInfo(
				devices[n].hDevice,
				RIDI_DEVICENAME,
				indev->name,
				&uSize
			);
			lprintf( "New Device: %s", indev->name );
			AddLink( &hidg.inputs, indev );
		}
#endif
		//SetTimer( hWnd, 100, 1000, NULL );
		return TRUE;
	case WM_HOOK2:
		lprintf( "HOOK2 MSG" );
		return TRUE;;;;;;
	case WM_INPUT:
		if( GetRawInputData( (HRAWINPUT)lParam,
			RID_INPUT, NULL, &uSize, sizeof( RAWINPUTHEADER ) ) == -1 ) {
			break;
		}
		LPBYTE lpb = new BYTE[uSize];
		if( lpb == NULL ) {
			break;
		}
		if( GetRawInputData( (HRAWINPUT)lParam,
			RID_INPUT, lpb, &uSize, sizeof( RAWINPUTHEADER ) ) != uSize ) {
			delete[] lpb;
			break;
		}
		RAWINPUT *input = (RAWINPUT *)lpb;
#if 0
#define RI_KEY_MAKE             0
#define RI_KEY_BREAK            1
#define RI_KEY_E0               2
#define RI_KEY_E1               4
#define RI_KEY_TERMSRV_SET_LED  8
#define RI_KEY_TERMSRV_SHADOW   0x10
#endif

		UINT Event;
		WCHAR keyChar;

		Event = input->data.keyboard.Message;
		keyChar = MapVirtualKey( input->data.keyboard.VKey, MAPVK_VK_TO_CHAR );
		{
			INDEX idx;
			struct input_data *indev;
			LIST_FORALL( hidg.inputs, idx, struct input_data *, indev ) {
				if( indev->hDevice == input->header.hDevice )
					break;
			}
			if( !indev ) {
				UINT uSize;
				indev = NewArray( struct input_data, 1 );
				memset( indev, 0, sizeof( *indev ) );
				indev->hDevice = input->header.hDevice;
				GetRawInputDeviceInfo(
					input->header.hDevice,
					RIDI_DEVICENAME,
					NULL,
					&uSize
				);
				indev->name = NewArray( char, uSize );
				GetRawInputDeviceInfo(
					input->header.hDevice,
					RIDI_DEVICENAME,
					indev->name,
					&uSize
				);
				lprintf( "New Device: %s", indev->name );
				AddLink( &hidg.inputs, indev );
			}
		}
		dispatchKey( (uintptr_t)hidg.eventHandler, input, keyChar, 0 );
		//if(0)
		LoG( "Got: %c(%d) %p %d %d %d %d %d %d %d ", keyChar,keyChar
			, input->header.hDevice
			, input->header.wParam
			, input->data.keyboard.Reserved, input->data.keyboard.ExtraInformation,
			input->data.keyboard.MakeCode,
			input->data.keyboard.Flags,
			input->data.keyboard.VKey,
			input->data.keyboard.Message );

		delete[] lpb;			// free this now
			
		return TRUE;
	}
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

int MakeProxyWindow( void )
{
	ATOM aClass;
	{
		WNDCLASS wc;
		memset( &wc, 0, sizeof( WNDCLASS ) );
		wc.style = CS_OWNDC | CS_GLOBALCLASS;

		wc.lpfnWndProc = (WNDPROC)WndProc;
		wc.hInstance = GetModuleHandle( NULL );
		wc.hbrBackground = 0;
		wc.lpszClassName = "sack.vfs.raw.input.receiver.class";
		aClass = RegisterClass( &wc );
		if( !aClass ) {
			MessageBox( NULL, "Failed to register class to handle SQL Proxy messagses.", "INIT FAILURE", MB_OK );
			return FALSE;
		}
	}

	hidg.hWnd = CreateWindowEx( 0,
		(char*)aClass,
		"sack.vfs.raw.input.receiver",
		0,
		0,
		0,
		0,
		0,
		HWND_MESSAGE, // Parent
		NULL, // Menu
		GetModuleHandle( NULL ),
		(void*)1 );
	if( !hidg.hWnd ) {
		Log( "Failed to create window!?!?!?!" );
		MessageBox( NULL, "Failed to create window to handle raw input Messages", "INIT FAILURE", MB_OK );
		return FALSE;
	}
	return TRUE;
}

LRESULT WINAPI KeyboardProc( int code, WPARAM wParam, LPARAM lParam ) {
	KBDLLHOOKSTRUCT *kbhook = (KBDLLHOOKSTRUCT*)lParam;
	lprintf( "   keyhook for key... %08x  %d %d %x", wParam, kbhook->scanCode, kbhook->vkCode, kbhook->flags );
	MSG msg;
	while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )  lprintf( "had message..." );
	return CallNextHookEx( hidg.hookHandle, code, wParam, lParam );
}


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
	if( up ) {
		if( lastDownSkip ) {
			States[n].eventsUp[States[n].state].type = INPUT_KEYBOARD;
			States[n].eventsUp[States[n].state].ki.dwExtraInfo = kbhook->dwExtraInfo;
			States[n].eventsUp[States[n].state].ki.dwFlags = kbhook->flags;
			States[n].eventsUp[States[n].state].ki.time = kbhook->time;
			States[n].eventsUp[States[n].state].ki.wScan = (WORD)kbhook->scanCode;
			States[n].eventsUp[States[n].state].ki.wVk = (WORD)kbhook->vkCode;
			lastDownSkip--;
			return 1;
		}
		else {
			return CallNextHookEx( hidg.hookHandleLL, code, wParam, lParam );
		}
	}
	if( kbhook->vkCode == 'P' ) {
		for( n = 0; n < 10; n++ ) {
			if( !States[n].state ) {
				States[n].events[States[n].state].type = INPUT_KEYBOARD;
				States[n].events[States[n].state].ki.dwExtraInfo = kbhook->dwExtraInfo;
				States[n].events[States[n].state].ki.dwFlags = kbhook->flags;
				States[n].events[States[n].state].ki.time = kbhook->time;
				States[n].events[States[n].state].ki.wScan = (WORD)kbhook->scanCode;
				States[n].events[States[n].state].ki.wVk = (WORD)kbhook->vkCode;

				States[n].tick = GetTickCount();
				LoG( "P KEY set state in %d", n );
				States[n].state = 1;
				break;
			}
		}
	}
	else if( kbhook->vkCode >= '0' && kbhook->vkCode <= '9' ) {
		for( n = 0; n < 10; n++ ) {
			if( States[n].state == 1 ) {
				States[n].events[States[n].state].type = INPUT_KEYBOARD;
				States[n].events[States[n].state].ki.dwExtraInfo = kbhook->dwExtraInfo;
				States[n].events[States[n].state].ki.dwFlags = kbhook->flags;
				States[n].events[States[n].state].ki.time = kbhook->time;
				States[n].events[States[n].state].ki.wScan = (WORD)kbhook->scanCode;
				States[n].events[States[n].state].ki.wVk = (WORD)kbhook->vkCode;

				States[n].tick = GetTickCount();
				LoG( "# KEY set state in %d", n );
				States[n].state = 2;
				break;
			}
			if( States[n].state == 2 ) {
				States[n].events[States[n].state].type = INPUT_KEYBOARD;
				States[n].events[States[n].state].ki.dwExtraInfo = kbhook->dwExtraInfo;
				States[n].events[States[n].state].ki.dwFlags = kbhook->flags;
				States[n].events[States[n].state].ki.time = kbhook->time;
				States[n].events[States[n].state].ki.wScan = (WORD)kbhook->scanCode;
				States[n].events[States[n].state].ki.wVk = (WORD)kbhook->vkCode;

				States[n].tick = GetTickCount();
				LoG( "# KEY set state in %d", n );
				States[n].state = 3;
				break;
			}
			if( States[n].state == 4 ) {
				States[n].events[States[n].state].type = INPUT_KEYBOARD;
				States[n].events[States[n].state].ki.dwExtraInfo = kbhook->dwExtraInfo;
				States[n].events[States[n].state].ki.dwFlags = kbhook->flags;
				States[n].events[States[n].state].ki.time = kbhook->time;
				States[n].events[States[n].state].ki.wScan = (WORD)kbhook->scanCode;
				States[n].events[States[n].state].ki.wVk = (WORD)kbhook->vkCode;

				States[n].tick = GetTickCount();
				LoG( "# KEY set state in %d", n );
				States[n].state = 5;
				break;
			}
			if( States[n].state == 5 ) {
				States[n].events[States[n].state].type = INPUT_KEYBOARD;
				States[n].events[States[n].state].ki.dwExtraInfo = kbhook->dwExtraInfo;
				States[n].events[States[n].state].ki.dwFlags = kbhook->flags;
				States[n].events[States[n].state].ki.time = kbhook->time;
				States[n].events[States[n].state].ki.wScan = (WORD)kbhook->scanCode;
				States[n].events[States[n].state].ki.wVk = (WORD)kbhook->vkCode;

				States[n].tick = GetTickCount();
				LoG( "# KEY set state in %d", n );
				States[n].state = 6;
				break;
			}
		}
	}
	else if( kbhook->vkCode == VK_RETURN ) {
		for( n = 0; n < 10; n++ ) {
			if( States[n].state == 6 ) {
				States[n].events[States[n].state].type = INPUT_KEYBOARD;
				States[n].events[States[n].state].ki.dwExtraInfo = kbhook->dwExtraInfo;
				States[n].events[States[n].state].ki.dwFlags = kbhook->flags;
				States[n].events[States[n].state].ki.time = kbhook->time;
				States[n].events[States[n].state].ki.wScan = (WORD)kbhook->scanCode;
				States[n].events[States[n].state].ki.wVk = (WORD)kbhook->vkCode;

				States[n].tick = GetTickCount();
				LoG( "\r KEY set state in %d", n );
				States[n].state = 0;
				break;
			}
		}
	}
	else if( kbhook->vkCode == 'T' ) {
		for( n = 0; n < 10; n++ ) {
			if( States[n].state  == 3) {
				States[n].events[States[n].state].type = INPUT_KEYBOARD;
				States[n].events[States[n].state].ki.dwExtraInfo = kbhook->dwExtraInfo;
				States[n].events[States[n].state].ki.dwFlags = kbhook->flags;
				States[n].events[States[n].state].ki.time = kbhook->time;
				States[n].events[States[n].state].ki.wScan = (WORD)kbhook->scanCode;
				States[n].events[States[n].state].ki.wVk = (WORD)kbhook->vkCode;

				States[n].tick = GetTickCount();
				LoG( "T KEY set state in %d", n );
				States[n].state = 4;
				break;
			}
		}
	}
	else {
		for( n = 0; n < 10; n++ ) {
			if( States[n].state ) {
				LoG( "pending state...%d", n );
				if( (GetTickCount() - States[n].tick) > 250 ) {
					int e;
					LoG( "Expired %d", States[n].state );
					for( e = 0; e < States[n].state; e++ ) {
						LoG( "Call next with what would have been?" );
						//RawInput()
						resending = 1;
						States[n].events[e].ki.time = GetTickCount();
						SendInput( 1, States[n].events + e, sizeof( INPUT ) );
						States[n].eventsUp[e].ki.time = GetTickCount();
						SendInput( 1, States[n].eventsUp + e, sizeof( INPUT ) );
						resending = 0;
						//CallNextHookEx( hidg.hookHandleLL, States[n].pending[e].code, States[n].pending[e].wParam, (LPARAM)&States[n].pending[e].lParam );
					}
					LoG( "State has been pending for tooo long. %d", n );
					States[n].state = 0;
				}
				else {
					LoG( "state pending, mismatch key; flush state?" );
				}
			}
		}
	}
	if( n < 10 ) {
		hidg.skipEvent = 1;
		if( 0 ) {
			LoG( "Drop key." );
			lastDownSkip++;
			return 1;
		}
	}
	//SendMessage( hidg.hWnd, WM_HOOK2, 0, 0 );
	//LoG( "LL keyhook for key... %08x  %d %d %x", wParam, kbhook->scanCode, kbhook->vkCode, kbhook->flags );
	MSG msg;
	while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) { DispatchMessage( &msg );  lprintf( "had LL message..." ); }
	return CallNextHookEx( hidg.hookHandleLL, code, wParam, lParam );
}

uintptr_t InputThread( PTHREAD thread )
{
	// --- InstallHook (HookingRawInputDemoDLL.cpp) ---
	// Register keyboard Hook
	//WH_KEYBOARD_LL
	HMODULE xx;
	xx = GetModuleHandle( "sack_vfs.node" );
	//lprintf( "%p", xx );
	hidg.hookHandleLL = SetWindowsHookEx( WH_KEYBOARD_LL, (HOOKPROC)KeyboardProcLL, xx, 0 );
	hidg.hookHandle = SetWindowsHookEx( WH_KEYBOARD, KeyboardProc, xx, 0 );
	//lprintf( "hook:%p %d", hidg.hookHandle, GetLastError() );
	hidg.nWriteTimeout = 150; // at 9600 == 144 characters
	if( MakeProxyWindow() ) {
		MSG msg;
		//lprintf( "Err:%d",GetLastError());
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
	MSG msg;
	// create message queue on main thread.
	PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );
	ThreadTo( InputThread, 0 );
}

KeyHidObject::~KeyHidObject() {
}


void KeyHidObject::Init( Isolate *isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> comTemplate;

	comTemplate = FunctionTemplate::New( isolate, New );
	comTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.KeyHidEvents", v8::NewStringType::kNormal ).ToLocalChecked() );
	comTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

																 // Prototype
	NODE_SET_PROTOTYPE_METHOD( comTemplate, "onKey", onRead );

	class constructorSet* c = getConstructors( isolate );

	c->KeyHidObject_constructor.Reset( isolate, comTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	SET( exports, "Keyboard"
		, comTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
}

void KeyHidObjectInit( Isolate *isolate, Local<Object> exports ) {
	KeyHidObject::Init( isolate, exports );

}

struct msgbuf {
	RAWINPUT event;
	WCHAR ch;
};

void asyncmsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	Local<Context> context = isolate->GetCurrentContext();

	KeyHidObject* myself = (KeyHidObject*)handle->data;

	HandleScope scope( isolate );
	{
		struct msgbuf *msg;
		while( msg = (struct msgbuf *)DequeLink( &myself->readQueue ) ) {
			Local<Object> eventObj = Object::New( isolate );
			struct input_data *indev;
			INDEX idx;
			LIST_FORALL( hidg.inputs, idx, struct input_data *, indev ) {
				if( indev->hDevice == msg->event.header.hDevice ) {
					if( indev->name ) {
						indev->v8name.Set( isolate, String::NewFromUtf8( isolate, indev->name, v8::NewStringType::kNormal ).ToLocalChecked() );
						Release( indev->name );
						indev->name = NULL;
					}
					break;
				}
			}
			SET( eventObj, "down", (msg->event.data.keyboard.Flags&RI_KEY_BREAK)?True(isolate):False(isolate) );
			SET( eventObj, "char", String::NewFromTwoByte( isolate, (const uint16_t*)&msg->ch, NewStringType::kNormal, 1 ).ToLocalChecked() );
			SET( eventObj, "id", Number::New( isolate,(double)idx ) );
			SET( eventObj, "device", indev->v8name.Get(isolate) );


			Local<Value> argv[] = { eventObj };
			Local<Function> cb = Local<Function>::New( isolate, myself->readCallback[0] );
			{
				MaybeLocal<Value> result = cb->Call( context, isolate->GetCurrentContext()->Global(), 1, argv );
				if( result.IsEmpty() ) {
					Deallocate( struct msgbuf *, msg );
					return;
				}
			}
			Deallocate( struct msgbuf *, msg );
		}
	}
}

void KeyHidObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		KeyHidObject* obj = new KeyHidObject( );
		{

			MemSet( &obj->async, 0, sizeof( obj->async ) );

			class constructorSet *c = getConstructors( isolate );
			uv_async_init( c->loop, &obj->async, asyncmsg );
			obj->async.data = obj;

			obj->Wrap( args.This() );
			args.GetReturnValue().Set( args.This() );
		}

	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->KeyHidObject_constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), NULL, 0 ).ToLocalChecked() );
	}
}



void CPROC dispatchKey( uintptr_t psv, RAWINPUT *event, WCHAR ch, int len ) {
	struct msgbuf *msgbuf = NewPlus( struct msgbuf, len );
	msgbuf->event = event[0];
	msgbuf->ch = ch;

	KeyHidObject *com = (KeyHidObject*)psv;
	EnqueLink( &com->readQueue, msgbuf );
	uv_async_send( &com->async );
}


void KeyHidObject::onRead( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc < 1 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must pass callback to onRead handler", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}

	KeyHidObject *com = ObjectWrap::Unwrap<KeyHidObject>( args.This() );

	Local<Function> arg0 = Local<Function>::Cast( args[0] );
	com->readCallback = new Persistent<Function, CopyablePersistentTraits<Function>>( isolate, arg0 );
}


