#if defined( _MSC_VER )
#  pragma warning( disable: 4251 4275 )
#endif

#include <node.h>
//#include <nan.h>
#include <node_object_wrap.h>
#include <v8.h>
#include <uv.h>

#if defined( _MSC_VER )
#  pragma warning( default: 4251 4275 )
#endif

#define V8_AT_LEAST(major, minor) (V8_MAJOR_VERSION > major || (V8_MAJOR_VERSION == major && V8_MINOR_VERSION >= minor))
//#include <nan.h>

#ifdef SACK_CORE
#include <stdhdrs.h>
#include <network.h>
//#include <idle.h>
#include <pssql.h>
#include <sack_vfs.h>
#include <salty_generator.h>
#include <filesys.h>
#include <deadstart.h>
#include <procreg.h>
#include <translation.h>
#include <json_emitter.h>
#include <jsox_parser.h>
#include <vesl_emitter.h>
#include <sqlgetoption.h>
#include <sackcomm.h>
#include <html5.websocket.h>
#include <html5.websocket.client.h>
#include <http.h>
#include <construct.h>
#else
#include "sack/sack.h"
#endif

#undef New

//#include <openssl/ssl.h>
#include <openssl/safestack.h>  // STACK_OF
#include <openssl/tls1.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
//#include <openssl/>

#ifdef INCLUDE_GUI
#include "gui/gui_global.h"

#endif


#if NODE_MAJOR_VERSION >= 10
#  define USE_ISOLATE(i)   (i),
#  define USE_ISOLATE_VOID(i)   (i)
#else
#  define USE_ISOLATE(i)
#  define USE_ISOLATE_VOID(i)
#endif

#if NODE_MAJOR_VERSION >= 13
#  define TOBOOL(i) BooleanValue( i )
#else
#  define TOBOOL(i) BooleanValue( i->GetCurrentContext() ).FromMaybe(false)
#endif

using namespace v8;

#include "task_module.h"

//fileObject->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8( isolate, "SeekSet" ), Integer::New( isolate, SEEK_SET ), ReadOnlyProperty );

#define SET_READONLY( object, name, data ) (object)->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8(isolate, name, v8::NewStringType::kNormal ).ToLocalChecked(), data, ReadOnlyProperty )
#define SET_READONLY_METHOD( object, name, method ) (object)->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8(isolate, name, v8::NewStringType::kNormal ).ToLocalChecked(), v8::Function::New(isolate->GetCurrentContext(), method ).ToLocalChecked(), ReadOnlyProperty )

#define GET(o,key)  (o)->Get( context, String::NewFromUtf8( isolate, (key), v8::NewStringType::kNormal ).ToLocalChecked() ).ToLocalChecked()
#define GETV(o,key)  (o)->Get( context, key ).ToLocalChecked()
#define GETN(o,key)  (o)->Get( context, Integer::New( isolate, (key) ) ).ToLocalChecked()
#define SETV(o,key,val)  (o)->Set( context, key, val )
#define SET(o,key,val)  (o)->Set( context, String::NewFromUtf8( isolate, (key), v8::NewStringType::kNormal ).ToLocalChecked(), val )
#define SETT(o,key,val)  (o)->Set( context, String::NewFromUtf8( isolate, GetText(key), v8::NewStringType::kNormal, (int)GetTextSize( key ) ).ToLocalChecked(), val )
#define SETN(o,key,val)  (o)->Set( context, Integer::New( isolate, key ), val )


void InitJSOX( Isolate *isolate, Local<Object> exports );
void InitJSON( Isolate *isolate, Local<Object> exports );
void InitSRG( Isolate *isolate, Local<Object> exports );
void InitWebSocket( Isolate *isolate, Local<Object> exports );
void InitUDPSocket( Isolate *isolate, Local<Object> exports );
void InitTask( Isolate *isolate, Local<Object> exports );
void KeyHidObjectInit( Isolate *isolate, Local<Object> exports );

void textObjectInit( Isolate *isolate, Local<Object> _exports );
	PTEXT isTextObject( Isolate *isolate, Local<Value> object );


#define ReadOnlyProperty (PropertyAttribute)((int)PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete)


struct PromiseWrapper {
	Persistent<Promise::Resolver> resolver;
	Persistent<Function> resolve;
	Persistent<Function> reject;
};
struct PromiseWrapper *makePromise( Local<Context> context, Isolate *isolate );

class VolumeObject : public node::ObjectWrap {
public:
	struct volume *vol;
	bool volNative;
	char *mountName;
	char *fileName;
	struct file_system_interface *fsInt;
	struct file_system_mounted_interface* fsMount;
	static v8::Persistent<v8::Function> constructor;

public:

	static void doInit( Local<Object> exports );
	static void Init( Local<Object> exports );
	static void Init( Local<Object> exports, Local<Value> val, void* p );
	VolumeObject( const char *mount, const char *filename, uintptr_t version, const char *key, const char *key2 );

	static void vfsObjectStorage( const v8::FunctionCallbackInfo<Value>& args );
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void getDirectory( const v8::FunctionCallbackInfo<Value>& args );
	static void fileRead( const v8::FunctionCallbackInfo<Value>& args );
	static void fileReadString( const v8::FunctionCallbackInfo<Value>& args );
	static void fileReadMemory( const v8::FunctionCallbackInfo<Value>& args );
	static void fileReadJSON( const v8::FunctionCallbackInfo<Value>& args );
	static void fileReadJSOX( const v8::FunctionCallbackInfo<Value>& args );
	static void fileWrite( const v8::FunctionCallbackInfo<Value>& args );
	static void fileExists( const v8::FunctionCallbackInfo<Value>& args );
	static void openVolDb( const v8::FunctionCallbackInfo<Value>& args );
	static void fileVolDelete( const v8::FunctionCallbackInfo<Value>& args );
	static void makeDirectory( const v8::FunctionCallbackInfo<Value>& args );
	static void mkdir( const v8::FunctionCallbackInfo<Value>& args );
	static void volRekey( const v8::FunctionCallbackInfo<Value>& args );
	static void renameFile( const v8::FunctionCallbackInfo<Value>& args );
	static void volDecrypt( const v8::FunctionCallbackInfo<Value>& args );

	~VolumeObject();
};



class ThreadObject : public node::ObjectWrap {
public:
	static v8::Persistent<v8::Function> constructor;
   	static Persistent<Function> idleProc;
public:

	static void Init( Local<Object> exports );
	ThreadObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void relinquish( const v8::FunctionCallbackInfo<Value>& args );
	static void wake( const v8::FunctionCallbackInfo<Value>& args );

	~ThreadObject();
};


class FileObject : public node::ObjectWrap {
	VolumeObject *vol;
	struct sack_vfs_file *file;
	FILE *cfile;
	//Local<Object> volume;
	PLIST buffers; // because these are passed as physical data buffers, don't release them willy-nilly
	char* buf;
	size_t size;
	Persistent<Object> volume;
public:
	static v8::Persistent<v8::Function> constructor;
	static Persistent<FunctionTemplate> tpl;
	static void Init(  );

	static void openFile( const v8::FunctionCallbackInfo<Value>& args );
	static void readFile( const v8::FunctionCallbackInfo<Value>& args );
	static void readLine( const v8::FunctionCallbackInfo<Value>& args );
	static void writeFile( const v8::FunctionCallbackInfo<Value>& args );
	static void writeLine( const v8::FunctionCallbackInfo<Value>& args );
	static void seekFile( const v8::FunctionCallbackInfo<Value>& args );
	static void tellFile( const v8::FunctionCallbackInfo<Value>& args );
	static void truncateFile( const v8::FunctionCallbackInfo<Value>& args );

	//static void readFile( const v8::FunctionCallbackInfo<Value>& args );

	static void Emitter( const v8::FunctionCallbackInfo<Value>& args );

	FileObject( VolumeObject* vol, const char *filename, Isolate*, Local<Object> o );
	~FileObject();
};



class ComObject : public node::ObjectWrap {
public:
	int handle;
	char *name;
	static Persistent<Function> constructor;

	Persistent<Function, CopyablePersistentTraits<Function>> *readCallback; //
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE readQueue;
	
public:

	static void Init( Local<Object> exports );
	ComObject( char *name );

private:
	Persistent<Object> jsObject;
	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void onRead( const v8::FunctionCallbackInfo<Value>& args );
	static void writeCom( const v8::FunctionCallbackInfo<Value>& args );
	static void closeCom( const v8::FunctionCallbackInfo<Value>& args );
	~ComObject();
};


class RegObject : public node::ObjectWrap {
public:
	static void Init( Local<Object> exports );

private:
	static void getRegItem( const v8::FunctionCallbackInfo<Value>& args );
	static void setRegItem( const v8::FunctionCallbackInfo<Value>& args );
};


class WebSockClientObject : public node::ObjectWrap {
public:
	static Persistent<Function> constructor;

	Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE readQueue;
	
public:

	static void Init( Local<Object> exports );
	WebSockClientObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void open( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void send( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );


	~WebSockClientObject();
};

class TLSObject : public node::ObjectWrap {

public:
	static v8::Persistent<v8::Function> constructor;

public:

	static void Init( Isolate *isolate, Local<Object> exports );
	TLSObject( );

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void seed( const v8::FunctionCallbackInfo<Value>& args );
	static void genKey( const v8::FunctionCallbackInfo<Value>& args );
	static void genCert( const v8::FunctionCallbackInfo<Value>& args );
	static void genReq( const v8::FunctionCallbackInfo<Value>& args );
	static void signReq( const v8::FunctionCallbackInfo<Value>& args );
	static void pubKey( const v8::FunctionCallbackInfo<Value>& args );
	static void validate( const v8::FunctionCallbackInfo<Value>& args );
	static void expiration( const v8::FunctionCallbackInfo<Value>& args );
	static void certToString( const v8::FunctionCallbackInfo<Value>& args );


	~TLSObject();
};

struct reviver_data {
	Persistent<Function> dateCons;

	Isolate *isolate;
	Local<Context> context;
	LOGICAL revive;
	//int index;
	Local<Value> value;
	Local<Object> _this;
	Local<Function> reviver;
	Local<Object> rootObject;
	class JSOXObject *parser;
};

Local<Value> convertMessageToJS( PDATALIST msg_data, struct reviver_data *reviver );
Local<Value> convertMessageToJS2( PDATALIST msg_data, struct reviver_data *reviver );

struct prototypeHolder {
	Persistent<Value> *cls;
	TEXTSTR className;
};

class JSOXObject : public node::ObjectWrap {
public:
	struct jsox_parse_state *state;
	static Persistent<Function> constructor;
	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //
	Persistent<Map> fromPrototypeMap;
	Persistent<Map> promiseFromPrototypeMap;
	PLIST prototypes; // revivde prototypes by class
public:

	static void Init( Local<Object> exports );
	JSOXObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void write( const v8::FunctionCallbackInfo<Value>& args );
	static void setFromPrototypeMap( const v8::FunctionCallbackInfo<Value>& args );
	static void setPromiseFromPrototypeMap( const v8::FunctionCallbackInfo<Value>& args );

	~JSOXObject();
};


struct arrayBufferHolder {
	void *buffer;
	Persistent<Object> o;
	Persistent<String> s;
	Persistent<ArrayBuffer> ab;
};
typedef struct arrayBufferHolder ARRAY_BUFFER_HOLDER, *PARRAY_BUFFER_HOLDER;
#define MAXARRAY_BUFFER_HOLDERSPERSET 512
DeclareSet( ARRAY_BUFFER_HOLDER );

void releaseBuffer( const WeakCallbackInfo<ARRAY_BUFFER_HOLDER> &info );
Local<String> localString( Isolate *isolate, const char *data, int len );
Local<String> localStringExternal( Isolate *isolate, const char *data, int len, const char *real_root );

void InitFS( const v8::FunctionCallbackInfo<Value>& args );
void ConfigScriptInit( Local<Object> exports );

void ObjectStorageInit( Isolate *isoalte, Local<Object> exports );


void SqlObjectInit( Local<Object> exports );
void createSqlObject( const char *name, Local<Object> into );
Local<Value> newSqlObject( Isolate *isolate, int argc, Local<Value> *argv );

class ObjectStorageObject*  openInVFS( Isolate *isolate, const char *mount, const char *name, const char *key1, const char *key2 );

#ifndef VFS_MAIN_SOURCE
extern
#endif
struct vfs_global_data {
	ARRAY_BUFFER_HOLDERSET *holders;
} vfs_global_data;

#define GetHolder() GetFromSet( ARRAY_BUFFER_HOLDER, &vfs_global_data.holders )
#define DropHolder(h) DeleteFromSet( ARRAY_BUFFER_HOLDER, vfs_global_data.holders, h )

