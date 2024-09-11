
#include "../global.h"

#include <winsvc.h>

// so we get the correct import/export attributes on the function
//#include <service_hook.h>

static struct {
	CTEXTSTR next_service_name;
	void (CPROC*Start)(void);
	void (CPROC*Stop)(void);
	uintptr_t (CPROC*StartThread)(PTHREAD);
	uintptr_t psvStartThread;
	PTHREAD main_thread;


	Local<Function> startFunc;
	Local<Function> stopFunc;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback

	
} local_service_info;
#define l local_service_info

static SERVICE_STATUS ServiceStatus;
static SERVICE_STATUS_HANDLE hStatus;

static void asyncmsg( uv_async_t* handle ) {
	
}

static void ControlHandler( DWORD request )
{
	//lprintf( "Received %08x", request );
	switch(request) 
	{ 
		case SERVICE_CONTROL_STOP: 
			//WriteToLog("Monitoring stopped.");
			if( l.Stop )
				l.Stop();

			ServiceStatus.dwWin32ExitCode = 0; 
			ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
			SetServiceStatus (hStatus, &ServiceStatus);
			WakeThread( l.main_thread );
			return; 
 
		case SERVICE_CONTROL_SHUTDOWN: 
			//WriteToLog("Monitoring stopped.");
			if( l.Stop )
				l.Stop();

			ServiceStatus.dwWin32ExitCode = 0; 
			ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
			SetServiceStatus (hStatus, &ServiceStatus);
			WakeThread( l.main_thread );
			return; 

	case SERVICE_CONTROL_PAUSE:
		break;
	//case SERVICE_CONTROL_RESUME:
		default:
			break;
	 } 
 
	 // Report current status
	 SetServiceStatus (hStatus, &ServiceStatus);
 
	 return; 
}


static void APIENTRY ServiceMain( DWORD argc, TEXTCHAR **argv )
{

	int error; 
 
	ServiceStatus.dwServiceType = 
		SERVICE_WIN32; 
	ServiceStatus.dwCurrentState = 
		SERVICE_START_PENDING; 
	ServiceStatus.dwControlsAccepted	=  
		SERVICE_ACCEPT_STOP | 
		SERVICE_ACCEPT_SHUTDOWN;

	ServiceStatus.dwWin32ExitCode = 0; 
	ServiceStatus.dwServiceSpecificExitCode = 0; 
	ServiceStatus.dwCheckPoint = 0; 
	ServiceStatus.dwWaitHint = 0; 

	//lprintf( "Register service handler..." );
	hStatus = RegisterServiceCtrlHandler(
		l.next_service_name,
		(LPHANDLER_FUNCTION)ControlHandler); 
	if (hStatus == (SERVICE_STATUS_HANDLE)0) 
	{ 
		// Registering Control Handler failed
		lprintf( "Failed." );
		return; 
	}

	// Initialize Service
	if( l.Start )
	{
		
		//lprintf( "Send the initialize..." );
		l.Start();
	}

	//lprintf( "Startup completd." );

	error = 0;//InitService();
	if (error) 
	{
		// Initialization failed
		ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
		ServiceStatus.dwWin32ExitCode = -1; 
		SetServiceStatus(hStatus, &ServiceStatus); 
		return; 
	} 
	// We report the running status to SCM. 
	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus (hStatus, &ServiceStatus);

	l.main_thread = MakeThread();

	//MEMORYSTATUS memory;
	// The worker loop of a service
	while (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		WakeableSleep( 100000 );
	}
	return; 
}


void SetupService( TEXTSTR name, void (CPROC*Start)( void ) )
{
	SERVICE_TABLE_ENTRY ServiceTable[2];
	l.Start = Start;
	l.next_service_name = name;
	ServiceTable[0].lpServiceName = name;
	ServiceTable[0].lpServiceProc = ServiceMain;
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	//lprintf( "Send to startup monitor.." );
	if( !StartServiceCtrlDispatcher( ServiceTable ) )
		lprintf( "Startup monitor failed(1)! %d", GetLastError() );
}

void SetupServiceEx( TEXTSTR name, void (CPROC*Start)( void ), void (CPROC*Stop)( void ) )
{
	SERVICE_TABLE_ENTRY ServiceTable[2];
	l.Start = Start;
	l.Stop = Stop;
	l.next_service_name = name;
	ServiceTable[0].lpServiceName = name;
	ServiceTable[0].lpServiceProc = ServiceMain;
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	//lprintf( "Send to startup monitor.." );
	if( !StartServiceCtrlDispatcher( ServiceTable ) )
		lprintf( "Startup monitor failed(2)! %d", GetLastError() );
}

void SetupServiceThread( TEXTSTR name, uintptr_t (CPROC*Start)( PTHREAD ), uintptr_t psv )
{
	SERVICE_TABLE_ENTRY ServiceTable[2];
	l.StartThread = Start;
	l.psvStartThread = psv;
	l.next_service_name = name;
	ServiceTable[0].lpServiceName = name;
	ServiceTable[0].lpServiceProc = ServiceMain;
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	//lprintf( "Send to startup monitor.." );
	if( !StartServiceCtrlDispatcher( ServiceTable ) )
		lprintf( "Startup monitor failed(3)! %d", GetLastError() );

}

//---------------------------------------------------------------------------

LOGICAL IsThisAService( void )
{
	TEXTCHAR buffer[256];
	DWORD length = 0;
	GetUserObjectInformation( GetProcessWindowStation(), UOI_NAME, buffer, 256, &length);
	if (!StrCaseCmp(buffer, "WinSta0"))
	{
		// normal user session
		return FALSE;
	}
	else
	{
		// service session
		return TRUE;
	}

	return (l.Start != NULL) || (l.StartThread != NULL );
}

//---------------------------------------------------------------------------

static int task_done;
static void CPROC MyTaskEnd( uintptr_t psv, PTASK_INFO task )
{
	task_done = 1;
}

static void CPROC GetOutput( uintptr_t psv, PTASK_INFO task, CTEXTSTR buffer, size_t length )
{
	lprintf( "%s", buffer );
}

//---------------------------------------------------------------------------

void ServiceInstallEx( CTEXTSTR ServiceName, CTEXTSTR descrip, CTEXTSTR extraArgs )
{
	TEXTCHAR **args;
	int nArgs;
	PVARTEXT pvt_cmd = VarTextCreate();

	CTEXTSTR pname = GetProgramName();
	CTEXTSTR ppath = GetProgramPath();

	vtprintf( pvt_cmd,  "sc create \"%s\" start= auto binpath= \"" 
			  , ServiceName
			  );

	if( StrChr( pname, ' ' ) || StrChr( ppath, ' ' ) )
		vtprintf( pvt_cmd, "\\\"%s\\%s\\\"", ppath, pname );
	else
		vtprintf( pvt_cmd, "%s\\%s", ppath, pname );
	if( extraArgs && extraArgs[0] ) {
		vtprintf( pvt_cmd, " %s", extraArgs );
	}
	vtprintf( pvt_cmd, "\"" );
	//lprintf( "%s", GetText( VarTextPeek( pvt_cmd ) ) );
	ParseIntoArgs( GetText( VarTextPeek( pvt_cmd ) ), &nArgs, &args );
	/*
	{
		int n;
		for( n = 0; n < nArgs; n++ )
			lprintf( "parsed arg [%d]=%s", n, args[n] );
	}
	*/
	VarTextEmpty( pvt_cmd );
	LaunchPeerProgram( "sc.exe", NULL, (PCTEXTSTR)args,  GetOutput, MyTaskEnd, 0 );
	while( !task_done )
		WakeableSleep( 100 );
	task_done = 0;
	{
		CTEXTSTR args_tmp[5];
		args_tmp[0] = "sc";
		args_tmp[1] = "description";
		args_tmp[2] = pname;
		args_tmp[3] = descrip;
		args_tmp[4] = NULL;

		LaunchPeerProgram( "sc.exe", NULL, (PCTEXTSTR)args_tmp, GetOutput, MyTaskEnd, 0 );
		while( !task_done )
			WakeableSleep( 100 );
		task_done = 0;
	}


	vtprintf( pvt_cmd,  "sc start \"%s\"" 
			  , ServiceName );
	ParseIntoArgs( GetText( VarTextPeek( pvt_cmd ) ), &nArgs, &args );
	VarTextEmpty( pvt_cmd );
	LaunchPeerProgram( "sc.exe", NULL, (PCTEXTSTR)args,  GetOutput, MyTaskEnd, 0 );
	while( !task_done )
		WakeableSleep( 100 );
}

void ServiceInstall( CTEXTSTR ServiceName )
{
	ServiceInstallEx( ServiceName, NULL, NULL );
}

//---------------------------------------------------------------------------

void ServiceUninstall( CTEXTSTR ServiceName )
{
	TEXTCHAR **args;
	int nArgs;
	PVARTEXT pvt_cmd = VarTextCreate();
	vtprintf( pvt_cmd,  "sc stop \"%s\"" 
			  , ServiceName );
	ParseIntoArgs( GetText( VarTextPeek( pvt_cmd ) ), &nArgs, &args );
	VarTextEmpty( pvt_cmd );
	LaunchPeerProgram( "sc", NULL, (PCTEXTSTR)args,  GetOutput, MyTaskEnd, 0 );
	while( !task_done )
		WakeableSleep( 100 );
	task_done = 0;
	vtprintf( pvt_cmd, "sc delete \"%s\"" 
			  , ServiceName );
	ParseIntoArgs( GetText( VarTextPeek( pvt_cmd ) ), &nArgs, &args );
	VarTextEmpty( pvt_cmd );
	LaunchPeerProgram( "sc", NULL, (PCTEXTSTR)args,  GetOutput, MyTaskEnd, 0 );
	while( !task_done )
		WakeableSleep( 100 );
	task_done = 0;
}


static uintptr_t serviceMonitor( PTHREAD thread ) {
	SERVICE_TABLE_ENTRY ServiceTable[2];
	l.StartThread = Start;
	l.psvStartThread = psv;
	l.next_service_name = name;
	ServiceTable[0].lpServiceName = name;
	ServiceTable[0].lpServiceProc = ServiceMain;
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	//lprintf( "Send to startup monitor.." );
	if( !StartServiceCtrlDispatcher( ServiceTable ) )
		lprintf( "Startup monitor failed(3)! %d", GetLastError() );
	
	return 0;
}

static void startService( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate *isolate = args->GetIsolate();

	if( args.Length() < 1 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, TranslateText( "Must specify a callback to start service" ) ) ) );
	}
	Local<Function> start = args[0].As<Function>();
	l.startFunc = start;	

	class constructorSet *c = getConstructors( isolate );
	uv_async_init( c->loop, &c->serviceAsync, asyncmsg );
	ThreadTo( serviceMonitor, 0 );
}

void InitService( Isolate* isolate, Local<Object> exports ) {

	Local<Context> context = isolate->GetCurrentContext();

	Local<Object> serviceMethods = Object::New( isolate );

	SET( serviceMethods, "start", startService );
	Local<Function> serviceFunc = configTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
	//class constructorSet* c = getConstructors( isolate );
	//c->ConfigObject_constructor.Reset( isolate, configfunc );


	SET( exports, "Service", serviceMethods );


}

