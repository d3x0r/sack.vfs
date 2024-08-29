#if defined( _MSC_VER )
#  pragma warning( disable: 4251 4275 26495)
// C26495 - uninitialized member; yes; It will be.
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
#include <idle.h>
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
#include <configscript.h>
#include <filemon.h>
#else
#  if defined( NODE_WANT_INTERNALS )
#    include "../../../deps/sack/sack.h"
#  else
#    include "sack/sack.h"
#  endif
#endif

#undef New

//#include <openssl/ssl.h>
#ifndef OPENSSL_API_COMPAT
#  define OPENSSL_API_COMPAT 10101
#endif

#if NODE_MAJOR_VERSION >= 17
#  include <openssl/configuration.h>
#endif
#include <openssl/safestack.h>  // STACK_OF
#include <openssl/tls1.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
//#include <openssl/>

#if OPENSSL_VERSION_MAJOR >= 3 
#include <openssl/core_names.h>
#endif

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

#if NODE_MAJOR_VERSION >= 12
#  define TOBOOL(i) BooleanValue( i )
#else
#  define TOBOOL(i) BooleanValue( i->GetCurrentContext() ).FromMaybe(false)
#endif

using namespace v8;

#include "task_module.h"
#include "ssh2_module.h"

//fileObject->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8Literal( isolate, "SeekSet" ), Integer::New( isolate, SEEK_SET ), ReadOnlyProperty );

#define SET_READONLY( object, name, data ) (object)->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8Literal(isolate, name, v8::NewStringType::kNormal ), data, ReadOnlyProperty )
#define SET_READONLY_METHOD( object, name, method ) (object)->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8Literal(isolate, name, v8::NewStringType::kNormal ), v8::Function::New(isolate->GetCurrentContext(), method ).ToLocalChecked(), ReadOnlyProperty )

#define GET(o,key)  (o)->Get( context, String::NewFromUtf8Literal( isolate, (key), v8::NewStringType::kNormal ) ).ToLocalChecked()
#define GETV(o,key)  (o)->Get( context, key ).ToLocalChecked()
#define GETN(o,key)  (o)->Get( context, Integer::New( isolate, (key) ) ).ToLocalChecked()
#define SETV(o,key,val)  (void)(o)->Set( context, key, val )
#define SET(o,key,val)  (void)(o)->Set( context, String::NewFromUtf8Literal( isolate, (key) ), val )
#define SETVAR(o,key,val)  (void)(o)->Set( context, String::NewFromUtf8( isolate, (key), v8::NewStringType::kNormal ).ToLocalChecked(), val )
#define SETT(o,key,val)  (void)(o)->Set( context, String::NewFromUtf8( isolate, GetText(key), v8::NewStringType::kNormal, (int)GetTextSize( key ) ).ToLocalChecked(), val )
#define SETN(o,key,val)  (void)(o)->Set( context, Integer::New( isolate, key ), val )


// --------- String Utilities for option objects ------------
#define DEF_STRING(name) Eternal<String> *name##String
#define MK_STRING(name)  check->name##String = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, #name ) );
#define GET_STRING(name)  	String::Utf8Value* name = NULL; \
		if( opts->Has( context, optName = strings->name##String->Get( isolate ) ).ToChecked() ) { \
				if( GETV( opts, optName )->IsString() ) { \
					name = new String::Utf8Value( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ); \
				} \
			}

#define GET_ARRAY_BUFFER(name)  	Local<ArrayBuffer> name##_ab; \
		if( opts->Has( context, optName = strings->name##String->Get( isolate ) ).ToChecked() ) { \
				if( GETV( opts, optName )->IsArrayBuffer() ) { \
					name##_ab = Local<ArrayBuffer>::Cast( GETV( opts, optName ) ); \
				} \
			}

#define GET_TYPED_ARRAY(name)  	Local<TypedArray> name##_ta; \
		if( opts->Has( context, optName = strings->name##String->Get( isolate ) ).ToChecked() ) { \
				if( GETV( opts, optName )->IsArrayBuffer() ) { \
					name##_ta = Local<TypedArray>::Cast( GETV( opts, optName ) ); \
				} \
			}

#define GET_NUMBER(name)  int name = 0;  \
		if( opts->Has( context, optName = strings->name##String->Get( isolate ) ).ToChecked() ) { \
				if( GETV( opts, optName )->IsString() ) { \
					name = (int)GETV( opts, optName )->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 ); \
				} \
			}
#define GET_BOOL(name)  bool name = false; \
		if( opts->Has( context, optName = strings->name##String->Get( isolate ) ).ToChecked() ) { \
				if( GETV( opts, optName )->IsBoolean() ) { \
					name = GETV( opts, optName )->TOBOOL( isolate ); \
				} \
			}

//------------------ end of string utilities ----------------

#if ( NODE_MAJOR_VERSION <= 13 )
#define NewFromUtf8Literal(a,b,...)  NewFromUtf8(a,b, v8::NewStringType::kNormal ).ToLocalChecked()
#endif



void InitJSOX( Isolate *isolate, Local<Object> exports );
void InitJSON( Isolate *isolate, Local<Object> exports );
void InitSRG( Isolate *isolate, Local<Object> exports );
void InitWebSocket( Isolate *isolate, Local<Object> exports );
void InitUDPSocket( Isolate *isolate, Local<Object> exports );
void InitTask( Isolate *isolate, Local<Object> exports );
void KeyHidObjectInit( Isolate *isolate, Local<Object> exports );
void SoundInit( Isolate* isolate, Local<Object> exports );
void fileMonitorInit( Isolate* isolate, Local<Object> exports );

void textObjectInit( Isolate *isolate, Local<Object> _exports );
PTEXT isTextObject( Isolate *isolate, Local<Value> object );
void SystemInit( Isolate* isolate, Local<Object> exports );
void InitSystray( Isolate * isolate, Local<Object> _exports );

Local<Object> makeSocket( Isolate* isolate, PCLIENT pc, struct html5_web_socket* pipe, class wssObject* wss, class wscObject* wsc, class wssiObject* wssi );

#define ReadOnlyProperty (PropertyAttribute)((int)PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete)

class constructorSet {
	public:
	Isolate *isolate;
	PTHREAD thread;
	Persistent<Function> dateCons; // Date constructor
	Persistent<Function> dateNsCons; // Date constructor
	Persistent<Function> ThreadObject_idleProc;

	// constructor
	Persistent<Function> volConstructor;
	Persistent<Function> fileConstructor;
	Persistent<FunctionTemplate> fileTpl;

	Persistent<Function> threadConstructor;
	Persistent<Function> comConstructor;
	Persistent<Function> wscConstructor;
	Persistent<FunctionTemplate> wscTpl;

	Persistent<Function> wssConstructor;
	Persistent<FunctionTemplate> wssTpl;
	Persistent<Function> wssiConstructor;
	Persistent<FunctionTemplate> wssiTpl;
	Persistent<Function> httpReqConstructor;
	Persistent<Function> httpConstructor;

	Persistent<Function> tlsConstructor;
	Persistent<Function> jsoxConstructor;
	Persistent<Function> jsoxRefConstructor;

	Persistent<Function> parseConstructor;
	Persistent<Function> parseConstructor6;
	Persistent<Function> parseConstructor6v;

	v8::Persistent<v8::Function> sqlConstructor;
	v8::Persistent<v8::Function> sqlStmtConstructor;
	v8::Persistent<v8::Function> otoConstructor;

	//Persistent<Function> jsonConstructor;
	Persistent<FunctionTemplate> pTextTemplate;
	Persistent<Function> textConstructor;

	Persistent<Function> addrConstructor;
	Persistent<FunctionTemplate> addrTpl;
	Persistent<Function> udpConstructor;
	Persistent<Function> tcpConstructor;

	Persistent<Map> fromPrototypeMap;

	uv_async_t clientSocketPoster;
	Persistent<Function> promiseThen;
	Persistent<Function> promiseCatch;

	v8::Persistent<v8::Function> SRGObject_constructor;
	v8::Persistent<v8::Function> TaskObject_constructor;
	v8::Persistent<v8::Function> ObjectStorageObject_constructor;
	v8::Persistent<v8::Function> TimelineCursorObject_constructor;
	v8::Persistent<v8::Function> monitorConstructor;  // File Monitor
	v8::Persistent<v8::Function> KeyHidObject_constructor;
	v8::Persistent<v8::Function> MouseHidObject_constructor;
	v8::Persistent<v8::Function> ConfigObject_constructor;
	v8::Persistent<v8::Function> SSH_Object_constructor;
	v8::Persistent<v8::Function> SSH_Channel_constructor;
	v8::Persistent<v8::Function> SSH_RemoteListen_constructor;
	//v8::Persistent<v8::Function> SSH_LocalListen_constructor;

	//Persistent<Function> onCientPost;
	uv_loop_t* loop;
	Persistent<Function> exitCallback;
	uv_async_t exitAsync; // different modules might have different signal registrations
#ifdef INCLUDE_GUI
	uv_async_t psiLocal_async;
	int eventLoopEnables = 0;
	LOGICAL eventLoopRegistered = FALSE;

	Persistent<Function> ImageObject_constructor;
	Persistent<FunctionTemplate> ImageObject_tpl;
	LOGICAL fontAsyncActive;
	uv_async_t fontAsync; // keep this instance around for as long as we might need to do the periodic callback
	Persistent<Function> FontObject_constructor;
	LOGICAL colorAsyncActive;
	uv_async_t colorAsync; // keep this instance around for as long as we might need to do the periodic callback
	Persistent<Function> ColorObject_constructor;
	Persistent<FunctionTemplate> ColorObject_tpl;
	Persistent<Function> fontResult;
	Persistent<Function> imageResult;
	Persistent<Object>   priorThis;

	Persistent<Object> canvasObject;

	v8::Persistent<v8::Function> InterShellObject_buttonConstructor;
	v8::Persistent<v8::Function> InterShellObject_buttonInstanceConstructor;
	v8::Persistent<v8::Function> InterShellObject_controlConstructor;
	v8::Persistent<v8::Function> InterShellObject_controlInstanceConstructor;
	v8::Persistent<v8::Function> InterShellObject_customControlConstructor;
	v8::Persistent<v8::Function> InterShellObject_customControlInstanceConstructor;
	v8::Persistent<v8::Function> InterShellObject_intershellConstructor;
	v8::Persistent<v8::Function> InterShellObject_configConstructor;

	v8::Persistent<v8::Function> ControlObject_constructor;   // Frame
	v8::Persistent<v8::Function> ControlObject_constructor2;  // Control
	v8::Persistent<v8::Function> ControlObject_registrationConstructor;  // Registration
	v8::Persistent<v8::FunctionTemplate> ControlObject_controlTemplate;
	v8::Persistent<v8::FunctionTemplate> ControlObject_frameTemplate;
	v8::Persistent<v8::FunctionTemplate> ControlObject_listItemTemplate;

	Persistent<Function> PopupObject_constructor;
	Persistent<Function> ListboxItemObject_constructor;
	Persistent<Function> MenuItemObject_constructor;

	v8::Persistent<v8::Function> VoidObject_constructor;   // generic void constructor
	v8::Persistent<v8::Function> VoidObject_constructor2;   // object color interface accessors

	v8::Persistent<v8::Function> RenderObject_constructor;
	v8::Persistent<v8::Function> RenderObject_constructor2;

	v8::Persistent<v8::Function> VulkanObject_constructor;
#endif
};
class constructorSet * getConstructors( Isolate *isolate );
class constructorSet* getConstructorsByThread( void );



struct PromiseWrapper {
	Persistent<Promise::Resolver> resolver;
	Persistent<Function> resolve;
	Persistent<Function> reject;
};
struct PromiseWrapper *makePromise( Local<Context> context, Isolate *isolate );

class VolumeObject : public node::ObjectWrap {
public:
	struct sack_vfs_volume *vol;
	bool volNative;
	char *mountName;
	char *fileName;
	int priority;
	struct file_system_interface *fsInt;
	struct file_system_mounted_interface* fsMount;
	static PLIST volumes;
	LOGICAL cleanupHappened;
	static PLIST transportDestinations;
	LOGICAL thrown;

public:

	static void doInit( Local<Context> context, Local<Object> exports );
	static void Init( Local<Context> context, Local<Object> exports );
	static void Init( Local<Object> exports, Local<Value> val, void* p );
	VolumeObject( const char *mount, const char *filename, uintptr_t version, const char *key, const char *key2, int priority = 2000 );
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
	static void changeDirectory( const v8::FunctionCallbackInfo<Value>& args );
	static void chDir( const v8::FunctionCallbackInfo<Value>& args );
	static void isDirectory( const v8::FunctionCallbackInfo<Value>& args );
	static void mkdir( const v8::FunctionCallbackInfo<Value>& args );
	static void volRekey( const v8::FunctionCallbackInfo<Value>& args );
	static void renameFile( const v8::FunctionCallbackInfo<Value>& args );
	static void volDecrypt( const v8::FunctionCallbackInfo<Value>& args );
	static void flush( const v8::FunctionCallbackInfo<Value>& args );

	~VolumeObject();
};



class ThreadObject : public node::ObjectWrap {
public:
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
	//Local<Object> sack_vfs_volume;
	PLIST buffers; // because these are passed as physical data buffers, don't release them willy-nilly
	char* buf;
	size_t size;
	Persistent<Object> volume;
public:
	static void Init(  );

	static void openFile( const v8::FunctionCallbackInfo<Value>& args );
	static void readFile( const v8::FunctionCallbackInfo<Value>& args );
	static void readLine( const v8::FunctionCallbackInfo<Value>& args );
	static void writeFile( const v8::FunctionCallbackInfo<Value>& args );
	static void writeLine( const v8::FunctionCallbackInfo<Value>& args );
	static void seekFile( const v8::FunctionCallbackInfo<Value>& args );
	static void tellFile( const v8::FunctionCallbackInfo<Value>& args );
	static void truncateFile( const v8::FunctionCallbackInfo<Value>& args );
	static void closeFile( const v8::FunctionCallbackInfo<Value>& args );

	//static void readFile( const v8::FunctionCallbackInfo<Value>& args );

	static void Emitter( const v8::FunctionCallbackInfo<Value>& args );

	FileObject( VolumeObject* vol, const char *filename, Isolate*, Local<Object> o );
	~FileObject();
};



extern void ComObjectInit( Local<Object> exports );

class RegObject : public node::ObjectWrap {
public:
	static void Init( Local<Object> exports );

private:
	static void getRegItem( const v8::FunctionCallbackInfo<Value>& args );
	static void setRegItem( const v8::FunctionCallbackInfo<Value>& args );
};


class WebSockClientObject : public node::ObjectWrap {
public:
	//static Persistent<Function> constructor;

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
	//static v8::Persistent<v8::Function> constructor;

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

struct reviveMemberReplacement {
	Local<Value> object;
	Local<Value> fieldName;
};

struct reviveStackMember {
	LOGICAL isArray;
	int index;
	Local<Value> fieldName;
	Local<Value> object;
	char* name;
	size_t nameLen;
	PDATALIST pdlSubsts;
	reviveStackMember() {
		pdlSubsts = CreateDataList( sizeof( struct reviveMemberReplacement ) );

	}
	~reviveStackMember() {
		{
			INDEX idx;
			struct reviveMemberReplacement* rep;
			DATA_FORALL( pdlSubsts, idx, struct reviveMemberReplacement*, rep ) {
				rep->fieldName.Clear();
				rep->object.Clear();
			}
		}
		DeleteDataList( &pdlSubsts );
	}
};

struct reviver_data {
	//Persistent<Function> dateCons;
	Local<Function> fieldCb;
	Isolate *isolate;
	Local <Object> refObject; // JSOX.Ref
	Local<Value> fieldName;
	Local<Context> context;
	LOGICAL revive;
	//int index;
	Local<Value> value;
	Local<Object> _this;
	Local<Function> reviver;
	Local<Object> rootObject;
	class JSOXObject *parser;
	LOGICAL failed;
	PLINKSTACK reviveStack;

	~reviver_data() {
		DeleteLinkStack( &this->reviveStack );
	}
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
	//static Persistent<Function> constructor;
	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> reviver; // on begin() save reviver function here
	Persistent<Map> fromPrototypeMap;
	Persistent<Map> promiseFromPrototypeMap;
	PLIST prototypes; // revivde prototypes by class
	struct reviver_data* currentReviver;
public:

	static void Init( Local<Object> exports );
	JSOXObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void write( const v8::FunctionCallbackInfo<Value>& args );
	static void parse( const v8::FunctionCallbackInfo<Value>& args );
	static void reset( const v8::FunctionCallbackInfo<Value>& args );
	static void getCurrentRef( const v8::FunctionCallbackInfo<Value>& args );

	static void setFromPrototypeMap( const v8::FunctionCallbackInfo<Value>& args );
	static void setPromiseFromPrototypeMap( const v8::FunctionCallbackInfo<Value>& args );

	~JSOXObject();
};


struct arrayBufferHolder {
	const void *buffer;
	Persistent<Object> o;
	Persistent<String> s;
	Persistent<ArrayBuffer> ab;
};
typedef struct arrayBufferHolder ARRAY_BUFFER_HOLDER, *PARRAY_BUFFER_HOLDER;
#define MAXARRAY_BUFFER_HOLDERSPERSET 512
DeclareSet( ARRAY_BUFFER_HOLDER );

void releaseBufferBackingStore( void* data, size_t length, void* deleter_data );
void dontReleaseBufferBackingStore(void* data, size_t length, void* deleter_data);
void releaseBuffer( const WeakCallbackInfo<ARRAY_BUFFER_HOLDER> &info );
Local<String> localString( Isolate *isolate, const char *data, int len = -1 );
Local<String> localStringExternal( Isolate *isolate, const char *data, int len = -1, const char *real_root = NULL );

void InitFS( const v8::FunctionCallbackInfo<Value>& args );
void ConfigScriptInit( Isolate *isolate, Local<Object> exports );

void ObjectStorageInit( Isolate *isolate, Local<Object> exports );


void SqlObjectInit( Local<Object> exports );
void createSqlObject( const char* name, Isolate* isolate, Local<Object> into );
Local<Value> newSqlObject( Isolate *isolate, int argc, Local<Value> *argv );

class ObjectStorageObject*  openInVFS( Isolate *isolate, const char *mount, const char *name, const char *key1, const char *key2 );
Local<Object> WrapObjectStorage( Isolate* isolate, class ObjectStorageObject* oso );


#ifndef VFS_MAIN_SOURCE
extern
#endif
struct vfs_global_data {
	ARRAY_BUFFER_HOLDERSET *holders;
	int shutdown;
} vfs_global_data;

#define GetHolder() GetFromSet( ARRAY_BUFFER_HOLDER, &vfs_global_data.holders )
#define DropHolder(h) DeleteFromSet( ARRAY_BUFFER_HOLDER, vfs_global_data.holders, h )

#ifdef _WIN32
struct command_line_result {
	DWORD dwProcessId;
	size_t length;
	char* data;
	char* processName;
};
#else
struct command_line_result {
	pid_t dwProcessId;
	size_t length;
	char** cmd;
};

#endif
// returns a PLIST of struct command_line_results
PLIST GetProcessCommandLines( const char* process );
int GetProcessParent( int pid );

void ReleaseCommandLineResults( PLIST* ppResults );