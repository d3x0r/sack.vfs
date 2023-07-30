
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
#if _WIN32	
	static void getDisplays( const v8::FunctionCallbackInfo<Value>& args );
	static void moveWindow( const v8::FunctionCallbackInfo<Value>& args );
	static void refreshWindow( const v8::FunctionCallbackInfo<Value>& args );
	static void getWindowTitle( const v8::FunctionCallbackInfo<Value>& args );
#endif
};

