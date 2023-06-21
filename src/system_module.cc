
#include "global.h"

static struct system_local {
	LOGICAL enabledExit;
	
}local;

static void openMemory( const v8::FunctionCallbackInfo<Value>& args ) {
  Isolate* isolate = args.GetIsolate();
  int hasWhat = args.Length() > 0;
  int hasWhere = args.Length() > 1;
  String::Utf8Value what( args.GetIsolate(), hasWhat ? args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() : Null( isolate )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
  String::Utf8Value where( args.GetIsolate(), hasWhere ? args[1]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() : Null( isolate )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
  size_t size = 0;
  POINTER p = OpenSpace( hasWhat ? *what : NULL, hasWhere ? *where : NULL, &size );
  if( p ) {
#if ( NODE_MAJOR_VERSION >= 14 )
    std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( p, size, releaseBufferBackingStore, NULL );
    Local<Object> arrayBuffer = ArrayBuffer::New( isolate, bs );
#else
    Local<Object> arrayBuffer = ArrayBuffer::New( isolate, p, size );
    PARRAY_BUFFER_HOLDER holder = GetHolder();
    holder->o.Reset( isolate, arrayBuffer );
    holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
    holder->buffer = p;
#endif
    args.GetReturnValue().Set( arrayBuffer );
  }
  else {
    args.GetReturnValue().Set( Null( isolate ) );
  }
}


static void createMemory( const v8::FunctionCallbackInfo<Value>& args ) {
  Isolate* isolate = args.GetIsolate();
  int hasWhat = args.Length() > 0;
  int hasWhere = args.Length() > 1;
  String::Utf8Value what( args.GetIsolate(), hasWhat ? args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() : Null( isolate )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
  String::Utf8Value where( args.GetIsolate(), hasWhere ? args[1]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() : Null( isolate )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
  size_t size = args.Length() > 2 ? args[2]->ToInt32( isolate->GetCurrentContext() ).ToLocalChecked()->Value() : 0;
  POINTER p = size ? OpenSpace( hasWhat ? *what : NULL, hasWhere ? *where : NULL, &size ) : NULL;
  if( p ) {
#if ( NODE_MAJOR_VERSION >= 14 )
    std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( p, size, releaseBufferBackingStore, NULL );
    Local<Object> arrayBuffer = ArrayBuffer::New( isolate, bs );
#else
    Local<Object> arrayBuffer = ArrayBuffer::New( isolate, p, size );
    PARRAY_BUFFER_HOLDER holder = GetHolder();
    holder->o.Reset( isolate, arrayBuffer );
    holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
    holder->buffer = p;
#endif
    args.GetReturnValue().Set( arrayBuffer );
  }
  else {
    args.GetReturnValue().Set( Null( isolate ) );
  }
}

static void dumpNames( const v8::FunctionCallbackInfo<Value>& args ) {
  int hasWhat = args.Length() > 0;
  Isolate* isolate = args.GetIsolate();
  String::Utf8Value what( args.GetIsolate(), hasWhat ? args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked()  : Null( isolate )->ToString( isolate->GetCurrentContext() ).ToLocalChecked()  );
  if( hasWhat )
    DumpRegisteredNamesFrom( (PCLASSROOT)*what );
  else
    DumpRegisteredNames();
}


static void enableThreadFS( const v8::FunctionCallbackInfo<Value>& args ) {
	sack_filesys_enable_thread_mounts();
}

static void allowSpawn( const v8::FunctionCallbackInfo<Value>& args ) {
	if( sack_system_allow_spawn() )
		args.GetReturnValue().Set( True( args.GetIsolate() ) );
	args.GetReturnValue().Set( False( args.GetIsolate() ) );

}

static void disallowSpawn( const v8::FunctionCallbackInfo<Value>& args ) {
	sack_system_disallow_spawn();
}

static void do_reboot( const char *mode ) {
#ifdef WIN32
	HANDLE hToken, hProcess;
	TOKEN_PRIVILEGES tp;

	if( DuplicateHandle( GetCurrentProcess(), GetCurrentProcess()
							 , GetCurrentProcess(), &hProcess, 0
							 , FALSE, DUPLICATE_SAME_ACCESS  ) )
	{
		if( OpenProcessToken( hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken ) )
		{
			tp.PrivilegeCount = 1;
			if( LookupPrivilegeValue( NULL
													 , SE_SHUTDOWN_NAME
													 , &tp.Privileges[0].Luid ) )
			{
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
				AdjustTokenPrivileges( hToken, FALSE, &tp, 0, NULL, NULL );
				// PTEXT temp;
				// DECLTEXT( msg, "Initiating system shutdown..." );
				// EnqueLink( &ps->Command->Output, &msg );
				// if( !(temp = GetParam( ps, &param ) ) )
            if( stricmp( mode, "shutdown" ) == 0 )
					ExitWindowsEx( EWX_SHUTDOWN|EWX_FORCE, 0 );
            else if( stricmp( mode, "reboot" ) == 0 )
					ExitWindowsEx( EWX_REBOOT|EWX_FORCE, 0 );
				else
               lprintf( "Mode specified invalid! (reboot/shutdown)\n" );
			}
			else
			{
				lprintf( "Failed to find privilege for shutdown:%d", GetLastError() );
			}
			CloseHandle( hToken );
		}
		else
		{
			lprintf( "Failed to open process token for reboot:%d", GetLastError() );
		}
		CloseHandle( hProcess );
	}
	else
		lprintf( "Failed to duplicate handle for reboot:%d", GetLastError() );
#endif
}

static void reboot( const v8::FunctionCallbackInfo<Value>& args ) {
  Isolate* isolate = args.GetIsolate();
  int hasWhat = args.Length() > 0;
	if( args.Length() > 0 )  {
	  String::Utf8Value mode( isolate, args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		do_reboot( *mode );
	}else 
		do_reboot( "reboot" );
	
}


static void dumpMemory( const v8::FunctionCallbackInfo<Value>& args ) {
	LOGICAL verbose = FALSE;
	String::Utf8Value *file;
	char *filename = NULL;
	if( args.Length() > 0 )  {
		verbose = args[0]->TOBOOL( args.GetIsolate() );
	}
	if( args.Length() > 1 ) {
		file = new String::Utf8Value( args.GetIsolate(), args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
		filename = *file[0];
	}
	if( filename )
		DebugDumpHeapMemFile( NULL, filename );
	else
		DebugDumpHeapMemEx( NULL, verbose );
}

CRITICALSECTION cs;
volatile int xx[32];

static uintptr_t  ThreadWrapper( void* pThread ){
	while( 1 ) {
		POINTER p = Allocate( 100 );
		EnterCriticalSecEx( &cs DBG_PASS );
		xx[(int)((uintptr_t)pThread)]++;
		LeaveCriticalSecEx( &cs DBG_PASS );
		Deallocate( POINTER, p );
	}
	return 0;
}

namespace sack {
	namespace timers {
		LOGICAL LeaveCriticalSecNoWakeEx( PCRITICALSECTION pcs DBG_PASS );
	}
}
static volatile int stopThread = 0;
static uintptr_t  ThreadWrapper2( PTHREAD pThread ){
	int id = (int)GetThreadParam( pThread );
	while( !stopThread ) {
		POINTER p = Allocate( 100 );
		xx[(int)id]++;
		/*
		if( EnterCriticalSecNoWaitEx( &cs, NULL DBG_PASS ) > 0 ) {
			LeaveCriticalSecNoWakeEx( &cs DBG_PASS );
		} else Relinquish();
		*/
		Deallocate( POINTER, p );
	}
	xx[id] = -xx[id];
	return 0;
}
#define TEST_THREADS 22
static void testCritSec( const v8::FunctionCallbackInfo<Value>& args ) {
	int n;
	InitializeCriticalSec( &cs );
	for( n = 0; n < TEST_THREADS; n++ )
	{
		ThreadTo( ThreadWrapper2, n );
	}
	Sleep( 5000 );
	stopThread = 1;
	for( n = 0; n < TEST_THREADS; n++ )
		if( xx[n] > 0 ) 
			n = -1;
	for( n = 0; n < TEST_THREADS; n++ )
		printf( "%d = %d\n", n, -xx[n] );
}

#ifdef _WIN32
static void create( const v8::FunctionCallbackInfo<Value>& args ) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset( &si, 0, sizeof( STARTUPINFO ) );
	si.cb = sizeof( STARTUPINFO );
	memset( &pi, 0, sizeof( PROCESS_INFORMATION ) );

	BOOL a = CreateProcess( NULL, "taskkill.exe /?"
		, NULL, NULL, TRUE
		, 0
		//                            | CREATE_NO_WINDOW
		//                            | DETACHED_PROCESS
		//                            | CREATE_NEW_PROCESS_GROUP
		, NULL
		, NULL
		, &si
		, &pi );
	printf( "Status:%d  %d", a, GetLastError() );
}

static int exitEvent( uintptr_t psv ) {
	class constructorSet* c = (class constructorSet*)psv;
	uv_async_send( &c->exitAsync );
	return 1; // don't exit yet; dispatch the event instead.
}

static void exitAsyncMsg( uv_async_t* handle ) {
	class constructorSet* c = (class constructorSet* )handle->data;
	HandleScope scope( c->isolate );
	if( !c->exitCallback.IsEmpty() ) {
		c->exitCallback.Get( c->isolate )->Call( c->isolate->GetCurrentContext(), Null( c->isolate ), 0, NULL );
	}
}

static void enableExitEvent( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( !local.enabledExit ) {
		EnableExitEvent();
		local.enabledExit = TRUE;
	}
	//lprintf( "Setting callback with isolate:%p", isolate );
	class constructorSet* c = getConstructors( isolate );
	AddKillSignalCallback( exitEvent, (uintptr_t)c );
	uv_async_init( c->loop, &c->exitAsync, exitAsyncMsg );
	c->exitAsync.data = c;
	if( args.Length() > 0 )
		c->exitCallback.Reset( isolate, Local<Function>::Cast( args[0]) );
}

HCURSOR hCursor;
int hidePos = 0;

uintptr_t hideCursorThread( PTHREAD thread ) {
	int *timeout = (int*)GetThreadParam( thread );
	int64_t now = timeGetTime();
	POINT oldPoint;
	POINT newPoint;
	GetCursorPos( &oldPoint );
	//lprintf( "Cursor at: %d %d", oldPoint.x, oldPoint.y );
	ShowCursor( TRUE );
	while( 1 ) {
		int64_t newNow = timeGetTime();
		//lprintf( "Tick: %d %d", newNow, newNow - now );
		GetCursorPos( &newPoint );
		//lprintf( "Cursor at: %d %d", newPoint.x, newPoint.y );
		if( ( newPoint.x != oldPoint.x ) || ( newPoint.y != oldPoint.y ) ) {
			oldPoint = newPoint;  // update the point position.
			now = newNow;
		} else

		if( ( newNow - now ) > timeout[0] ) {
			//lprintf( "Do Hide?" );
			//HCURSOR old = SetCursor( hCursor );
			SetCursorPos( 0, hidePos );
			//int r = ShowCursor( FALSE );
			//DWORD dwError = GetLastError();
			//lprintf( "Cursor R %d %d", old, dwError );
			now = newNow;
		}
		WakeableSleep( (uint32_t)( timeout[0] / 10 ) );
	}
}
/*
static void initBlankCursor( void ) {

	BYTE ANDmaskCursor[] =
	{
		0xFF, 0xFC, 0x3F, 0xFF,   // line 1 
		0xFF, 0xC0, 0x1F, 0xFF,   // line 2 
		0xFF, 0x00, 0x3F, 0xFF,   // line 3 
		0xFE, 0x00, 0xFF, 0xFF,   // line 4 

		0xF7, 0x01, 0xFF, 0xFF,   // line 5 
		0xF0, 0x03, 0xFF, 0xFF,   // line 6 
		0xF0, 0x03, 0xFF, 0xFF,   // line 7 
		0xE0, 0x07, 0xFF, 0xFF,   // line 8 

		0xC0, 0x07, 0xFF, 0xFF,   // line 9 
		0xC0, 0x0F, 0xFF, 0xFF,   // line 10 
		0x80, 0x0F, 0xFF, 0xFF,   // line 11 
		0x80, 0x0F, 0xFF, 0xFF,   // line 12 

		0x80, 0x07, 0xFF, 0xFF,   // line 13 
		0x00, 0x07, 0xFF, 0xFF,   // line 14 
		0x00, 0x03, 0xFF, 0xFF,   // line 15 
		0x00, 0x00, 0xFF, 0xFF,   // line 16 

		0x00, 0x00, 0x7F, 0xFF,   // line 17 
		0x00, 0x00, 0x1F, 0xFF,   // line 18 
		0x00, 0x00, 0x0F, 0xFF,   // line 19 
		0x80, 0x00, 0x0F, 0xFF,   // line 20 

		0x80, 0x00, 0x07, 0xFF,   // line 21 
		0x80, 0x00, 0x07, 0xFF,   // line 22 
		0xC0, 0x00, 0x07, 0xFF,   // line 23 
		0xC0, 0x00, 0x0F, 0xFF,   // line 24 

		0xE0, 0x00, 0x0F, 0xFF,   // line 25 
		0xF0, 0x00, 0x1F, 0xFF,   // line 26 
		0xF0, 0x00, 0x1F, 0xFF,   // line 27 
		0xF8, 0x00, 0x3F, 0xFF,   // line 28 

		0xFE, 0x00, 0x7F, 0xFF,   // line 29 
		0xFF, 0x00, 0xFF, 0xFF,   // line 30 
		0xFF, 0xC3, 0xFF, 0xFF,   // line 31 
		0xFF, 0xFF, 0xFF, 0xFF    // line 32 
	};

	// Yin-shaped cursor XOR mask 

	BYTE XORmaskCursor[] =
	{
		0x00, 0x00, 0x00, 0x00,   // line 1 
		0x00, 0x03, 0xC0, 0x00,   // line 2 
		0x00, 0x3F, 0x00, 0x00,   // line 3 
		0x00, 0xFE, 0x00, 0x00,   // line 4 

		0x0E, 0xFC, 0x00, 0x00,   // line 5 
		0x07, 0xF8, 0x00, 0x00,   // line 6 
		0x07, 0xF8, 0x00, 0x00,   // line 7 
		0x0F, 0xF0, 0x00, 0x00,   // line 8 

		0x1F, 0xF0, 0x00, 0x00,   // line 9 
		0x1F, 0xE0, 0x00, 0x00,   // line 10 
		0x3F, 0xE0, 0x00, 0x00,   // line 11 
		0x3F, 0xE0, 0x00, 0x00,   // line 12 

		0x3F, 0xF0, 0x00, 0x00,   // line 13 
		0x7F, 0xF0, 0x00, 0x00,   // line 14 
		0x7F, 0xF8, 0x00, 0x00,   // line 15 
		0x7F, 0xFC, 0x00, 0x00,   // line 16 

		0x7F, 0xFF, 0x00, 0x00,   // line 17 
		0x7F, 0xFF, 0x80, 0x00,   // line 18 
		0x7F, 0xFF, 0xE0, 0x00,   // line 19 
		0x3F, 0xFF, 0xE0, 0x00,   // line 20 

		0x3F, 0xC7, 0xF0, 0x00,   // line 21 
		0x3F, 0x83, 0xF0, 0x00,   // line 22 
		0x1F, 0x83, 0xF0, 0x00,   // line 23 
		0x1F, 0x83, 0xE0, 0x00,   // line 24 

		0x0F, 0xC7, 0xE0, 0x00,   // line 25 
		0x07, 0xFF, 0xC0, 0x00,   // line 26 
		0x07, 0xFF, 0xC0, 0x00,   // line 27 
		0x01, 0xFF, 0x80, 0x00,   // line 28 

		0x00, 0xFF, 0x00, 0x00,   // line 29 
		0x00, 0x3C, 0x00, 0x00,   // line 30 
		0x00, 0x00, 0x00, 0x00,   // line 31 
		0x00, 0x00, 0x00, 0x00    // line 32 
	};

	// Create a custom cursor at run time. 
	return ;
	hCursor = CreateCursor( GetModuleHandle( NULL ),   // app. instance 
		19,                // horizontal position of hot spot 
		2,                 // vertical position of hot spot 
		32,                // cursor width 
		32,                // cursor height 
		ANDmaskCursor,     // AND mask 
		XORmaskCursor );   // XOR mask 

	return;
	HBITMAP hBitmap = (HBITMAP)LoadImage( NULL,
		"test.bmp",
		IMAGE_BITMAP,
		0, 0,
		LR_LOADFROMFILE );
	BITMAP bmp;
	GetObject( hBitmap, sizeof( BITMAP ), &bmp );

	HBITMAP hMask = ::CreateCompatibleBitmap( GetDC( NULL ),
		bmp.bmWidth, bmp.bmHeight );
	if( hMask == INVALID_HANDLE_VALUE ) {
		lprintf( "Bad Handle" );
	}

	ICONINFO ii = { 0 };
	ii.fIcon = FALSE;
	ii.hbmColor = hBitmap;
	ii.hbmMask = hMask;
	ii.xHotspot = 0;
	ii.yHotspot = 0;
	//CreateCursor( GetInstance)
	hCursor = CreateIconIndirect( &ii );
	if( hCursor == INVALID_HANDLE_VALUE ) {
		lprintf( "Cursor is BAD!" );
	}
}
*/
static void hideCursor( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	static PTHREAD hideThread;
	static int defaultTimeout = 15000;
	if( !hidePos ) {
		RECT r;
		GetWindowRect( GetDesktopWindow(), &r );
		hidePos = r.bottom;
	}

	//if( !hCursor ) initBlankCursor();
	if( (args.Length() > 0) && args[0]->IsNumber() ) {
		// should be reset to empty when not in use...
		defaultTimeout = (int)args[0]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
	}
	if( !hideThread )
		hideThread = ThreadTo( hideCursorThread, (uintptr_t)&defaultTimeout);
}

#endif



void SystemInit( Isolate* isolate, Local<Object> exports )
{
  Local<Context> context = isolate->GetCurrentContext();
  Local<Object> systemInterface = Object::New( isolate );

  //regInterface->Set( String::NewFromUtf8Literal( isolate, "get" ),

  NODE_SET_METHOD( systemInterface, "enableThreadFileSystem", enableThreadFS );
  NODE_SET_METHOD( systemInterface, "allowSpawn", allowSpawn );
  NODE_SET_METHOD( systemInterface, "disallowSpawn", disallowSpawn );
  NODE_SET_METHOD( systemInterface, "openMemory", openMemory );
  NODE_SET_METHOD( systemInterface, "createMemory", createMemory );
  NODE_SET_METHOD( systemInterface, "dumpRegisteredNames", dumpNames );
  NODE_SET_METHOD( systemInterface, "reboot", reboot );
  NODE_SET_METHOD( systemInterface, "dumpMemory", dumpMemory );
  NODE_SET_METHOD( systemInterface, "testCritSec", testCritSec );
#ifdef _WIN32
  NODE_SET_METHOD( systemInterface, "createConsole", create );
  NODE_SET_METHOD( systemInterface, "enableExitSignal", enableExitEvent );
  NODE_SET_METHOD( systemInterface, "hideCursor", hideCursor );
#endif

  SET( exports, "system", systemInterface );

}


