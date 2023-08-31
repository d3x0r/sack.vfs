
struct taskObjectOutputItem {
	size_t size;
	PTHREAD waiter;  // used to hold the buffer until received by script
	TEXTCHAR buffer[1];

};


class TaskObject : public node::ObjectWrap {
public:
	PTASK_INFO task;
	Persistent<Object> _this;
	Persistent<Function, CopyablePersistentTraits<Function>> endCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> inputCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> inputCallback2; //
#if _WIN32
	Persistent<Function, CopyablePersistentTraits<Function>> cbMove; // temporary value for move window
	Persistent<Object> moveOpts; // if style is used with move, style is applied first and then move.
	Persistent<Function, CopyablePersistentTraits<Function>> cbStyle; // temporary value for style window
#endif
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	bool binary;
	bool ending;
	bool ended;
	uint32_t exitCode;
	bool killAtExit;

	PLINKQUEUE output;
	PLINKQUEUE output2;

	bool stopped;
	bool killed;
#if _WIN32
	int moved;
	LOGICAL moveSuccess;
	int styled;
	int styleSuccess;
#endif

	TaskObject( );
	~TaskObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void Write( const v8::FunctionCallbackInfo<Value>& args );
	static void End( const v8::FunctionCallbackInfo<Value>& args );
	static void Terminate( const v8::FunctionCallbackInfo<Value>& args );
	static void isRunning( const v8::FunctionCallbackInfo<Value>& args );
	static void loadLibrary( const v8::FunctionCallbackInfo<Value>& args );
	static void getExitCode( const v8::FunctionCallbackInfo<Value>& args );
	static void GetProcessList( const FunctionCallbackInfo<Value>& args );
	static void StopProcess( const FunctionCallbackInfo<Value>& args );
	static void KillProcess( const FunctionCallbackInfo<Value>& args );
#if _WIN32	
	static void getDisplays( const v8::FunctionCallbackInfo<Value>& args );
	static void moveWindow( const v8::FunctionCallbackInfo<Value>& args ); // deferred set window position
	static void styleWindow( const FunctionCallbackInfo<Value>& args );   // deferred set window style
	static void refreshWindow( const v8::FunctionCallbackInfo<Value>& args );
	static void getWindowTitle( const v8::FunctionCallbackInfo<Value>& args ); // get title from TaskObject->task
	static void getProcessWindowPos( const FunctionCallbackInfo<Value>& args );  // get window position by task
	static void setProcessWindowStyles( const FunctionCallbackInfo<Value>& args );  // set task window styles (direct)
	static void getProcessWindowStyles( const FunctionCallbackInfo<Value>& args );  // get task window styles (direct)
#endif
};

