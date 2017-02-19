
class ComObject : public node::ObjectWrap {
public:
	int handle;
	char *name;
	static Persistent<Function> constructor;

	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE readQueue;
	
public:

	static void Init( Handle<Object> exports );
	ComObject( char *name );

	static void New( const FunctionCallbackInfo<Value>& args );

	static void onRead( const FunctionCallbackInfo<Value>& args );
	static void writeCom( const FunctionCallbackInfo<Value>& args );
	static void closeCom( const FunctionCallbackInfo<Value>& args );


   ~ComObject();
};


class RegObject : public node::ObjectWrap {
public:
	static Persistent<Function> constructor;

	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE readQueue;
	
public:

	static void Init( Handle<Object> exports );
	RegObject();

	static void New( const FunctionCallbackInfo<Value>& args );

	static void getRegItem( const FunctionCallbackInfo<Value>& args );
	static void setRegItem( const FunctionCallbackInfo<Value>& args );


   ~RegObject();
};
