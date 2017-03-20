#include <node.h>
#include <nan.h>
#include <node_object_wrap.h>
#include <v8.h>
#include <uv.h>
//#include <nan.h>

#include "src/sack.h"
#undef New

using namespace v8;


class VolumeObject : public node::ObjectWrap {
public:
	struct volume *vol;
	bool volNative;
	struct file_system_mounted_interface* fsMount;
	static v8::Persistent<v8::Function> constructor;
	
public:

	static void Init( Handle<Object> exports );
	VolumeObject( const char *mount, const char *filename, const char *key, const char *key2 );

	static void New( const FunctionCallbackInfo<Value>& args );
	static void getDirectory( const FunctionCallbackInfo<Value>& args );
	static void fileRead( const FunctionCallbackInfo<Value>& args );
	static void fileWrite( const FunctionCallbackInfo<Value>& args );
	static void fileExists( const FunctionCallbackInfo<Value>& args );

   ~VolumeObject();
};


class ThreadObject : public node::ObjectWrap {
public:
	static v8::Persistent<v8::Function> constructor;
   	static Persistent<Function, CopyablePersistentTraits<Function>> idleProc;
public:

	static void Init( Handle<Object> exports );
	ThreadObject();

	static void New( const FunctionCallbackInfo<Value>& args );

	static void relinquish( const FunctionCallbackInfo<Value>& args );
	static void wake( const FunctionCallbackInfo<Value>& args );

   ~ThreadObject();
};


class FileObject : public node::ObjectWrap {
	VolumeObject *vol;
	struct sack_vfs_file *file;
	FILE *cfile;
	//Local<Object> volume;
   char* buf;
   size_t size;
   Persistent<Object> volume;
public:
	static v8::Persistent<v8::Function> constructor;
	static void Init(  );

	static void openFile( const FunctionCallbackInfo<Value>& args );
	static void readFile( const FunctionCallbackInfo<Value>& args );
	static void writeFile( const FunctionCallbackInfo<Value>& args );
	static void seekFile( const FunctionCallbackInfo<Value>& args );
	static void truncateFile( const FunctionCallbackInfo<Value>& args );

	//static void readFile( const FunctionCallbackInfo<Value>& args );

	static void Emitter( const FunctionCallbackInfo<Value>& args );

	FileObject( VolumeObject* vol, const char *filename, Isolate*, Local<Object> o );
   ~FileObject();
};



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


void InitFS( const FunctionCallbackInfo<Value>& args );
