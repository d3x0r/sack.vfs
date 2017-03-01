

class SqlObject : public node::ObjectWrap {
public:
	PODBC odbc;
   	int optionInitialized;
	static v8::Persistent<v8::Function> constructor;
	int columns;
	CTEXTSTR *result;
	CTEXTSTR *fields;
	//Persistent<Object> volume;
public:

	static void Init( Handle<Object> exports );
	SqlObject( const char *dsn, Isolate* isolate, Local<Object> o );

	static void New( const FunctionCallbackInfo<Value>& args );
	static void query( const FunctionCallbackInfo<Value>& args );
	static void option( const FunctionCallbackInfo<Value>& args );
	static void setOption( const FunctionCallbackInfo<Value>& args );
	static void makeTable( const FunctionCallbackInfo<Value>& args );
	static void closeDb( const FunctionCallbackInfo<Value>& args );
	static void commit( const FunctionCallbackInfo<Value>& args );
	static void transact( const FunctionCallbackInfo<Value>& args );
	static void autoTransact( const FunctionCallbackInfo<Value>& args );

	static void enumOptionNodes( const FunctionCallbackInfo<Value>& args );
	static void findOptionNode( const FunctionCallbackInfo<Value>& args );
	static void getOptionNode( const FunctionCallbackInfo<Value>& args );

   ~SqlObject();
};


class OptionTreeObject : public node::ObjectWrap {
public:
	POPTION_TREE_NODE node;
	SqlObject *db;
	static v8::Persistent<v8::Function> constructor;
	
public:

	static void Init( );
	OptionTreeObject(  );

	static void New( const FunctionCallbackInfo<Value>& args );

	static void enumOptionNodes( const FunctionCallbackInfo<Value>& args );
	static void findOptionNode( const FunctionCallbackInfo<Value>& args );
	static void getOptionNode( const FunctionCallbackInfo<Value>& args );
	static void writeOptionNode( v8::Local<v8::String> field,
		v8::Local<v8::Value> val,
		const PropertyCallbackInfo<void>&info );
	static void readOptionNode( v8::Local<v8::String> field,
		const PropertyCallbackInfo<v8::Value>& info );

   ~OptionTreeObject();
};




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
