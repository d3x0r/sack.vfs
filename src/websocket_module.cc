
#include "global.h"

#include "ssh2_module.h"
#include "websocket_module.h"

//#define DEBUG_AGGREGATE_WRITES
//#define DEBUG_EVENTS
//#define EVENT_DEBUG_STDOUT
#define USE_NETWORK_AGGREGATE 0
#define AGGREGATE_BEFORE_NETWORK 0

#ifdef DEBUG_EVENTS
#  ifdef EVENT_DEBUG_STDOUT
#    undef lprintf
#    define lprintf(f,...)  printf( "%d~" f "\n", (int)(GetThisThreadID() & 0x7FFFFFF),##__VA_ARGS__)
#  endif
#endif

/*

MessageEvent.data Read only
The data sent by the message emitter.
MessageEvent.origin Read only
A USVString representing the origin of the message emitter.
MessageEvent.lastEventId Read only
A DOMString representing a unique ID for the event.
MessageEvent.source Read only
A MessageEventSource (which can be a WindowProxy, MessagePort, or ServiceWorker object) representing the message emitter.
MessageEvent.ports Read only

*/


/*

CONNECTING = 0
OPEN (1)
CLOSING (2)
CLOSED (3)
---
LISTENING (4)


Constructor(DOMString type, optional CloseEventInit eventInitDict)]
interface CloseEvent : Event {
readonly attribute boolean wasClean;
readonly attribute unsigned short code;
readonly attribute DOMString reason;
};

dictionary CloseEventInit : EventInit {
boolean wasClean;
unsigned short code;
DOMString reason;
};

readonly attribute unsigned short readyState;
readonly attribute unsigned long bufferedAmount;

readonly attribute DOMString extensions;
The extensions attribute returns the extensions selected by the server, if any. (Currently this will only ever be the empty string.)

readonly attribute DOMString protocol;
The protocol attribute returns the subprotocol selected by the server, if any. It can be used in conjunction with the array form
of the constructor's second argument to perform subprotocol negotiation.

attribute DOMString binaryType;
void send(DOMString data);
void send(Blob data);
void send(ArrayBuffer data);
void send(ArrayBufferView data);

*/


enum wsEvents {
	WS_EVENT_OPEN,   // onaccept/onopen callback (wss(passes wssi),wsc)
	WS_EVENT_ACCEPT, // onaccept callback (wss)
	WS_EVENT_CLOSE,  // onClose callback (wss(internal only), wssi, wsc)
	WS_EVENT_READ,   // onMessage callback (wssi,wsc)
	WS_EVENT_ERROR,  // error event (unused)  (wssi,wss,wsc)
	WS_EVENT_REQUEST,// websocket non-upgrade request
	WS_EVENT_RESPONSE,// websocket non-upgrade request
	WS_EVENT_ERROR_CLOSE, // illegal server connection
	WS_EVENT_LOW_ERROR,
	WS_EVENT_RELEASE_BUFFER,
	WS_EVENT_ACCEPT_SSH, // a new websocket over SSH connection to create (uses wssi)
};

struct optionStrings {
	Isolate *isolate;

	Eternal<String> *portString;
	Eternal<String> *urlString;
	Eternal<String> *v4String;
	Eternal<String> *v6String;
	Eternal<String> *localFamilyString;
	Eternal<String> *remoteFamilyString;
	Eternal<String> *localPortString;
	Eternal<String> *remotePortString;
	Eternal<String> *addressString;
	Eternal<String> *localAddrString;
	Eternal<String> *remoteAddrString;
	Eternal<String>* localNameString;
	Eternal<String>* remoteNameString;

	Eternal<String> *localMacString;
	Eternal<String> *remoteMacString;
	Eternal<String> *headerString;
	Eternal<String> *certString;
	Eternal<String> *CGIString;
	Eternal<String> *contentString;
	Eternal<String> *keyString;
	Eternal<String> *pemString;
	Eternal<String> *passString;
	Eternal<String> *deflateString;
	Eternal<String> *deflateAllowString;
	Eternal<String>* resolveString;
	Eternal<String>* getMacString;
	Eternal<String> *caString;
	Eternal<String> *vUString;
	Eternal<String> *connectionString;
	Eternal<String> *maskingString;
	Eternal<String> *bytesReadString;
	Eternal<String> *bytesWrittenString;
	Eternal<String> *requestString;
	Eternal<String> *readyStateString;
	Eternal<String> *bufferedAmountString;
	Eternal<String> *hostnameString;
	Eternal<String>* hostsString;
	Eternal<String>* hostString;
	Eternal<String> *rejectUnauthorizedString;
	Eternal<String> *pathString;
	Eternal<String> *methodString;
	Eternal<String> *redirectString;
	Eternal<String> *keepAliveString;
	Eternal<String> *versionString;
	Eternal<String> *onReplyString;
	Eternal<String> *agentString;
	Eternal<String> *timeoutString;
	Eternal<String> *retriesString;
	Eternal<String>* pipeString;
};

static PLIST strings;

struct wssHostOption {
	char* host;
	int hostlen;
	char* cert_chain;
	int cert_chain_len;
	char* key;
	int key_len;
	char* pass;
	int pass_len;
};

struct wssOptions {
	char *url;
	char *address;
	int port;
	bool ssl;
	char *cert_chain;
	int cert_chain_len;
	char *key;
	int key_len;
	char *pass;
	int pass_len;
	bool deflate;
	bool deflate_allow;
	bool apply_masking;
	PLIST hostList;
	SSH2_Channel* channel;
	bool resolveName;
	bool getMAC;
	bool pipe;
};

struct wscOptions {
	char *url;
	char *protocol;
	bool ssl;
	char *key;
	int key_len;
	char *pass;
	int pass_len;
	char *root_cert;
	bool deflate;
	bool deflate_allow;
	bool apply_masking;
	bool keep_alive;
	bool resolveName;
	bool getMAC;
};

struct pendingSend {
	LOGICAL binary;
	POINTER buffer;
	size_t buflen;
};

struct wssEvent {
	enum wsEvents eventType;
	class wssObject *_this;
	class SSH2_Channel *channel;
	union {
		struct pendingWrite* write;
		struct {
			int accepted;
			int rejected;
			const char *protocol;
			const char *resource;
		} request;
		struct {
			int error;
			const char *buffer;
			size_t buflen;
			int fallback_ssl;
		} error;
	}data;
	PLIST send;
	PCLIENT pc;
	PTHREAD waiter;
	LOGICAL done;
	class wssiObject *result;
	int uses;
};
typedef struct wssEvent WSS_EVENT;
#define MAXWSS_EVENTSPERSET 64
DeclareSet( WSS_EVENT );

struct wssiEvent {
	enum wsEvents eventType;
	class wssiObject *_this;
	//PLIST send;
	//LOGICAL send_close; // need reason.
	int code;

	CPOINTER buf;
	size_t buflen;
	LOGICAL binary;
	//PTHREAD waiter;
	//LOGICAL done;
};
typedef struct wssiEvent WSSI_EVENT;
#define MAXWSSI_EVENTSPERSET 128
DeclareSet( WSSI_EVENT );

struct wscEvent {
	enum wsEvents eventType;
	class wscObject *_this;
	CPOINTER buf;
	int code;
	size_t buflen;
	LOGICAL binary;
};
typedef struct wscEvent WSC_EVENT;
#define MAXWSC_EVENTSPERSET 128
DeclareSet( WSC_EVENT );

struct httpRequestEvent {
	enum wsEvents eventType;
	class httpRequestObject *_this;
	HTTPState state;
};
typedef struct httpRequestEvent HTTP_REQUEST_EVENT;
#define MAXHTTP_REQUEST_EVENTSPERSET 128
DeclareSet( HTTP_REQUEST_EVENT );


struct socketTransport {
	class wssiObject *wssi;
	WSS_EVENT* acceptEventMessage;
	const char *protocolResponse;
};

struct socketUnloadStation {
	Persistent<Object> this_;
	String::Utf8Value* s;  // destination address
	uv_async_t clientSocketPoster;
	uv_loop_t  *targetThread;
	Persistent<Function> cb; // callback to invoke 
	PLIST transport;
};

struct pendingWrite {
	Persistent<ArrayBuffer> buffer;
	CPOINTER data;
	wssObject* wss;
};

static struct local {
	int data;
	//uv_loop_t* loop;
	int waiting;
	PTHREAD jsThread;
	CRITICALSECTION csWssEvents;
	PWSS_EVENTSET wssEvents;
	CRITICALSECTION csWssiEvents;
	PWSSI_EVENTSET wssiEvents;
	CRITICALSECTION csWscEvents;
	PWSC_EVENTSET wscEvents;

	CRITICALSECTION csHttpRequestEvents;
	PHTTP_REQUEST_EVENTSET httpRequestEvents;

	PLIST transportDestinations;
	PLIST pendingWrites;
	CRITICALSECTION csConnect;
	CRITICALSECTION csOpen;

} l;

static void promoteSSH_to_Websocket( const v8::FunctionCallbackInfo<Value>& args );

static struct optionStrings *getStrings( Isolate *isolate ) {
	INDEX idx;
	struct optionStrings * check;
	LIST_FORALL( strings, idx, struct optionStrings *, check ) {
		if( check->isolate == isolate )
			break;
	}
	if( !check ) {
		check = NewArray( struct optionStrings, 1 );
		AddLink( &strings, check );
		check->isolate = isolate;
		check->readyStateString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "readyState" ) );
		check->portString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "port" ) );
		check->urlString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "url" ) );
		check->localPortString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "localPort" ) );
		check->remotePortString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "remotePort" ) );
		check->addressString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "address" ) );
		check->localAddrString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "localAddress" ) );
		check->remoteAddrString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "remoteAddress" ) );
		check->localNameString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "localName" ) );
		check->remoteNameString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "remoteName" ) );
		check->localMacString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "localMAC" ) );
		check->remoteMacString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "remoteMAC" ) );
		check->localFamilyString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "localFamily" ) );
		check->remoteFamilyString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "remoteFamily" ) );
		check->headerString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "headers" ) );
		check->CGIString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "CGI" ) );
		check->contentString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "content" ) );
		check->certString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "cert" ) );
		check->keyString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "key" ) );
		check->pemString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "pem" ) );
		check->passString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "passphrase" ) );
		check->resolveString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "resolveNames" ) );
		check->getMacString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "getMAC" ) );
		check->deflateString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "perMessageDeflate" ) );
		check->pipeString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "pipe" ) );
		
		check->deflateAllowString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "perMessageDeflateAllow" ) );
		check->caString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "ca" ) );
		check->v4String = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "IPv4" ) );
		check->v6String = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "IPv6" ) );
		check->vUString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "unknown" ) );
		check->connectionString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "connection" ) );
		check->maskingString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "masking" ) );
		check->bytesReadString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "bytesRead" ) );
		check->bytesWrittenString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "bytesWritten" ) );
		check->requestString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "request" ) );
		check->bufferedAmountString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "bufferedAmount" ) );

		check->hostnameString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "hostname" ) );
		check->hostString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "host" ) );
		check->hostsString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "hosts" ) );
		check->retriesString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "retries" ) );
		check->timeoutString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "timeout" ) );
		check->pathString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "path" ) );
		check->methodString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "method" ) );
		check->redirectString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "redirect" ) );
		check->rejectUnauthorizedString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "rejectUnauthorized" ) );
		check->keepAliveString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "keepAlive" ) );
		check->versionString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "version" ) );
		check->onReplyString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "onReply" ) );
		check->agentString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "agent" ) );
		
	}
	return check;
}

class httpRequestObject : public node::ObjectWrap {
public:
	PCLIENT pc;
	//static Persistent<Function> constructor;
	Persistent<Object> _this;
	struct HTTPRequestOptions* opts;
	//PVARTEXT pvtResult;
	bool ssl;
	int port;
	char *hostname;
	const char *method = "GET";
	const char* content = NULL;
	size_t contentLen = 0;
	const char* httpVersion = NULL;
	int timeout = 3000;
	int retries = 3;
	PLIST headers = NULL;
	char *ca = NULL;
	char *path = NULL;
	char *agent = NULL;

	bool rejectUnauthorized;

	bool firstDispatchDone;
	bool dataDispatch;
	bool endDispatch;

	bool finished;
	PTHREAD waiter;

	PTEXT result;
	HTTPState state;
	// if set, will be called when content buffer has been sent.
	//void ( *writeComplete )(void );
public:

	httpRequestObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void wait( const v8::FunctionCallbackInfo<Value>& args );
	static void get( const v8::FunctionCallbackInfo<Value>& args );
	static void gets( const v8::FunctionCallbackInfo<Value>& args );
	static void getRequest( const v8::FunctionCallbackInfo<Value>& args, bool secure );


	Persistent<Object> resultObject;
	Persistent<Function, CopyablePersistentTraits<Function>> resultCallback;
	Persistent<Function, CopyablePersistentTraits<Function>> cbError;

	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE eventQueue;

	~httpRequestObject();
};


// web sock server Object
class httpObject : public node::ObjectWrap {
public:
	PCLIENT pc;
	//static Persistent<Function> constructor;
	PVARTEXT pvtResult;
	bool ssl;
	wssObject* wss;
	Isolate *isolate;
	bool headWritten = false;
public:

	httpObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void writeHead( const v8::FunctionCallbackInfo<Value>& args );
	static void end( const v8::FunctionCallbackInfo<Value>& args );

	~httpObject();
};

// web sock client Object
class wscObject : public node::ObjectWrap {

public:
	PCLIENT pc;
	bool resolveAddr = false;
	bool resolveMac = false;
	uv_async_t async = {}; // keep this instance around for as long as we might need to do the periodic callback
	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
	//wscEvent *eventMessage;
	LOGICAL closed;
	enum wsReadyStates readyState;
	Isolate *isolate;
public:
	//static Persistent<Function> constructor;
	//static Persistent<FunctionTemplate> tpl;
	//Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	//Persistent<Function, CopyablePersistentTraits<Function>> messageCallback; //
	//Persistent<Function, CopyablePersistentTraits<Function>> errorCallback; //
	//Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; //
	PLIST openCallbacks = NULL;
	PLIST messageCallbacks = NULL;
	PLIST closeCallbacks = NULL;
	PLIST errorCallbacks = NULL;

public:

	wscObject( wscOptions *opts );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void onMessage( const v8::FunctionCallbackInfo<Value>& args );
	static void onError( const v8::FunctionCallbackInfo<Value>& args );
	static void onClose( const v8::FunctionCallbackInfo<Value>& args );
	static void onOpen( const v8::FunctionCallbackInfo<Value>& args );
	static void getOnMessage( const v8::FunctionCallbackInfo<Value>& args );
	static void getOnError( const v8::FunctionCallbackInfo<Value>& args );
	static void getOnClose( const v8::FunctionCallbackInfo<Value>& args );
	static void getOnOpen( const v8::FunctionCallbackInfo<Value>& args );
	static void write( const v8::FunctionCallbackInfo<Value>& args );
	static void getReadyState( const FunctionCallbackInfo<Value>& args );
	static void ping( const v8::FunctionCallbackInfo<Value>& args );
	static void nodelay( const v8::FunctionCallbackInfo<Value>& args );
	static void aggregate( const v8::FunctionCallbackInfo<Value>& args );

	~wscObject();
};

class callbackFunction {
public:
	Persistent<Function, CopyablePersistentTraits<Function>> callback; //
};

// web sock server instance Object  (a connection from a remote)
class wssiObject : public node::ObjectWrap {
public:
	uv_async_t async = {}; // keep this instance around for as long as we might need to do the periodic callback
	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
	PCLIENT pc;
	struct html5_web_socket* wsPipe;

	bool resolveAddr = false;
	bool resolveMac = false;
	LOGICAL closed;
	const char *protocolResponse;
	enum wsReadyStates readyState;
	Isolate *isolate;
	class wssiObjectReference *wssiRef;
	class wssObject *server; // send open event to this object.
	WSS_EVENT* acceptEventMessage;
	Persistent<Promise> acceptPromise;
	LOGICAL blockReturn;
	LOGICAL thrown; // prevent event to server connect callback, event becomes post to thread
public:
	//static Persistent<Function> constructor;
	//static Persistent<FunctionTemplate> tpl;
	////Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	//Persistent<Function, CopyablePersistentTraits<Function>> errorCallback; //
	//Persistent<Function, CopyablePersistentTraits<Function>> messageCallback; //
	//Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; //

	PLIST messageCallbacks = NULL;
	PLIST closeCallbacks = NULL;
	PLIST errorCallbacks = NULL;

public:

	wssiObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void write( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void onmessage( const v8::FunctionCallbackInfo<Value>& args );
	static void onclose( const v8::FunctionCallbackInfo<Value>& args );
	static void getReadyState( const FunctionCallbackInfo<Value>& args );
	static void nodelay( const FunctionCallbackInfo<Value>& args );
	static void ping( const v8::FunctionCallbackInfo<Value>& args );
	static void aggregate( const v8::FunctionCallbackInfo<Value>& args );
	
	~wssiObject();
};

class wssiObjectReference {
public:
	class wssiObject *wssi;
};

//------------------------------------ Event Helpers ---------------------------------------

static WSS_EVENT *GetWssEvent() {
	EnterCriticalSec( &l.csWssEvents );
	WSS_EVENT *evt = GetFromSet( WSS_EVENT, &l.wssEvents );
	memset( evt, 0, sizeof( WSS_EVENT ) );
	evt->uses = 1;
	LeaveCriticalSec( &l.csWssEvents );
	return evt;
}

static WSSI_EVENT *GetWssiEvent() {
	EnterCriticalSec( &l.csWssiEvents );
	WSSI_EVENT *evt = GetFromSet( WSSI_EVENT, &l.wssiEvents );
	memset( evt, 0, sizeof( WSSI_EVENT ) );
	LeaveCriticalSec( &l.csWssiEvents );
	return evt;
}

static WSC_EVENT *GetWscEvent() {
	EnterCriticalSec( &l.csWscEvents );
	WSC_EVENT *evt = GetFromSet( WSC_EVENT, &l.wscEvents );
	memset( evt, 0, sizeof( WSC_EVENT ) );
	LeaveCriticalSec( &l.csWscEvents );
	return evt;
}

static HTTP_REQUEST_EVENT *GetHttpRequestEvent() {
	EnterCriticalSec( &l.csHttpRequestEvents );
	HTTP_REQUEST_EVENT *evt = GetFromSet( HTTP_REQUEST_EVENT, &l.httpRequestEvents );
	memset( evt, 0, sizeof( HTTP_REQUEST_EVENT ) );
	LeaveCriticalSec( &l.csHttpRequestEvents );
	return evt;
}

static void HoldWssEvent( WSS_EVENT *evt ) {
	evt->uses++;
}

static void DropWssEvent( WSS_EVENT *evt ) {
	EnterCriticalSec( &l.csWssEvents );
	if( !(--evt->uses ) ) {
		DeleteFromSet( WSS_EVENT, l.wssEvents, evt );
	}
	//lprintf( "Events outstanding:%d %d", evt->uses, CountUsedInSet( WSS_EVENT, l.wssEvents ) );
	LeaveCriticalSec( &l.csWssEvents );
}

static void DropWssiEvent( WSSI_EVENT *evt ) {
	EnterCriticalSec( &l.csWssiEvents );
	DeleteFromSet( WSSI_EVENT, l.wssiEvents, evt );
	LeaveCriticalSec( &l.csWssiEvents );
}

static void DropWscEvent( WSC_EVENT *evt ) {
	EnterCriticalSec( &l.csWscEvents );
	DeleteFromSet( WSC_EVENT, l.wscEvents, evt );
	LeaveCriticalSec( &l.csWscEvents );
}

static void DropHttpRequestEvent( HTTP_REQUEST_EVENT *evt ) {
	EnterCriticalSec( &l.csHttpRequestEvents );
	DeleteFromSet( HTTP_REQUEST_EVENT, l.httpRequestEvents, evt );
	LeaveCriticalSec( &l.csHttpRequestEvents );
}


static void uv_closed_wssi( uv_handle_t* handle ) {
	wssiObject* myself = (wssiObject*)handle->data;
	//myself->openCallback.Reset();
	INDEX idx;
	callbackFunction* c;
	LIST_FORALL( myself->messageCallbacks, idx, callbackFunction*, c ) c->callback.Reset();
	LIST_FORALL( myself->closeCallbacks, idx, callbackFunction*, c ) c->callback.Reset();
	LIST_FORALL( myself->errorCallbacks, idx, callbackFunction*, c ) c->callback.Reset();
	myself->_this.Reset();
}
static void uv_closed_wss( uv_handle_t* handle ) {
	wssObject* myself = (wssObject*)handle->data;
	myself->openCallback.Reset();
	myself->acceptCallback.Reset();
	myself->requestCallback.Reset();
	myself->errorCloseCallback.Reset();
	myself->_this.Reset();
}
static void uv_closed_wsc( uv_handle_t* handle ) {
	wscObject* myself = (wscObject*)handle->data;
	INDEX idx;
	callbackFunction* c;
	LIST_FORALL( myself->messageCallbacks, idx, callbackFunction*, c ) c->callback.Reset();
	LIST_FORALL( myself->closeCallbacks, idx, callbackFunction*, c ) c->callback.Reset();
	LIST_FORALL( myself->errorCallbacks, idx, callbackFunction*, c ) c->callback.Reset();
	LIST_FORALL( myself->openCallbacks, idx, callbackFunction*, c ) c->callback.Reset();
	myself->_this.Reset();
}

struct cgiParams {
	Isolate *isolate;
	Local<Object> cgi;
};

static void cgiParamSave(uintptr_t psv, PTEXT name, PTEXT value){
	struct cgiParams *cgi = (struct cgiParams*)psv;
	Isolate *isolate = cgi->isolate;
	Local<Context> context = cgi->isolate->GetCurrentContext();
	if( value )
		SETT( cgi->cgi, name, String::NewFromUtf8( cgi->isolate, GetText( value ), v8::NewStringType::kNormal ).ToLocalChecked() );
	else
		SETT( cgi->cgi, name, Null( cgi->isolate ) );
}

Local<Object> makeSocket( Isolate* isolate, PCLIENT pc, struct html5_web_socket* pipe, wssObject* wss, wscObject* wsc, wssiObject* wssi ) {
	Local<Context> context = isolate->GetCurrentContext();
	//wssi
	if( wss || wsc || wssi ) {
		if( wssi && wssi->wsPipe )
			pc = NULL;
	}
	PLIST headers = (pc)?GetWebSocketHeaders( pc ):wss?GetWebSocketPipeHeaders( wss->wsPipe ) : wssi ? GetWebSocketPipeHeaders( wssi->wsPipe ) : NULL;
	SOCKADDR *remoteAddress = ( pc ) ?(SOCKADDR *)GetNetworkLong( pc, GNL_REMOTE_ADDRESS ) : NULL;
	SOCKADDR *localAddress = ( pc ) ? (SOCKADDR *)GetNetworkLong( pc, GNL_LOCAL_ADDRESS ): NULL;
	Local<String> remote = String::NewFromUtf8( isolate, remoteAddress?GetAddrName( remoteAddress ):"0.0.0.0", v8::NewStringType::kNormal ).ToLocalChecked();
	Local<String> local = String::NewFromUtf8( isolate, localAddress?GetAddrName( localAddress ):"0.0.0.0", v8::NewStringType::kNormal ).ToLocalChecked();
	uint8_t mac[12];
	TEXTCHAR macText[36];
	size_t maclen = 12;
	uint8_t macRemote[12];
	TEXTCHAR macRemoteText[36];
	size_t macRemoteLen = 12;

	//lprintf( "Mac AAddress of socket:" );
	//LogBinary( mac, maclen );
	//LogBinary( macRemote, macRemoteLen );
	
	//	Local<String> localMac = String::NewFromUtf8( isolate, GetAddrName( remoteAddress ):"0.0.0.0", v8::NewStringType::kNormal ).ToLocalChecked();
	Local<Object> result = Object::New( isolate );
	Local<Object> arr = Object::New( isolate );
	INDEX idx;
	struct HttpField *header;
	LIST_FORALL( headers, idx, struct HttpField *, header ) {
		SETT( arr, header->name
			, String::NewFromUtf8( isolate, (const char*)GetText( header->value ), NewStringType::kNormal, (int)GetTextSize( header->value ) ).ToLocalChecked() );
	}
	optionStrings *strings = getStrings( isolate );
	if( headers )
		SETV( result, strings->headerString->Get( isolate ), arr );
	CTEXTSTR host = pc?ssl_GetRequestedHostName( pc ):NULL;
	if( host )
		SETV( result, strings->hostnameString->Get( isolate ), String::NewFromUtf8( isolate, host, v8::NewStringType::kNormal ).ToLocalChecked() );
	else
		SETV( result, strings->hostnameString->Get( isolate ), Null( isolate ) );

	if( remoteAddress )
	SETV( result, strings->remoteFamilyString->Get( isolate )
			, (remoteAddress->sa_family == AF_INET) ? strings->v4String->Get( isolate ) :
				(remoteAddress->sa_family == AF_INET6) ? strings->v6String->Get( isolate ) : strings->vUString->Get( isolate )
		);
	SETV( result, strings->remoteAddrString->Get( isolate ), remote );
	SETV( result, strings->remotePortString->Get( isolate ), Integer::New( isolate, pc?(int32_t)GetNetworkLong( pc, GNL_PORT ):0 ) );
	if( localAddress )
		SETV( result, strings->localFamilyString->Get( isolate )
			, (localAddress->sa_family == AF_INET)?strings->v4String->Get(isolate):
				(localAddress->sa_family == AF_INET6) ? strings->v6String->Get( isolate ) : strings->vUString->Get(isolate)
		);
	SETV( result, strings->localAddrString->Get( isolate ), local );
	SETV( result, strings->localPortString->Get( isolate ), Integer::New( isolate, pc?(int32_t)GetNetworkLong( pc, GNL_MYPORT ):0 ) );

	if( pc && ( ( wss && wss->resolveAddr ) || ( wsc && wsc->resolveAddr ) || ( wssi && wssi->resolveAddr ) ) ) {
		char tmp[NI_MAXHOST+1];
		#define SOCKADDR_LENGTH(sa) ( (int)*(uintptr_t*)( ( (uintptr_t)(sa) ) - 2*sizeof(uintptr_t) ) )
		if( remoteAddress ) {
			int addrres = getnameinfo( remoteAddress, SOCKADDR_LENGTH( remoteAddress ), tmp, NI_MAXHOST, NULL, 0, NI_NAMEREQD);
			if( !addrres ) {
				//lprintf( "Name info for address: %s %d", tmp, addrres );
				SETV( result, strings->remoteNameString->Get(isolate), String::NewFromUtf8( isolate, tmp ).ToLocalChecked() );
			} else {
				uint32_t dwError = WSAGetLastError();
				if( addrres == EAI_NONAME ) {
					SETV( result, strings->remoteNameString->Get(isolate), remote );
				}else
					lprintf( "name info address error: %d", addrres );
			}
		}
		if( localAddress ) {
			int addrres = getnameinfo( localAddress, SOCKADDR_LENGTH( localAddress ), tmp, NI_MAXHOST, NULL, 0, NI_NAMEREQD );
			if( !addrres ) {
				//lprintf( "Name info for address: %s %d", tmp, addrres );
				SETV( result, strings->localNameString->Get( isolate ), String::NewFromUtf8( isolate, tmp ).ToLocalChecked() );
			} else {
				uint32_t dwError = WSAGetLastError();
				if( addrres == EAI_NONAME ) {
					SETV( result, strings->localNameString->Get(isolate), local );
				}else
					lprintf( "name info address error: %d", addrres );
			}
			
		}
	}

	if( pc && ( ( wss && wss->resolveMac ) || ( wsc && wsc->resolveMac ) || ( wssi && wssi->resolveMac ) ) ) {
		if( !GetMacAddress( pc, mac, &maclen, macRemote, &macRemoteLen ) ) {
			strcpy( macText, "00:00:00:00:00:00" );
			strcpy( macRemoteText, "00:00:00:00:00:00" );
		} else {
			if( !maclen )
				snprintf( macText, 36, "00:00:00:00:00:00" );
			else
				snprintf( macText, 36, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
				if( !macRemoteLen )
					snprintf( macRemoteText, 36, "00:00:00:00:00:00" );
				else
					snprintf( macRemoteText, 36, "%02x:%02x:%02x:%02x:%02x:%02x", macRemote[0], macRemote[1], macRemote[2], macRemote[3], macRemote[4], macRemote[5] );
		}

		SETV( result, strings->remoteMacString->Get( isolate ), String::NewFromUtf8( isolate, macRemoteText, v8::NewStringType::kNormal ).ToLocalChecked() );
		SETV( result, strings->localMacString->Get( isolate ), String::NewFromUtf8( isolate, macText, v8::NewStringType::kNormal ).ToLocalChecked() );
	}


//	SETV( result, strings->localMacString->Get( isolate ), localMac );

	return result;
}

static Local<Value> makeRequest( Isolate *isolate, struct optionStrings *strings, PCLIENT pc, int sslRedirect, wssObject *wss ) {
	
	// .url
	// .socket
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> req = Object::New( isolate );
	Local<Object> socket;
	struct HttpState *pHttpState = pc?GetWebSocketHttpState( pc ):wss?GetWebSocketPipeHttpState( wss->wsPipe ):NULL;
	if( pHttpState ) {
		struct cgiParams cgi;
		PTEXT content;
		cgi.isolate = isolate;
		cgi.cgi = Object::New( isolate );
		ProcessCGIFields( pHttpState, cgiParamSave, (uintptr_t)&cgi );
		SETV( req, strings->redirectString->Get( isolate ), sslRedirect?True( isolate ):False(isolate) );
		SETV( req, strings->CGIString->Get( isolate ), cgi.cgi );
		SETV( req, strings->versionString->Get( isolate ), Integer::New( isolate, GetHttpVersion( pHttpState ) ) );
		if (content = GetHttpContent(pHttpState))
			SETV( req, strings->contentString->Get(isolate), String::NewFromUtf8(isolate, GetText(content), v8::NewStringType::kNormal).ToLocalChecked());
		else
			SETV( req, strings->contentString->Get(isolate), Null(isolate));
		if (!GetText(GetHttpRequest(pHttpState))) {
			//lprintf("lost request url");
			return Null( isolate );
		}
		else
			SETV( req, strings->urlString->Get(isolate)
				, String::NewFromUtf8(isolate
					, GetText(GetHttpRequest(pHttpState)),v8::NewStringType::kNormal).ToLocalChecked());
		//ResetHttpContent(pc, pHttpState);
	}
	SETV( req, strings->connectionString->Get( isolate ), socket = makeSocket( isolate, pc, wss->wsPipe, wss, NULL, NULL ) );
	SETV( req, strings->headerString->Get( isolate ), GETV( socket, strings->headerString->Get( isolate ) ) );
	return req;
}


static void acceptResolved(const v8::FunctionCallbackInfo<Value>& args ) {
	v8::Isolate* isolate = args.GetIsolate();
	Local<Object> wssi = args.Data().As<Object>();
	wssiObject *wssiInternal = wssiObject::Unwrap<wssiObject>( wssi );

	struct wssEvent *eventMessage = wssiInternal->acceptEventMessage;

	//lprintf( "Resolve called on promise...");
	if( eventMessage ) {
		wssiInternal->acceptEventMessage = NULL;
		eventMessage->done = TRUE;
		eventMessage->data.request.accepted = 1;
		eventMessage->result = wssiInternal;
		args.GetReturnValue().Set( wssiInternal->acceptPromise.Get(isolate) );
		wssiInternal->acceptPromise.Reset();
		if( eventMessage->waiter )
			WakeThread( eventMessage->waiter );
	}
}

static void acceptRejected(const v8::FunctionCallbackInfo<Value>& args ) {
	v8::Isolate* isolate = args.GetIsolate();
	Local<Object> wssi = args.Data().As<Object>();
	wssiObject *wssiInternal = wssiObject::Unwrap<wssiObject>( wssi );

	struct wssEvent *eventMessage = wssiInternal->acceptEventMessage;

	//lprintf( "Reject called on promise...");
	if( eventMessage ) { // this is released on post...
		wssiInternal->acceptEventMessage = NULL;
		eventMessage->done = TRUE;
		eventMessage->data.request.accepted = 0;
		args.GetReturnValue().Set( wssiInternal->acceptPromise.Get(isolate) );
		wssiInternal->acceptPromise.Reset();

		if( eventMessage->waiter )
			WakeThread( eventMessage->waiter );
	}
}

static void uv_closed_httpRequest( uv_handle_t* handle ) {
	httpRequestObject* myself = (httpRequestObject*)handle->data;

	myself->_this.Reset();
	delete myself;
}

static void httpRequestAsyncMsg( uv_async_t* handle ) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	httpRequestObject* myself = (httpRequestObject*)handle->data;
	struct HTTPRequestOptions *opts = myself->opts;

	Local<Context> context = isolate->GetCurrentContext();
	{
		struct httpRequestEvent* eventMessage;
		while( eventMessage = (struct httpRequestEvent*)DequeLink( &myself->eventQueue ) ) {
			Local<Value> argv[1];
			Local<ArrayBuffer> ab;
			switch( eventMessage->eventType ) {

			case WS_EVENT_RESPONSE:
			{
				HTTPState state;
				Local<Function> cb;

				Local<Object> result; result = Object::New( isolate );

				state = eventMessage->state;
				{
					HTTPRequestOptions* opts; opts = myself->opts;
					char* header;
					INDEX idx;
					// cleanup what we allocated.
					LIST_FORALL( opts->headers, idx, char*, header )
						Release( header );
					DeleteList( &opts->headers );
				}
				if( !state ) {
					SET( result, "error",
						state ? String::NewFromUtf8Literal( isolate, "No Content" ) : String::NewFromUtf8Literal( isolate, "Connect Error" ) );

					cb = Local<Function>::New( isolate, myself->resultCallback );
					argv[0] = result;
					cb->Call( context, myself->_this.Get( isolate ), 1, argv );

					//args.GetReturnValue().Set( result );
					myself->_this.Reset();
					myself->resultCallback.Reset();

					break;
				}
				PTEXT content; content = GetHttpContent( state );
				if( state && GetHttpResponseCode( state ) ) {
					if( content ) {
						SET( result, "content"
							, String::NewFromUtf8( isolate, GetText( content ), v8::NewStringType::kNormal ).ToLocalChecked() );
					}
					SET( result, "statusCode"
						, Integer::New( isolate, GetHttpResponseCode( state ) ) );
					const char* textCode = GetHttpResponseStatus( state );

					SET( result, "status"
						, String::NewFromUtf8( isolate, textCode ? textCode : "NO RESPONSE", v8::NewStringType::kNormal ).ToLocalChecked() );
					Local<Array> arr = Array::New( isolate );
					PLIST headers = GetHttpHeaderFields( state );
					INDEX idx;
					struct HttpField* header;
					//headers
					LIST_FORALL( headers, idx, struct HttpField*, header ) {
						SETT( arr, header->name
							, String::NewFromUtf8( isolate, (const char*)GetText( header->value )
								, NewStringType::kNormal, (int)GetTextSize( header->value ) ).ToLocalChecked() );
					}
					SET( result, "headers", arr );

					DestroyHttpState( state );
				} else {
					SET( result, "error",
						state ? String::NewFromUtf8Literal( isolate, "No Content" )
						: String::NewFromUtf8Literal( isolate, "Connect Error" ) );

				}

				LineRelease( opts->url );
				cb = Local<Function>::New( isolate, myself->resultCallback );
				argv[0] = result;
				cb->Call( context, Undefined(isolate), 1, argv );
				Deallocate( HTTPRequestOptions*, myself->opts);

				DeleteLinkQueue(&myself->eventQueue);
#ifdef DEBUG_EVENTS				
				lprintf( "Sack uv_close1 %p", &myself->async );
#endif				
				myself->_this.Reset();
				myself->resultCallback.Reset();
				uv_close( (uv_handle_t*)&myself->async, uv_closed_httpRequest );
				DropHttpRequestEvent( eventMessage );
				break;
			}
			}
		}
	}
}

static void wssiAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	wssiObject* myself = (wssiObject*)handle->data;

	Local<Context> context = isolate->GetCurrentContext();
	{
		struct wssiEvent* eventMessage;
		INDEX idx;
		callbackFunction* callback;
		while( eventMessage = ( struct wssiEvent* )DequeLink( &myself->eventQueue ) ) {
			Local<Value> argv[2];
			Local<ArrayBuffer> ab;
			switch( eventMessage->eventType ) {
			case WS_EVENT_OPEN:
				if( !myself->thrown && !myself->server->openCallback.IsEmpty() ) {
					Local<Function> cb = Local<Function>::New( isolate, myself->server->openCallback );
					argv[0] = myself->_this.Get( isolate );
					cb->Call( context, myself->_this.Get( isolate ), 1, argv );
				}
				break;
			case WS_EVENT_READ:
				LIST_FORALL( myself->messageCallbacks, idx, callbackFunction*, callback ) {
					//if( !myself->messageCallback.IsEmpty() ) {
					if( eventMessage->binary ) {
#if ( NODE_MAJOR_VERSION >= 14 )
						std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( (POINTER)eventMessage->buf, eventMessage->buflen, releaseBufferBackingStore, NULL );
						ab = ArrayBuffer::New( isolate, bs );
#else
						ab =
							ArrayBuffer::New( isolate
								, (void*)eventMessage->buf
								, eventMessage->buflen );

#endif

						PARRAY_BUFFER_HOLDER holder = GetHolder();
						holder->o.Reset( isolate, ab );
						holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
						holder->buffer = eventMessage->buf;
						argv[0] = ab;

						callback->callback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 1, argv );
						// buf will deallocate when the arraybuffer does.
					}
					else {
						MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
						argv[0] = buf.ToLocalChecked();
						//lprintf( "Message:', %s", eventMessage->buf );
						callback->callback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 1, argv );
						Deallocate( CPOINTER, eventMessage->buf );
					}
				}
				break;
			case WS_EVENT_CLOSE:
				argv[0] = Integer::New( isolate, eventMessage->code );
				if( eventMessage->buf ) {
					MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
					argv[1] = buf.ToLocalChecked();
					Deallocate( CPOINTER, eventMessage->buf );
				}
				else
					argv[1] = Null( isolate );
				LIST_FORALL( myself->closeCallbacks, idx, callbackFunction*, callback ) {
					callback->callback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 2, argv );
				}
#ifdef DEBUG_EVENTS				
				lprintf( "Sack uv_close2 %p", &myself->async);
#endif
				uv_close( (uv_handle_t*)&myself->async, uv_closed_wssi );
				DropWssiEvent( eventMessage );
				DeleteLinkQueue( &myself->eventQueue );
				myself->closed = 1;
				myself->readyState = CLOSED;
				continue;
				break;
			case WS_EVENT_ERROR:
				LIST_FORALL( myself->errorCallbacks, idx, callbackFunction*, callback ) {
					callback->callback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 0, argv );
				}
				break;
			}
			DropWssiEvent( eventMessage );
		}
	}
	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}

static void handleWebSockEOF( uintptr_t psv ) {
	wssiObject* wssiInternal = (wssiObject*)psv;
	lprintf( "get EOF in websocket - this means, for websocket, close" );
	WebSocketPipeClose( wssiInternal->wsPipe, 1006, "Underlaying channel at EOF" );
	wssiInternal->wsPipe = NULL; // have to use server->wsPipe to get the pipe.
}

static void handleWebSockClose( uintptr_t psv ) {
	wssiObject* wssiInternal = (wssiObject*)psv;
	lprintf( "get close in websocket - this means, for websocket, close" );
	if( wssiInternal->wsPipe ) // prevent double close.
		WebSocketPipeClose( wssiInternal->wsPipe, 1006, "Underlaying channel closed." );
}

static void wssAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	wssObject* myself = (wssObject*)handle->data;
	v8::Isolate* isolate = myself->isolate;//v8::Isolate::GetCurrent();
	HandleScope scope(isolate);
	Local<Context> context = isolate->GetCurrentContext();
	//class constructorSet *c = getConstructors( isolate);
	int handled = 0;
	{
		struct wssEvent *eventMessage;
		while( eventMessage = (struct wssEvent *)DequeLink( &myself->eventQueue ) ) {
			Local<Value> argv[3];
			handled++;
			//lprintf( "handling an event from somewhere %p  %s", GetWebSocketHttpState( eventMessage->pc ), GetText( GetHttpRequest( GetWebSocketHttpState( eventMessage->pc ) ) ) );
			myself->eventMessage = eventMessage;
			if( eventMessage->eventType == WS_EVENT_LOW_ERROR ) {
				if( !myself->errorLowCallback.IsEmpty() ) {
					argv[0] = Integer::New( isolate, eventMessage->data.error.error );
					argv[1] = makeSocket( isolate, eventMessage->pc, NULL, myself, NULL, NULL );
					if( eventMessage->data.error.buffer ) {
#if ( NODE_MAJOR_VERSION >= 14 )
						std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( (void*)eventMessage->data.error.buffer,
							eventMessage->data.error.buflen, dontReleaseBufferBackingStore, NULL );
						argv[2] = ArrayBuffer::New( isolate, bs );
#else
					if( eventMessage->data.error.buffer )
						argv[2] = ArrayBuffer::New( isolate,
						(void*)eventMessage->data.error.buffer,
							eventMessage->data.error.buflen );
#endif
					} else
						argv[2] = Null( isolate );
					myself->errorLowCallback.Get( isolate )->Call( context, myself->_this.Get( isolate ), 3, argv );
				}

			} else if( eventMessage->eventType == WS_EVENT_REQUEST ) {
				//lprintf( "Comes in directly as a request; don't even get accept..." );
				if( !myself->requestCallback.IsEmpty() ) {
					class constructorSet *c = getConstructors( isolate );
					Local<Function> cons = Local<Function>::New( isolate, c->httpConstructor );
					MaybeLocal<Object> _http = cons->NewInstance( isolate->GetCurrentContext(), 0, argv );
					if( _http.IsEmpty() ) {
						isolate->ThrowException( Exception::Error(
							String::NewFromUtf8( isolate, TranslateText( "Internal Error request instance." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
						break;
					}
					Local<Object> http = _http.ToLocalChecked();
					struct optionStrings *strings = getStrings( isolate );
					SETV( http, strings->connectionString->Get( isolate ), makeSocket( isolate, eventMessage->pc, myself->wsPipe, myself, NULL, NULL ) );

					httpObject *httpInternal = httpObject::Unwrap<httpObject>( http );
					httpInternal->wss = myself;
					if( eventMessage->pc )
						httpInternal->ssl = ssl_IsClientSecure( eventMessage->pc );
					int sslRedirect = (httpInternal->ssl != myself->ssl);
					httpInternal->pc = eventMessage->pc;
#if USE_NETWORK_AGGREGATE 
					SetTCPWriteAggregation( eventMessage->pc, TRUE );
#endif
					AddLink( &myself->requests, httpInternal );
					//if( myself->requests->Cnt > 200 )
					//	DebugBreak();
					//lprintf( "requests %p is %d", myself->requests, myself->requests->Cnt );
					//lprintf( "New request..." );
					argv[0] = makeRequest( isolate, strings, eventMessage->pc, sslRedirect, myself );
					if( !argv[0]->IsNull() ) {
						argv[1] = http;
						Local<Function> cb = Local<Function>::New( isolate, myself->requestCallback );
						cb->Call( context, eventMessage->_this->_this.Get( isolate ), 2, argv );
						// and then even after this returns, the write might be pending...
					}
					//else
					//	lprintf( "This request was a false alarm, and was empty." );
				}
			} else if( eventMessage->eventType == WS_EVENT_ACCEPT ) {
				//lprintf( "Dispatch open event because connect." );
				class constructorSet *c = getConstructors( isolate );
				Local<Function> cons = Local<Function>::New( isolate, c->wssiConstructor );
				MaybeLocal<Object> _wssi = cons->NewInstance( isolate->GetCurrentContext(), 0, argv );
				if( _wssi.IsEmpty() ) {
					isolate->ThrowException( Exception::Error(
								String::NewFromUtf8( isolate, TranslateText( "Internal Error creating client connection instance." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
					lprintf( "Internal error. I wonder what exception looks like" );
					break;
				}
				Local<Object> wssi = _wssi.ToLocalChecked();
				SET_READONLY( wssi, "OPEN", Integer::New( isolate, wsReadyStates::OPEN ) );
				SET_READONLY( wssi, "CLOSED", Integer::New( isolate, wsReadyStates::CLOSED ) );
				SET_READONLY( wssi, "CLOSING", Integer::New( isolate, wsReadyStates::CLOSING ) );
				SET_READONLY( wssi, "CONNECTING", Integer::New( isolate, wsReadyStates::CONNECTING ) );
				SET_READONLY( wssi, "INITIALIZING", Integer::New( isolate, wsReadyStates::INITIALIZING ) );
				struct optionStrings *strings = getStrings( isolate );
				Local<Object> socket;
				wssiObject *wssiInternal = wssiObject::Unwrap<wssiObject>( wssi );

				wssiInternal->resolveAddr = myself->resolveAddr;
				wssiInternal->resolveMac = myself->resolveMac;
				struct html5_web_socket* ws = ( !eventMessage->_this->pc ) ? (struct html5_web_socket*)eventMessage->pc : NULL;
				if( ws ) {
					wssiInternal->wsPipe = ws;
					wssiInternal->pc = NULL;
				} else {
					wssiInternal->pc = eventMessage->pc;
					wssiInternal->wsPipe = NULL;
				}

				// events are setup in the ssh_module when the channel is accepted
				//  
				wssiInternal->server = myself;
				uv_async_init( c->loop, &wssiInternal->async, wssiAsyncMsg );
				
				AddLink( &myself->opening, wssiInternal );
				eventMessage->result = wssiInternal;

				SETV( wssi, strings->connectionString->Get(isolate), socket = makeSocket( isolate, eventMessage->pc, wssiInternal->wsPipe, NULL, NULL, wssiInternal ) );
				SETV( wssi, strings->headerString->Get( isolate ), GETV( socket, strings->headerString->Get( isolate ) ) );

				struct HttpState* pHttpState = wssiInternal->pc ? GetWebSocketHttpState( wssiInternal->pc )
					: wssiInternal->wsPipe ? GetWebSocketPipeHttpState( wssiInternal->wsPipe ) : NULL;
				{
					struct cgiParams cgi;
					cgi.isolate = isolate;
					cgi.cgi = Object::New( isolate );
					ProcessCGIFields( pHttpState, cgiParamSave, (uintptr_t)&cgi );
					SETV( wssi, strings->CGIString->Get( isolate ), cgi.cgi );
				}

				SETV( wssi, strings->urlString->Get( isolate )
					, String::NewFromUtf8( isolate
						, eventMessage->data.request.resource, v8::NewStringType::kNormal ).ToLocalChecked() );

				argv[0] = wssi;
				wssiInternal->acceptEventMessage = eventMessage;

				if( !myself->acceptCallback.IsEmpty() ) {
					Local<Function> cb = myself->acceptCallback.Get( isolate );
					// return can be a promise which will block the accept until resolved.
					MaybeLocal<Value> result = cb->Call( context, eventMessage->_this->_this.Get( isolate ), 1, argv );
					if( wssiInternal->blockReturn ) {
						continue;
					}
					if( !result.IsEmpty() ) {
						Local<Value> ret = result.ToLocalChecked();
						if( ret->IsPromise() ) { 
							Local<Promise> retPro = ret.As<Promise>();
							if(  !retPro.IsEmpty() ){
								Promise::PromiseState s = retPro->State();
								if( s == Promise::PromiseState::kPending ) {
									if( c->promiseThen.IsEmpty()){
										Local<Function> cons = Local<Function>::Cast(
											retPro->Get( context, String::NewFromUtf8( isolate, "then", NewStringType::kNormal ).ToLocalChecked() ).ToLocalChecked());
										c->promiseThen.Reset( isolate, cons );
									}
									if( c->promiseCatch.IsEmpty()){
										Local<Function> cons = Local<Function>::Cast(
											retPro->Get( context, String::NewFromUtf8( isolate, "catch", NewStringType::kNormal ).ToLocalChecked() ).ToLocalChecked());
										c->promiseCatch.Reset( isolate, cons );

									}
									//lprintf( "Register thren/cancel callbacks on pending..." );
									Local<Function> promiseResolved = v8::Function::New( isolate->GetCurrentContext(), acceptResolved, wssi ).ToLocalChecked();
									Local<Function> promiseRejected = v8::Function::New( isolate->GetCurrentContext(), acceptRejected, wssi ).ToLocalChecked();

									Local<Value> argv[1] = { promiseResolved };
									Local<Value> result = c->promiseThen.Get( isolate )->Call( context, retPro, 1, argv ).ToLocalChecked();
									argv[0] = { promiseRejected };
									result = c->promiseCatch.Get( isolate )->Call( context, retPro, 1, argv ).ToLocalChecked();

									//wssiInternal->acceptPromise.Reset( isolate, retPro );
									//promiseRejected->
									//Then (Local< Context > context, Local< Function > on_fulfilled, Local< Function > on_rejected)
									//MaybeLocal<Promise> retval = retPro->Then( context, promiseResolved, promiseRejected );
									continue;
								}
								else if( s == Promise::PromiseState::kFulfilled ) {
									Local<Value> e = retPro->Result();
									String::Utf8Value s( isolate, e->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
									lprintf( "Is already fulfulled, no problem... %s", *s );
								}
								else if( s == Promise::PromiseState::kRejected ) {
									Local<Value> e = retPro->Result();
									String::Utf8Value s( isolate, e->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
									lprintf( "Bad news, it's rejected, but... ok. %s", *s );
									//break;
								}
								//  TypeError: Method Promise.prototype.then called on incompatible receiver undefined at then (<anonymous>)
								// don't set done, keep blocking the acceptor...
								continue;
							}
						}
					}
				}
				else {
					WebSocketAccept( wssiInternal->pc, (char*)eventMessage->data.request.protocol, TRUE );
				}
			} else if( eventMessage->eventType == WS_EVENT_ERROR_CLOSE ) {
				Local<Object> closingSock = makeSocket( isolate, eventMessage->pc, myself->wsPipe, myself, NULL, NULL );
				if( !myself->errorCloseCallback.IsEmpty() ) {
					Local<Function> cb = myself->errorCloseCallback.Get( isolate );
					argv[0] = closingSock;
					cb->Call( context, eventMessage->_this->_this.Get( isolate ), 1, argv );
				}

				//myself->readyState = CLOSED;
				//uv_close( (uv_handle_t*)&myself->async, uv_closed_wss );
				//DeleteLinkQueue( &myself->eventQueue );
				//return;
			} else if( eventMessage->eventType == WS_EVENT_RELEASE_BUFFER ) {
				// allow buffer to get freed through GC system.
				eventMessage->data.write->buffer.Reset();
			} else if( eventMessage->eventType == WS_EVENT_CLOSE ) {
				myself->readyState = CLOSED;
				lprintf( "Sack uv_close2 %p", &myself->async);

				uv_close( (uv_handle_t*)&myself->async, uv_closed_wss );
				if( !myself->closeCallback.IsEmpty() ) {
					Local<Function> cb = myself->closeCallback.Get( isolate );
					cb->Call( context, eventMessage->_this->_this.Get( isolate ), 0, NULL );
				}
				DropWssEvent( eventMessage );
				DeleteLinkQueue( &myself->eventQueue );
				return;
			}

			myself->eventMessage = NULL;
			eventMessage->done = TRUE;
			if( eventMessage->waiter )
				WakeThread( eventMessage->waiter );
			DropWssEvent( eventMessage );
		}
		myself->last_count_handled = handled;
	}
	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}

static void wscAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope(isolate);
	wscObject* wsc = (wscObject*)handle->data;
	wscEvent *eventMessage;
	Local<Context> context = isolate->GetCurrentContext();

	{
		Local<Value> argv[2];
		while( eventMessage = (struct wscEvent*)DequeLink( &wsc->eventQueue ) ) {
			Local<Function> cb;
			Local<ArrayBuffer> ab;
			switch( eventMessage->eventType ) {
			case WS_EVENT_OPEN:
				//cb = Local<Function>::New( isolate, wsc->openCallback );
				//if( !cb.IsEmpty() ) 
				{
					struct optionStrings *strings;
					strings = getStrings( isolate );
					SETV( wsc->_this.Get( isolate ), strings->connectionString->Get( isolate ), makeSocket( isolate, wsc->pc, NULL, NULL, wsc, NULL ) );

					INDEX idx;
					callbackFunction* callback;
					LIST_FORALL( wsc->openCallbacks, idx, callbackFunction*, callback ) {
						callback->callback.Get(isolate)->Call( context, eventMessage->_this->_this.Get( isolate ), 0, argv );
					}
				}
				break;
			case WS_EVENT_READ:
				if( eventMessage->binary ) {
#if ( NODE_MAJOR_VERSION >= 14 )
					std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( (POINTER)eventMessage->buf, eventMessage->buflen, releaseBufferBackingStore, NULL );
					ab = ArrayBuffer::New( isolate,bs );
#else
					ab =
						ArrayBuffer::New( isolate,
						(void*)eventMessage->buf,
							eventMessage->buflen );

					PARRAY_BUFFER_HOLDER holder = GetHolder();
					holder->o.Reset( isolate, ab );
					holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
					holder->buffer = eventMessage->buf;
#endif
					argv[0] = ab;
					INDEX idx; callbackFunction* callback;
					LIST_FORALL( wsc->messageCallbacks, idx, callbackFunction*, callback ) {
						callback->callback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 1, argv );
					}
				}
				else {
					MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
					argv[0] = buf.ToLocalChecked();
					INDEX idx; callbackFunction* callback;
					LIST_FORALL( wsc->messageCallbacks, idx, callbackFunction*, callback ) {
						callback->callback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 1, argv );
					}
				}
				Deallocate( CPOINTER, eventMessage->buf );
				break;
			case WS_EVENT_ERROR:
				{
					INDEX idx; callbackFunction* callback;
					LIST_FORALL( wsc->errorCallbacks, idx, callbackFunction*, callback ) {
						callback->callback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 0, argv );
					}
				}
				break;
			case WS_EVENT_CLOSE:
				{
					argv[0] = Integer::New( isolate, eventMessage->code );
					if( eventMessage->buf ) {
						MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
						argv[1] = buf.ToLocalChecked();
						Deallocate( CPOINTER, eventMessage->buf );
					}
					else
						argv[1] = Null( isolate );
					INDEX idx; callbackFunction* callback;
					LIST_FORALL( wsc->closeCallbacks, idx, callbackFunction*, callback ) {
						callback->callback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 2, argv );
					}
#ifdef DEBUG_EVENTS				
					lprintf( "Sack uv_close3 %p", &wsc->async);
#endif					
					uv_close( (uv_handle_t*)&wsc->async, uv_closed_wsc );
					DeleteLinkQueue( &wsc->eventQueue );
					wsc->readyState = CLOSED;
					wsc->closed = TRUE;
				}
				break;
			}
			DropWscEvent( eventMessage );
		}
	}
	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}




static LOGICAL PostClientSocket( Isolate *isolate, String::Utf8Value *name, wssiObject* obj ) {
	if( !obj->acceptEventMessage ) {
		lprintf( "Don't double post a socket." );
		return TRUE; // success though.
	}
	{
		struct socketUnloadStation* station;
		INDEX idx;
		LIST_FORALL( l.transportDestinations, idx, struct socketUnloadStation*, station ) {
			if( memcmp( *station->s[0], *(name[0]), (name[0]).length() ) == 0 ) {
				struct socketTransport* trans = new struct socketTransport();
				trans->wssi = obj;
				trans->acceptEventMessage = obj->acceptEventMessage;

				obj->blockReturn = TRUE;
				obj->acceptEventMessage = NULL;
				trans->protocolResponse = obj->protocolResponse;
				obj->protocolResponse = NULL;
				AddLink( &station->transport, trans );
#ifdef DEBUG_EVENTS
				lprintf( "Send Post Request %p", &station->clientSocketPoster );
#endif				
				uv_async_send( &station->clientSocketPoster );

				break;
			}
		}
		if( !station ) {
			return FALSE;
			//isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Failed to find target accepting thread" ) ) );
		}
	}
	return TRUE;
}

static void postClientSocket( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Required parameter missing: (unique,socket)" ) ) );
		return;
	}
	wssiObject* obj = wssObject::Unwrap<wssiObject>( args[1].As<Object>() );
	if( obj ){
		String::Utf8Value s( isolate, args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		if( PostClientSocket( isolate, &s, obj ) ) 
			args.GetReturnValue().Set( True(isolate) );
		else
			args.GetReturnValue().Set( False(isolate) );
		
	}
	else {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Second paramter is not an accepted socket" ) ) );
	}
}



static void postClientSocketObject( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 1 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Required parameter missing: (unique)" ) ) );
		return;
	}
	wssiObject* obj = wssObject::Unwrap<wssiObject>( args.This() );
	if( obj ){
		String::Utf8Value s( isolate, args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		if( PostClientSocket( isolate, &s, obj ) ) 
			args.GetReturnValue().Set( True(isolate) );
		else
			args.GetReturnValue().Set( False(isolate) );
	}
	else {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Object is not an accepted socket" ) ) );
	}
}


static void blockClientSocketAccept( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	wssiObject* obj = wssObject::Unwrap<wssiObject>( args.This() );
	if( obj ) {
		obj->blockReturn = TRUE;
	}
}

static void resumeClientSocketAccept( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	wssiObject* obj = wssObject::Unwrap<wssiObject>( args.This() );
	if( obj && obj->acceptEventMessage ) {
		obj->acceptEventMessage->data.request.accepted = 1;
		obj->acceptEventMessage->done = TRUE;
		if( obj->acceptEventMessage->waiter )
			WakeThread( obj->acceptEventMessage->waiter );
	}
}

static void finishPostClose( uv_handle_t *async ) {
	struct socketUnloadStation* unload = ( struct socketUnloadStation* )async->data;
	delete unload->s;
	delete unload;
}

static void handlePostedClient( uv_async_t* async ) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	struct socketUnloadStation* unload = ( struct socketUnloadStation* )async->data;
	Local<Function> f = unload->cb.Get( isolate );
	INDEX idx;
	struct socketTransport* trans;
	LIST_FORALL( unload->transport, idx, struct socketTransport*, trans ) {

		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->wssiConstructor );

		Local<Object> newThreadSocket = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
		SET_READONLY( newThreadSocket, "OPEN", Integer::New( isolate, wsReadyStates::OPEN ) );
		SET_READONLY( newThreadSocket, "CLOSED", Integer::New( isolate, wsReadyStates::CLOSED ) );
		SET_READONLY( newThreadSocket, "CLOSING", Integer::New( isolate, wsReadyStates::CLOSING ) );
		SET_READONLY( newThreadSocket, "CONNECTING", Integer::New( isolate, wsReadyStates::CONNECTING ) );
		SET_READONLY( newThreadSocket, "INITIALIZING", Integer::New( isolate, wsReadyStates::INITIALIZING ) );

		Local<Value> args[] = { localStringExternal( isolate, **( unload->s ), unload->s->length() ), newThreadSocket };
		wssiObject* obj = wssiObject::Unwrap<wssiObject>( newThreadSocket );
		obj->acceptEventMessage = trans->acceptEventMessage;
		obj->server = trans->wssi->server;
		uv_async_init( c->loop, &obj->async, wssiAsyncMsg );
		{
			INDEX idx2 = FindLink( &obj->server->opening, trans->wssi );
			SetLink( &obj->server->opening, idx2, obj );
		}
		obj->protocolResponse = trans->protocolResponse;
		obj->pc = trans->wssi->pc;
		obj->thrown = TRUE;
		delete obj->wssiRef;
		obj->wssiRef = trans->wssi->wssiRef;
		obj->wssiRef->wssi = obj; // update reference to be this JS object.
		/* 
			connectionString, headerString, urlString are all missing
		*/
		struct optionStrings *strings;
		strings = getStrings( isolate );
		Local<Object> socket;
		SETV( newThreadSocket, strings->connectionString->Get(isolate), socket = makeSocket( isolate, obj->pc, NULL, NULL, NULL, trans->wssi ) );
		SETV( newThreadSocket, strings->headerString->Get( isolate ), GETV( socket, strings->headerString->Get( isolate ) ) );
		SETV( newThreadSocket, strings->urlString->Get( isolate )
			, String::NewFromUtf8( isolate
				, GetText( GetHttpRequest( GetWebSocketHttpState( obj->pc ) ) ), v8::NewStringType::kNormal ).ToLocalChecked() );

		MaybeLocal<Value> ml_result = f->Call( context, unload->this_.Get(isolate), 2, args );
		if( !ml_result.IsEmpty() ) {
			Local<Value> result = ml_result.ToLocalChecked();
			if( result->TOBOOL(isolate) ) {
			}
		}
		//lprintf( "Cleanup this event.." );
		unload->cb.Reset();
		unload->this_.Reset();
		DeleteLink( &l.transportDestinations, unload );
		SetLink( &unload->transport, 0, NULL );
#ifdef DEBUG_EVENTS				
		lprintf( "Sack uv_close3 %p", async);
#endif
		uv_close( (uv_handle_t*)async, finishPostClose ); // have to hold onto the handle until it's freed.
		break;
	}
}

static void setClientSocketHandler( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	class constructorSet* c = getConstructors( isolate );
	String::Utf8Value unique( isolate, args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );

	Local<Function> f = args[1].As<Function>();
	struct socketUnloadStation* unloader = new struct socketUnloadStation();
	unloader->this_.Reset( isolate, args.This() );
	unloader->s = new String::Utf8Value( isolate, args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	unloader->cb.Reset( isolate, f );
	unloader->targetThread = c->loop;
	unloader->clientSocketPoster.data = unloader;
	unloader->transport = NULL;
	//lprintf( "New async event handler for this unloader%p", &unloader->clientSocketPoster );

	uv_async_init( unloader->targetThread, &unloader->clientSocketPoster, handlePostedClient );

	AddLink( &l.transportDestinations, unloader );
}

void InitWebSocket( Isolate *isolate, Local<Object> exports ){
	Local<Context> context = isolate->GetCurrentContext();

	l.jsThread = MakeThread();
	//NetworkWait( NULL, 2000000, 16 );  // 1GB memory
	InitializeCriticalSec( &l.csWssEvents );
	InitializeCriticalSec( &l.csWssiEvents );
	InitializeCriticalSec( &l.csWscEvents );
	InitializeCriticalSec( &l.csHttpRequestEvents );
	InitializeCriticalSec( &l.csConnect );
	InitializeCriticalSec( &l.csOpen );
	Local<Object> o = Object::New( isolate );

	Local<Object> wsWebStatesObject = Object::New( isolate );
	SET_READONLY( wsWebStatesObject, "OPEN", Integer::New( isolate, wsReadyStates::OPEN ) );
	SET_READONLY( wsWebStatesObject, "CLOSED", Integer::New( isolate, wsReadyStates::CLOSED ) );
	SET_READONLY( wsWebStatesObject, "CLOSING", Integer::New( isolate, wsReadyStates::CLOSING ) );
	SET_READONLY( wsWebStatesObject, "CONNECTING", Integer::New( isolate, wsReadyStates::CONNECTING ) );
	SET_READONLY( wsWebStatesObject, "INITIALIZING", Integer::New( isolate, wsReadyStates::INITIALIZING ) );
	SET_READONLY( wsWebStatesObject, "LISTENING", Integer::New( isolate, wsReadyStates::LISTENING ) );
	SET_READONLY( o, "readyStates", wsWebStatesObject );

	SET_READONLY( exports, "WSBanana", o );
	SET_READONLY( exports, "WebSocket", o );
	class constructorSet *c = getConstructors( isolate );
	{
		Local<FunctionTemplate> httpTemplate;
		httpTemplate = FunctionTemplate::New( isolate, httpObject::New );
		httpTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.core.http.requestHandler" ) );
		httpTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( httpTemplate, "writeHead", httpObject::writeHead );
		NODE_SET_PROTOTYPE_METHOD( httpTemplate, "end", httpObject::end );
		httpTemplate->ReadOnlyPrototype();

		c->httpConstructor.Reset( isolate, httpTemplate->GetFunction(context).ToLocalChecked() );

		// this is not exposed as a class that javascript can create.
		//SET_READONLY( o, "R", wscTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	}
	{
		Local<FunctionTemplate> httpRequestTemplate;
		httpRequestTemplate = FunctionTemplate::New( isolate, httpRequestObject::New );
		httpRequestTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.core.HTTP[S]" ) );
		httpRequestTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( httpRequestTemplate, "on", httpRequestObject::on );
		NODE_SET_PROTOTYPE_METHOD( httpRequestTemplate, "wait", httpRequestObject::wait );
		httpRequestTemplate->ReadOnlyPrototype();
		c->httpReqConstructor.Reset( isolate, httpRequestTemplate->GetFunction(context).ToLocalChecked() );

		Local<Object> oHttps = Object::New( isolate );
		SET_READONLY( exports, "HTTPS", oHttps );
		SET_READONLY_METHOD( oHttps, "get", httpRequestObject::gets );

		Local<Object> oHttp = Object::New( isolate );
		SET_READONLY( exports, "HTTP", oHttp );
		SET_READONLY_METHOD( oHttp, "get", httpRequestObject::get );

		// this is not exposed as a class that javascript can create.
		//SET_READONLY( o, "R", wscTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	}
	{
		Local<Object> threadObject = Object::New( isolate );
		SET_READONLY_METHOD( threadObject, "post", postClientSocket );
		SET_READONLY_METHOD( threadObject, "accept", setClientSocketHandler );
		SET_READONLY( o, "Thread", threadObject );
	}
	{
		SET_READONLY_METHOD( o, "ssh2ws", promoteSSH_to_Websocket );
	}
	{
		Local<FunctionTemplate> wssTemplate;
		wssTemplate = FunctionTemplate::New( isolate, wssObject::New );
		wssTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.core.ws.server" ) );
		wssTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "close", wssObject::close );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "disableSSL", wssObject::disableSSL );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "on", wssObject::on );

		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onconnect", wssObject::onConnect );
		wssTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "onconnect" )
			, Local<FunctionTemplate>()
			, FunctionTemplate::New( isolate, wssObject::onConnect )
		);
		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onaccept", wssObject::onAccept );
		wssTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "onaccept" )
			, Local<FunctionTemplate>()
			, FunctionTemplate::New( isolate, wssObject::onAccept )
		);
		wssTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "onrequest" )
			, Local<FunctionTemplate>()
			, FunctionTemplate::New( isolate, wssObject::onRequest )
		);
		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onclose", wssObject::onClose );
		wssTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "onclose" )
			, FunctionTemplate::New( isolate, wssObject::getOnClose )
			, FunctionTemplate::New( isolate, wssObject::onClose )
		);
		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onerror", wssObject::onError );
		wssTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "onerror" )
			, Local<FunctionTemplate>()
			, FunctionTemplate::New( isolate, wssObject::onError )
		);
		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onerrorlow", wssObject::onErrorLow );
		wssTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "onerrorlow" )
			, Local<FunctionTemplate>()
			, FunctionTemplate::New( isolate, wssObject::onErrorLow )
		);
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "accept", wssObject::accept );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "reject", wssObject::reject );

		wssTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "readyState" )
			, FunctionTemplate::New( isolate, wssObject::getReadyState )
			, Local<FunctionTemplate>() );
		//wssTemplate->SetNativeDataProperty( String::NewFromUtf8Literal( isolate, "readyState" )
		//	, wssObject::getReadyState
		//	, NULL );
		wssTemplate->ReadOnlyPrototype();
		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onmessage", wsObject::on );
		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "on", wsObject::on );

		c->wssConstructor.Reset( isolate, wssTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
		c->wssTpl.Reset( isolate, wssTemplate );

		SET_READONLY( o, "Server", wssTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	}

	{
		Local<FunctionTemplate> wscTemplate;
		wscTemplate = FunctionTemplate::New( isolate, wscObject::New );
		wscTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.core.ws.client" ) );
		wscTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( wscTemplate, "close", wscObject::close );
		NODE_SET_PROTOTYPE_METHOD( wscTemplate, "send", wscObject::write );
		NODE_SET_PROTOTYPE_METHOD( wscTemplate, "on", wscObject::on );
		NODE_SET_PROTOTYPE_METHOD( wscTemplate, "ping", wscObject::ping );

		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "aggregate" )
			, Local<FunctionTemplate>()
			, FunctionTemplate::New( isolate, wscObject::aggregate )
		);

		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "readyState" )
			, FunctionTemplate::New( isolate, wscObject::getReadyState )
			, Local<FunctionTemplate>() );
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "onopen" )
			, FunctionTemplate::New( isolate, wscObject::getOnOpen )
			, FunctionTemplate::New( isolate, wscObject::onOpen )
		);
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "onmessage" )
			, FunctionTemplate::New( isolate, wscObject::getOnMessage )
			, FunctionTemplate::New( isolate, wscObject::onMessage )
		);
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "onclose" )
			, FunctionTemplate::New( isolate, wscObject::getOnClose )
			, FunctionTemplate::New( isolate, wscObject::onClose )
		);
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "noDelay" )
			, Local<FunctionTemplate>()
			, FunctionTemplate::New( isolate, wscObject::nodelay )
		);
		wscTemplate->ReadOnlyPrototype();

		c->wscConstructor.Reset( isolate, wscTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
		c->wscTpl.Reset( isolate, wscTemplate );

		SET_READONLY( o, "Client", wscTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	}

	{
		Local<FunctionTemplate> wssiTemplate;
		wssiTemplate = FunctionTemplate::New( isolate, wssiObject::New );
		wssiTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.core.ws.connection" ) );
		wssiTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "post", postClientSocketObject );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "block", blockClientSocketAccept );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "resume", resumeClientSocketAccept );

		wssiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "aggregate" )
			, Local<FunctionTemplate>()
			, FunctionTemplate::New( isolate, wssiObject::aggregate )
		);

		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "send", wssiObject::write );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "close", wssiObject::close );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "on", wssiObject::on );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "ping", wssiObject::ping );
		wssiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "onmessage" )
			, Local<FunctionTemplate>()
			, FunctionTemplate::New( isolate, wssiObject::onmessage )
			);
		//NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "onmessage", wssiObject::onmessage );
		wssiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "onclose" )
			, Local<FunctionTemplate>()
			, FunctionTemplate::New( isolate, wssiObject::onclose )
			);
		//NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "onclose", wssiObject::onclose );
		wssiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "readyState" )
			, FunctionTemplate::New( isolate, wssiObject::getReadyState )
			, Local<FunctionTemplate>() );
		wssiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "noDelay" )
			, Local<FunctionTemplate>()
			, FunctionTemplate::New( isolate, wssiObject::nodelay )
			);
		wssiTemplate->ReadOnlyPrototype();

		c->wssiConstructor.Reset( isolate, wssiTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
		c->wssiTpl.Reset( isolate, wssiTemplate );
		//SET_READONLY( o, "Client", wscTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	}
}

static void Wait( void ) {
	IdleFor( 250 );
	return;
	uint32_t tick = GetTickCount();
	WakeableSleep( 250 );
	if( (GetTickCount() - tick) > 200 ) {
		tick = GetTickCount() - tick;
		lprintf( "slept %d", tick );
	}
}
#define Wait() IdleFor( 100 )


static uintptr_t webSockServerOpen( PCLIENT pc, uintptr_t psv ) {
	wssObject*wss = (wssObject*)psv;
	while( !wss->eventQueue )
		Relinquish();
	INDEX idx;
	struct html5_web_socket *ws = ( !wss->pc )?(struct html5_web_socket*)pc:NULL;
	if( ws ) pc = NULL;
	wssiObject *wssi;
	LIST_FORALL( wss->opening, idx, wssiObject*, wssi ) {
		if( wssi->pc == pc && wssi->wsPipe == ws ) {
			SetLink( &wss->opening, idx, NULL );
			break;
		}
	}
	if( wssi && !wssi->thrown ) {
		struct wssiEvent *pevt = GetWssiEvent();
		if( wssi->protocolResponse ) {
			Deallocate( const char *, wssi->protocolResponse );
			wssi->protocolResponse = NULL;
		}
		(*pevt).eventType = WS_EVENT_OPEN;
		(*pevt)._this = wssi;
		EnqueLink( &wssi->eventQueue, pevt );
		//lprintf( "Send Event:%p", &wss->async );
		wssi->readyState = wsReadyStates::OPEN;
#ifdef DEBUG_EVENTS
		lprintf( "Open event send...%p", &wssi->async );
#endif
		if( MakeThread() == l.jsThread ) {
			wssiAsyncMsg( &wssi->async );
		}
		else
			uv_async_send( &wssi->async );
		// can't change this result later, so need to send 
		// it as a refernence, in case the JS object changes
		return (uintptr_t)wssi->wssiRef;
	}
	if( !wssi )
		lprintf( "FAILED TO HAVE WSSI to open." );
	wssi->readyState = wsReadyStates::OPEN;
	return (uintptr_t)wssi->wssiRef;
}



static void webSockServerCloseEvent( wssObject *wss ) {
	struct wssEvent *pevt = GetWssEvent();
	//lprintf( "Server Websocket closed; post to javascript %p", wss );
	(*pevt).eventType = WS_EVENT_CLOSE;
	(*pevt)._this = wss;
	wss->closing = 1;
	EnqueLink( &wss->eventQueue, pevt );
	if( (*pevt).waiter == l.jsThread ) {
		wssAsyncMsg( &wss->async );
	}
	else {
#ifdef DEBUG_EVENTS
		lprintf( "socket close send %p", &wss->async);
#endif
		uv_async_send( &wss->async );
	}
	/*
	while( wss->event_waker ) {
		WakeThread( wss->event_waker );
		Relinquish();
	}
	*/
}

static void webSockServerClosed( PCLIENT pc, uintptr_t psv, int code, const char *reason )
{
	class wssiObjectReference *wssiRef = (class wssiObjectReference*)psv;
	class wssiObject *wssi = wssiRef->wssi;
	if( wssi ) {
		struct wssiEvent *pevt = GetWssiEvent();
		//lprintf( "Server Websocket closed; post to javascript %p  %p", pc, wssi );
		(*pevt).eventType = WS_EVENT_CLOSE;
		(*pevt)._this = wssi;
		(*pevt).code = code;
		(*pevt).buf = StrDup(reason);
		(*pevt).buflen = StrLen( reason );
		wssi->pc = NULL;
		EnqueLink( &wssi->eventQueue, pevt );
#ifdef DEBUG_EVENTS
		lprintf( "socket server close send %p", &wssi->async);
#endif		
		uv_async_send( &wssi->async );
	}
	else {
		uintptr_t psvServer = WebSocketGetServerData( pc );
		wssObject *wss = (wssObject*)psvServer;
		if( wss ) {	
			httpObject *req;
			INDEX idx;
			//int tot = 0;
			LOGICAL requested = FALSE;
			// close on wssObjectEvent; may have served HTTP requests
			//lprintf( "requests %p is %d", wss->requests, wss->requests?wss->requests->Cnt:0 );
			LIST_FORALL( wss->requests, idx, httpObject *, req ) {
				//tot++;
				if( req->pc == pc ) {
					//lprintf( "Removing request from wss %d %d", idx, tot );
					SetLink( &wss->requests, idx, NULL );
					requested = TRUE;
					//tot--;
					//return;
				}
			}
			if( requested ) 
				return;
		}
		//DumpAddr( "IP", ip );
		struct wssEvent *pevt = GetWssEvent();
		if( ( (*pevt).waiter = MakeThread() ) != l.jsThread )  {
			//lprintf( "Server Websocket closed; post to javascript %p", wss );
			(*pevt).eventType = WS_EVENT_ERROR_CLOSE;
			(*pevt).waiter = MakeThread();
			if( (*pevt).done ) lprintf( "FAIL!" );
			(*pevt).pc = pc;
			(*pevt)._this = wss;
			EnqueLink( &wss->eventQueue, pevt );
			if( (*pevt).waiter == l.jsThread ) {
				wssAsyncMsg( &wss->async );
			}
			else {
#ifdef DEBUG_EVENTS
				lprintf( "socket server close send2 %p", &wss->async);
#endif
				uv_async_send( &wss->async );
			}

			while( !(*pevt).done )
				Wait();
		} else {
			Isolate *isolate = Isolate::GetCurrent();
			Local<Object> closingSock = makeSocket( isolate, pc, wss->wsPipe, wss, NULL, NULL );
			if( !wss->errorCloseCallback.IsEmpty() ) {
				Local<Function> cb = wss->errorCloseCallback.Get( isolate );
				Local<Value> argv[1];
				argv[0] = closingSock;
				cb->Call( isolate->GetCurrentContext(), wss->_this.Get( isolate ), 1, argv );
			}
		}
	}
}

static void webSockServerError_( PCLIENT pc, class wssiObjectReference* wssiRef, int error ){
	class wssiObject *wssi = wssiRef->wssi;
	struct wssiEvent *pevt = GetWssiEvent();
	(*pevt).eventType = WS_EVENT_ERROR;
	(*pevt)._this = wssi;
	EnqueLink( &wssi->eventQueue, pevt );
#ifdef DEBUG_EVENTS
	lprintf( "socket server error send %p", &wssi->async);
#endif	
	uv_async_send( &wssi->async );
}

static void webSockServerError( PCLIENT pc, uintptr_t psv, int error ) {
	return webSockServerError_( pc, (wssiObjectReference*)psv, error );
}


static void webSockServerEvent_( PCLIENT pc, class wssiObjectReference*wssiRef, LOGICAL binary, CPOINTER buffer, size_t msglen ){
	//lprintf( "Received:%p %d", buffer, binary );
	//class wssiObjectReference *wssiRef = (class wssiObjectReference*)psv;
	class wssiObject *wssi = wssiRef->wssi;
	struct wssiEvent *pevt = GetWssiEvent();
	(*pevt).eventType = WS_EVENT_READ;
	(*pevt)._this = wssi;
	(*pevt).binary = binary;
	(*pevt).buf = NewArray( uint8_t, msglen );
	memcpy( (char*)(*pevt).buf, buffer, msglen );
	(*pevt).buflen = msglen;
	EnqueLink( &wssi->eventQueue, pevt );
#ifdef DEBUG_EVENTS
	lprintf( "socket data evnet send %p", &wssi->async );
#endif	
	uv_async_send( &wssi->async );

}

static void webSockServerEvent( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, size_t msglen ) {
	return webSockServerEvent_( pc, (wssiObjectReference*)psv, binary, buffer, msglen );
}

static void webSockServerAcceptAsync( PCLIENT pc, uintptr_t psv, const char* protocols, const char* resource )
{
	wssObject* wss = (wssObject*)psv;
	const struct html5_web_socket* ws = ( !wss->pc ) ? (struct html5_web_socket*)pc : NULL;
	//channel->
	struct wssEvent* pevt = GetWssEvent();
	//struct wssEvent evt;
	( *pevt ).data.request.protocol = protocols;
	( *pevt ).data.request.resource = resource;
	if( !pc && !ws ) lprintf( "FATALITY - ACCEPT EVENT RECEVIED ON A NON SOCKET!?" );

	( *pevt ).pc = pc;
	( *pevt ).data.request.accepted = 0;
	( *pevt ).data.request.rejected = 0;
	//lprintf( "Websocket accepted... (blocks until handled.)" );
	( *pevt ).eventType = WS_EVENT_ACCEPT;
	( *pevt ).done = FALSE;
	( *pevt ).waiter = MakeThread();
	( *pevt ).channel = NULL;
	( *pevt )._this = wss;
	//HoldWssEvent( pevt );

	EnqueLink( &wss->eventQueue, pevt );
	if( ( *pevt ).waiter == l.jsThread ) {
		wssAsyncMsg( &wss->async );
	} else {
#ifdef DEBUG_EVENTS
		lprintf( "socket server accept %p", &wss->async );
#endif		
		uv_async_send( &wss->async );
	}

	//while( !( *pevt ).done )
	//	Wait();
	//if( ( *pevt ).data.request.protocol != protocols )
	//	( *pevt ).result->protocolResponse = ( *pevt ).data.request.protocol;
	//( *protocolReply ) = (char*)( *pevt ).data.request.protocol;
	{
		//LOGICAL result = (LOGICAL)( *pevt ).data.request.accepted;
		//DropWssEvent( pevt );
		return;// result;
	}

}



static LOGICAL webSockServerAccept( PCLIENT pc, uintptr_t psv, const char* protocols, const char* resource, char** protocolReply ) {

	wssObject* wss = (wssObject*)psv;
	const struct html5_web_socket* ws = ( !wss->pc ) ? (struct html5_web_socket*)pc : NULL;
	//channel->
	struct wssEvent* pevt = GetWssEvent();
	//struct wssEvent evt;
	( *pevt ).data.request.protocol = protocols;
	( *pevt ).data.request.resource = resource;
	if( !pc && !ws ) lprintf( "FATALITY - ACCEPT EVENT RECEVIED ON A NON SOCKET!?" );

	( *pevt ).pc = pc;
	( *pevt ).data.request.accepted = 0;
	//lprintf( "Websocket accepted... (blocks until handled.)" );
	( *pevt ).eventType = WS_EVENT_ACCEPT;
	( *pevt ).done = FALSE;
	( *pevt ).waiter = MakeThread();
	( *pevt ).channel = NULL;
	( *pevt )._this = wss;
	//HoldWssEvent( pevt );

	EnqueLink( &wss->eventQueue, pevt );
	if( ( *pevt ).waiter == l.jsThread ) {
		wssAsyncMsg( &wss->async );
	} else {
#ifdef DEBUG_EVENTS
		lprintf( "socket server accept %p", &wss->async );
#endif		
		uv_async_send( &wss->async );
	}

	while( !( *pevt ).done )
		Wait();
	if( ( *pevt ).data.request.protocol != protocols )
		( *pevt ).result->protocolResponse = ( *pevt ).data.request.protocol;
	( *protocolReply ) = (char*)( *pevt ).data.request.protocol;
	{
		LOGICAL result = (LOGICAL)( *pevt ).data.request.accepted;
		DropWssEvent( pevt );
		return result;
	}

}



httpObject::httpObject() {
	pvtResult = VarTextCreate();
	ssl = 0;
	pc = NULL;
}

httpObject::~httpObject() {
	INDEX idx = FindLink( &this->wss->requests, this );
	if( idx != INVALID_INDEX ) {
		SetLink( &wss->requests, idx, NULL );
	}
	VarTextDestroy( &pvtResult );
}

void httpObject::New( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();

	Local<Object> _this = args.This();
	httpObject* obj = new httpObject( );
	//obj->_this.Reset( isolate, _this );
	obj->Wrap( _this );
	args.GetReturnValue().Set( _this );
}

void httpObject::writeHead( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	httpObject* obj = Unwrap<httpObject>( args.This() );
	PTEXT tmp;
	tmp = VarTextPeek( obj->pvtResult );
	if( tmp && GetTextSize( tmp ) ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Headers have already been set; cannot change resulting status or headers" ) ) );
		return;
	}
	int status = 404;
	Local<Object> headers;
	if( args.Length() > 0 ) {
		status = args[0]->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
	}
	HTTPState http = obj->pc?GetWebSocketHttpState( obj->pc ):obj->wss?GetWebSocketPipeHttpState( obj->wss->wsPipe ): NULL;
	if( http ) {
		int vers = GetHttpVersion( http );
		obj->headWritten = true;
		vtprintf( obj->pvtResult, "HTTP/%d.%d %d %s\r\n", vers / 100, vers % 100, status, "OK" );

		if( args.Length() > 1 ) {
			headers = args[1]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
			Local<Array> keys = headers->GetPropertyNames( isolate->GetCurrentContext() ).ToLocalChecked();
			int len = keys->Length();
			int i;
			for( i = 0; i < len; i++ ) {
				Local<Value> key = GETN( keys, i );
				Local<Value> val = GETV( headers, key );
				String::Utf8Value keyname( USE_ISOLATE( isolate ) key );
				String::Utf8Value keyval( USE_ISOLATE( isolate ) val );
				vtprintf( obj->pvtResult, "%s:%s\r\n", *keyname, *keyval );
			}
		}
	}
	else {
		lprintf("Failed to find HTTP state to write to?? %p", obj->pc);
	}
}

void httpObject::end( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	bool doSend = true;
	httpObject* obj = Unwrap<httpObject>( args.This() );
	char* content = NULL;
	size_t contentLen = 0;
	LOGICAL found_content_length = FALSE;
	int include_close = 1;
	{
		PLIST headers = obj->pc?GetWebSocketHeaders( obj->pc ):obj->wss->wsPipe?GetWebSocketPipeHeaders( obj->wss->wsPipe ): NULL;
		INDEX idx;
		struct HttpField *header;
		LIST_FORALL( headers, idx, struct HttpField *, header ) {
			if( StrCaseCmp( GetText( header->name ), "content-length" ) == 0 ) {
				found_content_length = TRUE;
			}
			if( StrCmp( GetText( header->name ), "Connection" ) == 0 ) {
				if( StrCaseCmp( GetText( header->value ), "keep-alive" ) == 0 ) {
					include_close = 0;
				}
			}
		}
		if( !include_close )
			vtprintf( obj->pvtResult, "Connection: keep-alive\r\n" );
	}
	if( args.Length() > 0 && !args[0]->IsNull() ) {
		if( args[0]->IsString() ) {
			String::Utf8Value body( USE_ISOLATE( isolate ) args[0] );
			if( !found_content_length )
				vtprintf( obj->pvtResult, "content-length:%d\r\n", body.length() );
			vtprintf( obj->pvtResult, "\r\n" );
			vtprintf( obj->pvtResult, "%*.*s", body.length(), body.length(), *body );
		}
		else if( args[0]->IsUint8Array() ) {
			Local<Uint8Array> body = args[0].As<Uint8Array>();
			Local<ArrayBuffer> bodybuf = body->Buffer();
			if( !found_content_length )
				vtprintf( obj->pvtResult, "content-length:%d\r\n", body->ByteLength() );
			vtprintf( obj->pvtResult, "\r\n" );
#if ( NODE_MAJOR_VERSION >= 14 )
			content = (char*)bodybuf->GetBackingStore()->Data();
			contentLen = bodybuf->ByteLength();
			//VarTextAddData( obj->pvtResult, (CTEXTSTR)bodybuf->GetBackingStore()->Data(), bodybuf->ByteLength() );
#else
			content = (char*)bodybuf->GetContents().Data();
			contentLen = bodybuf->ByteLength();
			//VarTextAddData( obj->pvtResult, (CTEXTSTR)bodybuf->GetContents().Data(), bodybuf->ByteLength() );
#endif
			{
				struct pendingWrite* write = new struct pendingWrite();
				write->wss = obj->wss;
				write->buffer.Reset( isolate, bodybuf );
				write->data = content;
				AddLink( &l.pendingWrites, write );
			}
		}
		else if( args[0]->IsArrayBuffer() ) {
			Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
			if( !found_content_length )
				vtprintf( obj->pvtResult, "content-length:%d\r\n", ab->ByteLength() );
			vtprintf( obj->pvtResult, "\r\n" );
#if ( NODE_MAJOR_VERSION >= 14 )
			content = (char*)ab->GetBackingStore()->Data();
			contentLen = ab->ByteLength();
			//VarTextAddData( obj->pvtResult, (CTEXTSTR)ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
			content = (char*)ab->GetContents().Data();
			contentLen = ab->ByteLength();
			//VarTextAddData( obj->pvtResult, (CTEXTSTR)ab->GetContents().Data(), ab->ByteLength() );
#endif
			{
				struct pendingWrite* write = new struct pendingWrite();
				write->wss = obj->wss;
				write->buffer.Reset( isolate, ab );
				write->data = content;
				AddLink( &l.pendingWrites, write );
			}
		} else if( args[0]->IsObject() ) {
			class constructorSet *c = getConstructors( isolate );
			Local<FunctionTemplate> wrapper_tpl = c->fileTpl.Get( isolate );
			if( (wrapper_tpl->HasInstance( args[0] )) ) {
				//FileObject *file = FileObject::Unwrap<FileObject>( args[0]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() );
				lprintf( "Incomplete; streaming file content to socket...." );
				doSend = false;
			}
		} else if( args[0]->IsNumber() ){
			if( !obj->headWritten ) 
				writeHead( args );

			vtprintf( obj->pvtResult, "\r\n" );
		} else {
			lprintf( "Unhandled argument type passed to http response.end(); just ending head" );
			vtprintf( obj->pvtResult, "\r\n" );
		}
	}
	else
		vtprintf( obj->pvtResult, "\r\n" );

	if( doSend ) {
#if AGGREGATE_BEFORE_NETWORK
		if( content && contentLen )
			VarTextAddData( obj->pvtResult, content, contentLen );
		PTEXT buffer = VarTextPeek( obj->pvtResult );
		if( obj->ssl ) {
			ssl_Send( obj->pc, GetText( buffer ), GetTextSize( buffer ) );
		} else {
			if( obj->pc ) {
				SendTCP( obj->pc, GetText( buffer ), GetTextSize( buffer ) );
			} else {
				WebSocketPipeSend( obj->wss->wsPipe, GetText( buffer ), GetTextSize( buffer ) );
			}
		}
#else
		PTEXT buffer = VarTextPeek( obj->pvtResult );
		if( obj->ssl ) {
			ssl_Send( obj->pc, GetText( buffer ), GetTextSize( buffer ) );
			if( content && contentLen ) {
				ssl_Send( obj->pc, content, contentLen );
			}
		} else {
			if( obj->pc ) {
#ifdef DEBUG_AGGREGATE_WRITES
				lprintf( "Sending header buffer: %p  %d", obj->pc, GetTextSize(buffer) );
#endif
				SendTCP( obj->pc, GetText( buffer ), GetTextSize( buffer ) );
				if( content && contentLen ) {
#ifdef DEBUG_AGGREGATE_WRITES
					lprintf( "And there's some content to send: %p %d", obj->pc, contentLen );
#endif
					// allow network layer to keep this content buffer
					SendTCPLong( obj->pc, content, contentLen );
				}
			} else {
#ifdef DEBUG_AGGREGATE_WRITES
				lprintf( "Send to pipe somehow??" );
#endif
				WebSocketPipeSend( obj->wss->wsPipe, GetText( buffer ), GetTextSize( buffer ) );
				if( content && contentLen )
					WebSocketPipeSend( obj->wss->wsPipe, content, contentLen );
			}
#ifdef DEBUG_AGGREGATE_WRITES
			lprintf( "Done with end function %p", obj->pc );
#endif
		}
#endif
	}
#ifdef DEBUG_AGGREGATE_WRITES
	else lprintf( "Decided not to send?" );
#endif
	if( obj->pc )
		ClearNetWork( obj->pc, (uintptr_t)obj->wss );
	{
		struct HttpState *pHttpState = obj->pc?GetWebSocketHttpState( obj->pc ):obj->wss?GetWebSocketPipeHttpState( obj->wss->wsPipe ):NULL;
		Hold( pHttpState );
		if( include_close ) {
			//lprintf( "Close is included... is this a reset close?" );
			if( obj->pc )
				RemoveClientEx( obj->pc, 0, 1 );
			else
				WebSocketPipeSocketClose( obj->wss->wsPipe );
		}
		//else {

		if (pHttpState) {
			int result;
			//lprintf( "ending http %p on %p, checking for more data", pHttpState, obj->pc );
			EndHttp(pHttpState);
			while ((result = ProcessHttp(pHttpState, NULL, 0 )))
			{
				//lprintf("result = %d  %zd", result, HTTP_STATE_RESULT_CONTENT == result);
				switch (result)
				{
				case HTTP_STATE_RESULT_CONTENT:

					struct wssEvent *pevt = GetWssEvent();
					//lprintf( "A posting request event to JS %p %s", obj->pc, GetText( GetHttpRequest( pHttpState ) ) );
					(*pevt).eventType = WS_EVENT_REQUEST;
					//(*pevt).waiter = MakeThread();
					(*pevt).pc = obj->pc;
					(*pevt)._this = obj->wss;
					obj->ssl = obj->pc?ssl_IsClientSecure( obj->pc ):0;
					EnqueLink(&obj->wss->eventQueue, pevt);
					//lprintf( "Send Request" );
					if( (*pevt).waiter == l.jsThread ) {
						wssAsyncMsg( &obj->wss->async );
					} else {
#ifdef DEBUG_EVENTS
						lprintf( "socket HTTP Parse Send %p", &obj->wss->async);
#endif						
						uv_async_send(&obj->wss->async);
					}
					break;
				}
			}
			Release( pHttpState );
		}
	}

	VarTextEmpty( obj->pvtResult );
}

static void webSockHttpClose( PCLIENT pc, uintptr_t psv ) {
	wssObject *wss = (wssObject*)psv;
	//uintptr_t psvServer = WebSocketGetServerData( pc );
	if( wss ) {
		httpObject *req;
		INDEX idx;
		//int tot = 0;
		LOGICAL requested = FALSE;
		// close on wssObjectEvent; may have served HTTP requests
		LIST_FORALL( wss->requests, idx, httpObject *, req ) {
			//tot++;
			if( req->pc == pc ) {
				//lprintf( "Removing request from wss %d %d", idx, tot );
				SetLink( &wss->requests, idx, NULL );
				requested = TRUE;
				//tot--;
				//return;
			}
		}
		if( requested )
			return;
	}

	//lprintf( "(close before accept)Illegal connection" );

	struct wssEvent *pevt = GetWssEvent();
	(*pevt).eventType = WS_EVENT_ERROR_CLOSE;
	(*pevt)._this = wss;
	(*pevt).pc = pc;
	(*pevt).waiter = MakeThread();
	EnqueLink( &wss->eventQueue, pevt );
	if( (*pevt).waiter == l.jsThread ) {
		wssAsyncMsg( &wss->async );
	}
	else {
#ifdef DEBUG_EVENTS
		lprintf( "socket HTTP close Send %p", &wss->async);
#endif		
		uv_async_send( &wss->async );
		while( (*pevt).done )
			Wait();
	}

}

static void webSocketWriteComplete( PCLIENT pc, CPOINTER buffer, size_t len ) {
	if( buffer ) {
		INDEX idx;
		struct pendingWrite* write;
		LIST_FORALL( l.pendingWrites, idx, struct pendingWrite*, write ) {
			if( write->data == buffer ) {
				// needs to be posted as an event
				struct wssEvent* pevt = GetWssEvent();
				//lprintf( "posting request event to JS  %s", GetText( GetHttpRequest( GetWebSocketHttpState( pc ) ) ) );
				SetWebSocketHttpCloseCallback( pc, webSockHttpClose );
				SetNetworkWriteComplete( pc, webSocketWriteComplete );
				pevt->eventType = WS_EVENT_RELEASE_BUFFER;
				pevt->data.write = write;
				EnqueLink( &write->wss->eventQueue, pevt );
#ifdef DEBUG_EVENTS
				lprintf( "socket write complete Send %p", &write->wss->async );
#endif
				uv_async_send( &write->wss->async );
				SetLink( &l.pendingWrites, idx, NULL );
				break;
			}	
		}
	}
}

static uintptr_t webSockHttpRequest( PCLIENT pc, uintptr_t psv ) {
	wssObject *wss = (wssObject*)psv;
	if( !wss->requestCallback.IsEmpty() ) {
		if( pc ) {
			AddNetWork( pc, psv );
			//lprintf( "posting request event to JS  %s", GetText( GetHttpRequest( GetWebSocketHttpState( pc ) ) ) );
			SetWebSocketHttpCloseCallback( pc, webSockHttpClose );
			SetNetworkWriteComplete( pc, webSocketWriteComplete );
		}

		struct wssEvent* pevt = GetWssEvent();
		(*pevt).eventType = WS_EVENT_REQUEST;
		//(*pevt).waiter = MakeThread();
		(*pevt). pc = pc;
		(*pevt)._this = wss;
		EnqueLink( &wss->eventQueue, pevt );
#ifdef DEBUG_EVENTS
		lprintf( "socket HTTP Request Send %p", &wss->async );
#endif		
		uv_async_send( &wss->async );
		//while (!(*pevt).done) WakeableSleep(SLEEP_FOREVER);
		//lprintf("queued and evented  request event to JS");
	} else {
		if( pc )
			RemoveClient( pc );
		else if( wss->wsPipe )
			WebSocketPipeSocketClose( wss->wsPipe );
		else
			lprintf( "FATALITY - HTTP REQUEST RECEVIED ON A NON SOCKET!?" );
	}
	return 0;
}

static void webSockServerLowError( uintptr_t psv, PCLIENT pc, enum SackNetworkErrorIdentifier error, ... ) {
	wssObject *wss = (wssObject*)psv;
	va_list args;
	struct wssEvent *pevt = GetWssEvent();
	va_start( args, error );
	(*pevt).eventType = WS_EVENT_LOW_ERROR;
	(*pevt).data.error.error = (int)error;
	switch( error ) {
	default:
		break;
	case SACK_NETWORK_ERROR_SSL_HANDSHAKE:
		(*pevt).data.error.buffer = va_arg( args, const char * );
		(*pevt).data.error.buflen = va_arg( args, size_t );
		(*pevt).data.error.fallback_ssl = 0; // make sure this is cleared.
		break;
	}
	(*pevt).pc = pc;
	(*pevt)._this = wss;
	(*pevt).waiter = MakeThread();
	HoldWssEvent( pevt );
	EnqueLink( &wss->eventQueue, pevt );
#ifdef DEBUG_EVENTS
	lprintf( "socket HTTP LowError Send %p", &wss->async);
#endif	
	uv_async_send( &wss->async );
	while( !(*pevt).done )
		WakeableSleep( 1000 );

	if( (*pevt).data.error.fallback_ssl )
		ssl_EndSecure( pc, (POINTER)(*pevt).data.error.buffer, (*pevt).data.error.buflen );

	DropWssEvent( pevt );
}

static uintptr_t catchLostEvents( PTHREAD thread ) {
	wssObject *wss = (wssObject*)GetThreadParam( thread );
	while( !wss->closing ) {
		if( wss->last_count_handled ) {
#ifdef DEBUG_EVENTS
			lprintf( "socket Lost Event Send %p", &wss->async );
#endif			
			uv_async_send( &wss->async );
		}
		WakeableSleep( 1000 );
	}
	wss->event_waker = NULL;
	return 0;
}

wssObject::wssObject( struct wssOptions *opts ) {
	char tmp[256];
	int clearUrl = 0;
	this->resolveMac = opts->getMAC;
	this->resolveAddr = opts->resolveName;
	last_count_handled = 0;
	closing = 0;
	readyState = INITIALIZING;
	eventQueue = NULL;
	requests = NULL;
	opening = FALSE;
	eventMessage = NULL;
	if( !opts->url ) {
		if( opts->address ) {
			if( strchr( opts->address, ':' ) )
				snprintf( tmp, 256, "ws%s://[%s]:%d/", opts->ssl?"s":"", opts->address, opts->port ? opts->port : 8080 );
			else
				snprintf( tmp, 256, "ws%s://%s:%d/", opts->ssl?"s":"", opts->address, opts->port ? opts->port : 8080 );
		}
		else
			snprintf( tmp, 256, "ws%s://[::]:%d/", opts->ssl?"s":"", opts->port ? opts->port : 8080 );
		//snprintf( tmp, 256, "ws://0.0.0.0:%d/", opts->port ? opts->port : 8080 );
		opts->url = tmp;
		clearUrl = 1;
	}
	closed = 0;
	if( opts->pipe ) {
		//lprintf( "++++ Creating a server pipe for %p", this );
		// for many of these functions, the psv of 'this' changes later... so it's ok to use this now (but it's not alawys this user value)
		wsPipe = WebSocketCreateServerPipe( webSockServerOpen // this gets this as psv
			, webSockServerEvent // this gets psvSender
			, webSockServerClosed // this gets psv returned from open
			, webSockServerError // this gets psv returned from open
			, NULL //webSockServerAccept // this gets psv returned from open
			, webSockHttpRequest  // this gets this as psv
			, webSockHttpClose
			, NULL, (uintptr_t)this );

		SetWebSocketAcceptAsyncCallback( pc, webSockServerAcceptAsync );
		
		pc = NULL;
		eventQueue = CreateLinkQueue();
		readyState = LISTENING;
	} else {
		NetworkWait( NULL, 256, 2 );  // 1GB memory
		pc = WebSocketCreate_v2( opts->url, webSockServerOpen, webSockServerEvent, webSockServerClosed
					, webSockServerError, (uintptr_t)this, WEBSOCK_SERVER_OPTION_WAIT );
		wsPipe = NULL;
	}
	if( pc ) {
		eventQueue = CreateLinkQueue();
		SetNetworkErrorCallback( pc, webSockServerLowError, (uintptr_t)this );
		SetSocketReusePort( pc, TRUE );
		SetSocketReuseAddress( pc, TRUE );
		//event_waker = ThreadTo( catchLostEvents, (uintptr_t)this );

		SetWebSocketHttpCallback( pc, webSockHttpRequest );

		if( opts->deflate ) {
			SetWebSocketDeflate( pc, WEBSOCK_DEFLATE_ENABLE );
			//lprintf( "deflate yes" );
		}
		if( opts->deflate_allow ) {
			SetWebSocketDeflate( pc, WEBSOCK_DEFLATE_ALLOW );
			//lprintf( "deflate allow, do not deflate" );
		}
		if( opts->apply_masking )
			SetWebSocketMasking( pc, 1 );
		if( opts->ssl ) {
			INDEX idx;
			struct wssHostOption* opt;
			if( opts->cert_chain ) {
				ssl_BeginServer( pc
					, opts->cert_chain, opts->cert_chain_len
					, opts->key, opts->key_len
					, opts->pass, opts->pass_len );
			}
			LIST_FORALL( opts->hostList, idx, struct wssHostOption*, opt ) {
				if( opt )
				ssl_BeginServer_v2( pc
					, opt->cert_chain, opt->cert_chain_len
					, opt->key, opt->key_len
					, opt->pass, opt->pass_len
					, opt->host
				);
			}
		}
		SetWebSocketAcceptAsyncCallback( pc, webSockServerAcceptAsync );
		readyState = LISTENING;
		SetNetworkListenerReady( pc );
	} else {

	}
	if( clearUrl )
		opts->url = NULL;
}

wssObject::~wssObject() {
	//lprintf( "Destruct wssObject" );
	if( pc )
		RemoveClient( pc );
	if( wsPipe )
		WebSocketPipeSocketClose( wsPipe );
	DeleteLinkQueue( &eventQueue );
}

static void ParseWssHostOption( struct optionStrings *strings
		, struct wssOptions* wssOpts
		, Isolate* isolate, Local<Object> hostOpt ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<String> optName;
	struct wssHostOption* newOpt = NewArray( struct wssHostOption, 1 );

	if( hostOpt->Has( context, optName = strings->hostString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value address( USE_ISOLATE( isolate ) GETV( hostOpt, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		newOpt->host = StrDup( *address );
		newOpt->hostlen = address.length();
	}

	if( hostOpt->Has( context, optName = strings->certString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value cert( USE_ISOLATE( isolate ) GETV( hostOpt, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		newOpt->cert_chain = StrDup( *cert );
		newOpt->cert_chain_len = cert.length();
	}

	if( hostOpt->Has( context, optName = strings->caString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value ca( USE_ISOLATE( isolate ) GETV( hostOpt, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		if( newOpt->cert_chain ) {
			newOpt->cert_chain = (char*)Reallocate( newOpt->cert_chain, newOpt->cert_chain_len + ca.length() + 1 );
			strcpy( newOpt->cert_chain + newOpt->cert_chain_len, *ca );
			newOpt->cert_chain_len += ca.length();
		}
		else {
			newOpt->cert_chain = StrDup( *ca );
			newOpt->cert_chain_len = ca.length();
		}
	}

	if( !hostOpt->Has( context, optName = strings->keyString->Get( isolate ) ).ToChecked() ) {
		newOpt->key = NULL;
		newOpt->key_len = 0;
	}
	else {
		String::Utf8Value cert( USE_ISOLATE( isolate ) GETV( hostOpt, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		newOpt->key = StrDup( *cert );
		newOpt->key_len = cert.length();
	}

	if( !hostOpt->Has( context, optName = strings->passString->Get( isolate ) ).ToChecked() ) {
		newOpt->pass = NULL;
		newOpt->pass_len = 0;
	}
	else {
		String::Utf8Value cert( USE_ISOLATE( isolate ) GETV( hostOpt, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		newOpt->pass = StrDup( *cert );
		newOpt->pass_len = cert.length();
	}

	AddLink( &wssOpts->hostList, newOpt );

}

static void ParseWssOptions( struct wssOptions *wssOpts, Isolate *isolate, Local<Object> opts ) {
	Local<Context> context = isolate->GetCurrentContext();

	Local<String> optName;
	struct optionStrings *strings = getStrings( isolate );
	MemSet( wssOpts, 0, sizeof( *wssOpts ) );
	wssOpts->cert_chain = NULL;
	wssOpts->cert_chain_len = 0;

	if( opts->Has( context, optName = strings->addressString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value address( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		wssOpts->address = StrDup( *address );
	}

	if( !opts->Has( context, optName = strings->portString->Get( isolate ) ).ToChecked() ) {
		wssOpts->port = 8080;
	}
	else {
		wssOpts->port = (int)GETV( opts, optName )->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
	}
	if( opts->Has( context, optName = strings->certString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value cert( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		wssOpts->cert_chain = StrDup( *cert );
		wssOpts->cert_chain_len = cert.length();
	}
	if( opts->Has( context, optName = strings->caString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value ca( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		if( wssOpts->cert_chain ) {
			wssOpts->cert_chain = (char*)Reallocate( wssOpts->cert_chain, wssOpts->cert_chain_len + ca.length() + 1 );
			strcpy( wssOpts->cert_chain + wssOpts->cert_chain_len, *ca );
			wssOpts->cert_chain_len += ca.length();
		} else {
			wssOpts->cert_chain = StrDup( *ca );
			wssOpts->cert_chain_len = ca.length();
		}
	}

	if( !opts->Has( context, optName = strings->keyString->Get( isolate ) ).ToChecked() ) {
		wssOpts->key = NULL;
		wssOpts->key_len = 0;
	}
	else {
		String::Utf8Value cert( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		wssOpts->key = StrDup( *cert );
		wssOpts->key_len = cert.length();
	}
	if( !opts->Has( context, optName = strings->passString->Get( isolate ) ).ToChecked() ) {
		wssOpts->pass = NULL;
		wssOpts->pass_len = 0;
	}
	else {
		String::Utf8Value cert( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		wssOpts->pass = StrDup( *cert );
		wssOpts->pass_len = cert.length();
	}

	if( wssOpts->key || wssOpts->cert_chain ) {
		wssOpts->ssl = 1;
	}
	else
		wssOpts->ssl = 0;

	if( !opts->Has( context, optName = strings->resolveString->Get( isolate ) ).ToChecked() ) {
		wssOpts->resolveName = false;
	}
	else
		wssOpts->resolveName = (GETV( opts, optName )->TOBOOL( isolate ));

	if( !opts->Has( context, optName = strings->getMacString->Get( isolate ) ).ToChecked() ) {
		wssOpts->getMAC = false;
	} else
		wssOpts->getMAC = ( GETV( opts, optName )->TOBOOL( isolate ) );

	if( !opts->Has( context, optName = strings->deflateString->Get( isolate ) ).ToChecked() ) {
		wssOpts->deflate = false;
	} else
		wssOpts->deflate = ( GETV( opts, optName )->TOBOOL( isolate ) );

	if( !opts->Has( context, optName = strings->pipeString->Get( isolate ) ).ToChecked() ) {
		wssOpts->pipe = false;
	} else
		wssOpts->pipe = ( GETV( opts, optName )->TOBOOL( isolate ) );

	if( !opts->Has( context, optName = strings->deflateAllowString->Get( isolate ) ).ToChecked() ) {
		wssOpts->deflate_allow = false;
	}
	else {
		wssOpts->deflate_allow = (GETV( opts, optName )->TOBOOL( isolate ));
		//lprintf( "deflate allow:%d", wssOpts->deflate_allow );
	}

	if( opts->Has( context, optName = strings->maskingString->Get( isolate ) ).ToChecked() ) {
		wssOpts->apply_masking = GETV( opts, optName )->TOBOOL( isolate );
	}
	else
		wssOpts->apply_masking = false;

	if( opts->Has( context, optName = strings->hostsString->Get( isolate ) ).ToChecked() ) {
		Local<Value> val = GETV( opts, optName );
		if( val->IsArray() ) {
		Local<Array> hosts = GETV( opts, optName ).As<Array>();
		uint32_t o;
		for( o = 0; o < hosts->Length(); o++ ) {
			Local<Object> host = GETV( hosts, o ).As<Object>();
			ParseWssHostOption( strings, wssOpts, isolate, host );
		}
		}
	}


}

void wssObject::New(const FunctionCallbackInfo<Value>& args){
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Must specify options for server." ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		struct wssOptions wssOpts;
		MemSet( &wssOpts, 0, sizeof( wssOpts ) );

		int argOfs = 0;
		if( args[argOfs]->IsString() ) {
			String::Utf8Value url( USE_ISOLATE( isolate ) args[argOfs]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
			wssOpts.url = StrDup( *url );
			argOfs++;
		}
		if( args[argOfs]->IsNumber() ) {
			wssOpts.port = (int)args[0]->IntegerValue(isolate->GetCurrentContext()).FromMaybe(0);
			argOfs++;
		}
		if( args[argOfs]->IsObject() ) {
			Local<Object> opts = args[0]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
			ParseWssOptions( &wssOpts, isolate, opts );
		}

		Local<Object> _this = args.This();
		wssObject* obj = new wssObject( &wssOpts );
		class constructorSet *c = getConstructors(isolate);
		uv_async_init( c->loop, &obj->async, wssAsyncMsg );
		obj->async.data = obj;

		obj->isolate = isolate;
		obj->ssl = wssOpts.ssl;
		if( wssOpts.cert_chain )
			Deallocate( char *, wssOpts.cert_chain );
		if( wssOpts.key )
			Deallocate( char *, wssOpts.key );
		if( wssOpts.pass )
			Deallocate( char *, wssOpts.pass );
		if( wssOpts.address )
			Deallocate( char *, wssOpts.address );
		if( wssOpts.url )
			Deallocate( char *, wssOpts.url );
		if( !obj->pc && !obj->wsPipe ) {
			delete obj;
			isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Failed to create listener." ) ) );
			return;
		}
		if( args.Length() > 1 && args[1]->IsFunction() ) {
			Local<Function> arg0 = Local<Function>::Cast( args[1] );
			obj->openCallback.Reset( isolate, arg0 );
		}
		obj->_this.Reset( isolate, _this );
		obj->Wrap( _this );
		args.GetReturnValue().Set( _this );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];
		class constructorSet *c = getConstructors( isolate );

		Local<Function> cons = Local<Function>::New( isolate, c->wssConstructor );
		MaybeLocal<Object> result = cons->NewInstance( isolate->GetCurrentContext(), argc, argv );
		delete[] argv;
		if( !result.IsEmpty() )
			args.GetReturnValue().Set( result.ToLocalChecked() );
	}
}

void wssObject::disableSSL( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
	if( obj->eventMessage && obj->eventMessage->eventType == WS_EVENT_LOW_ERROR ) {
		// delay until we return to the thread that dispatched this error.
		obj->eventMessage->data.error.fallback_ssl = 1;
		//ssl_EndSecure( obj->eventMessage->pc, (POINTER)obj->eventMessage->data.error.buffer, obj->eventMessage->data.error.buflen );
	}
}

void wssObject::close( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
	obj->readyState = CLOSING;
	lprintf( "remove client." );
	if( obj->pc )
		RemoveClient( obj->pc );
	if( obj->wsPipe ) {
		WebSocketPipeSocketClose( obj->wsPipe );
	}
	webSockServerCloseEvent( obj );
}

void wssObject::on( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() == 2 ) {
		Isolate* isolate = args.GetIsolate();
		String::Utf8Value event( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		Local<Function> cb = Local<Function>::Cast( args[1] );
		if( !cb->IsFunction() ) {
			isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Argument is not a function" ) ) );
			return;
		}
		if( StrCmp( *event, "request" ) == 0 ) {
			obj->requestCallback.Reset( isolate, cb );
		}
		else if( StrCmp( *event, "connect" ) == 0 ) {
			obj->openCallback.Reset( isolate, cb );
		}
		else if( StrCmp( *event, "accept" ) == 0 ) {
			obj->acceptCallback.Reset( isolate, cb );
		}
		else if( StrCmp( *event, "close" ) == 0 ) {
			obj->closeCallback.Reset( isolate, cb );
		}
		else if( StrCmp( *event, "error" ) == 0 ) {
			obj->errorCloseCallback.Reset( isolate, cb );
		}
		else if( StrCmp( *event, "lowError" ) == 0 ) {
			obj->errorLowCallback.Reset( isolate, cb );
		}
	}
}

void wssObject::onConnect( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->openCallback.Reset( isolate, Local<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Argument is not a function" ) ) );
	}
}

void wssObject::onAccept( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->acceptCallback.Reset( isolate, Local<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Argument is not a function" ) ) );
	}
}

void wssObject::onRequest( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->requestCallback.Reset( isolate, Local<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Argument is not a function" ) ) );
	}
}

void wssObject::onError( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->errorCloseCallback.Reset( isolate, Local<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Argument is not a function" ) ) );
	}
}

void wssObject::onErrorLow( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->errorLowCallback.Reset( isolate, Local<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Argument is not a function" ) ) );
	}
}

void wssObject::getOnClose( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
	args.GetReturnValue().Set( obj->closeCallback.Get( isolate ) );
}
void wssObject::onClose( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		obj->closeCallback.Reset( isolate, cb );
	}
}

void wssObject::accept( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( !obj->eventMessage ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Reject cannot be used outside of connection callback." ) ) );
		return;
	}
	if( args.Length() > 0 ) {
		String::Utf8Value protocol( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		obj->eventMessage->data.request.protocol = StrDup( *protocol );
	}
	obj->eventMessage->data.request.accepted = 1;
	WebSocketAccept( obj->eventMessage->pc, (char*)obj->eventMessage->data.request.protocol, 1 );
}

void wssObject::reject( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( !obj->eventMessage ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Reject cannot be used outside of connection callback." ) ) );
		return;
	}
	obj->eventMessage->data.request.rejected = 1;
	WebSocketAccept( obj->eventMessage->pc, NULL, 0 );
}

void wssObject::getReadyState( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
	args.GetReturnValue().Set( Integer::New( isolate, (int)obj->readyState ) );
#if 0
	Local<Object> h = args.Holder();
	Local<Object> t = args.This();
	//if( wssObject::tpl. )
	class constructorSet *c = getConstructors( isolate );
	Local<FunctionTemplate> wrapper_tpl = c->wssTpl.Get( isolate );
	int ht = wrapper_tpl->HasInstance( h );
	int tt = wrapper_tpl->HasInstance( t );
	wssObject *obj;
	if( ht )
		obj = ObjectWrap::Unwrap<wssObject>( h );
	else if( tt )
		obj = ObjectWrap::Unwrap<wssObject>( t );
	else
		obj = NULL;
	if( obj )
		args.GetReturnValue().Set( Integer::New( args.GetIsolate(), (int)obj->readyState ) );
#endif
}


wssiObject::wssiObject( ) {
	eventQueue = CreateLinkQueue();
	readyState = wsReadyStates::CONNECTING;
	protocolResponse = NULL;
	this->closed = 0;
	this->thrown = FALSE;
	this->blockReturn = 0;
	this->acceptEventMessage = NULL;
}

wssiObject::~wssiObject() {
	if( !closed ) {
		lprintf( "destruct, try to generate WebSockClose" );
		RemoveClient( pc );
		if( wsPipe ) {
			WebSocketPipeSocketClose( wsPipe );
		}
	}
	//all member would have been reset... in the event close
	DeleteList( &closeCallbacks );
	DeleteList( &messageCallbacks );
	DeleteList( &errorCallbacks );
}

void wssiObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.IsConstructCall() ) {
		wssiObject *obj = new wssiObject();
		obj->isolate = isolate;
		//class constructorSet *c = getConstructors(isolate);
		//uv_async_init( c->loop, &obj->async, wssiAsyncMsg );
		obj->wssiRef = new wssiObjectReference();
		obj->wssiRef->wssi = obj;
		obj->async.data = obj;
		obj->_this.Reset( isolate, args.This() );
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Value> *argv = new Local<Value>[args.Length()];
		class constructorSet *c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->wssiConstructor );
		Local<Object> wssi = cons->NewInstance( isolate->GetCurrentContext(), 0, argv ).ToLocalChecked();
		SET_READONLY( wssi, "OPEN", Integer::New( isolate, wsReadyStates::OPEN ) );
		SET_READONLY( wssi, "CLOSED", Integer::New( isolate, wsReadyStates::CLOSED ) );
		SET_READONLY( wssi, "CLOSING", Integer::New( isolate, wsReadyStates::CLOSING ) );
		SET_READONLY( wssi, "CONNECTING", Integer::New( isolate, wsReadyStates::CONNECTING ) );
		SET_READONLY( wssi, "INITIALIZING", Integer::New( isolate, wsReadyStates::INITIALIZING ) );
		args.GetReturnValue().Set( wssi );
		delete[] argv;
	}
}

void wssiObject::ping( const v8::FunctionCallbackInfo<Value>& args ) {
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
	if (obj->pc) {
		WebSocketPing(obj->pc, 0);
	}
}

void wssiObject::on( const FunctionCallbackInfo<Value>& args){
	Isolate* isolate = args.GetIsolate();

	if( args.Length() == 2 ) {
		wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
		String::Utf8Value event( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		Local<Function> cb = Local<Function>::Cast( args[1] );
		if(  StrCmp( *event, "message" ) == 0 ) {
			callbackFunction* c = new callbackFunction();
			c->callback.Reset( isolate, cb );
			AddLink( &obj->messageCallbacks, c );
		} else if(  StrCmp( *event, "error" ) == 0 ) {
			callbackFunction* c = new callbackFunction();
			c->callback.Reset( isolate, cb );
			AddLink( &obj->errorCallbacks, c );
		} else if(  StrCmp( *event, "close" ) == 0 ){
			callbackFunction* c = new callbackFunction();
			c->callback.Reset( isolate, cb );
			AddLink( &obj->closeCallbacks, c );
		}
	}
}

void wssiObject::onmessage( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		callbackFunction* c = new callbackFunction();
		c->callback.Reset( isolate, cb );
		AddLink( &obj->messageCallbacks, c );
	}
}

void wssiObject::onclose( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		callbackFunction* c = new callbackFunction();
		c->callback.Reset( isolate, cb );
		AddLink( &obj->closeCallbacks, c );
	}
}

void wssiObject::nodelay( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
		
		SetTCPNoDelay( obj->pc, args[0]->BooleanValue(isolate) );
	}
}

void wssiObject::aggregate( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wssiObject* obj = ObjectWrap::Unwrap<wssiObject>( args.This() );

		SetTCPWriteAggregation( obj->pc, args[0]->BooleanValue( isolate ) );
	}
}

void wssiObject::close( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
	if( args.Length() == 0 ) {
		if( obj->pc && !obj->closed )
			if( obj->pc )
				WebSocketClose( obj->pc, 1000, NULL );
			else if( obj->wsPipe )
				WebSocketPipeClose( obj->wsPipe, 1000, NULL );
	}
	if( args[0]->IsNumber() ) {
		int code = args[0]->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		{
			if( args.Length() > 1 ) {
				String::Utf8Value reason( USE_ISOLATE( isolate ) args[1]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
				if( reason.length() > 123 ) {
					isolate->ThrowException( Exception::Error(
						String::NewFromUtf8( isolate, TranslateText( "SyntaxError (text reason longer than 123)" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
					return;
				}
				if( obj->pc ) 
					WebSocketClose( obj->pc, code, *reason);
				else if( obj->wsPipe )
					WebSocketPipeClose( obj->wsPipe, code, *reason );
			
			}
			else {
				if( obj->pc ) 
					WebSocketClose( obj->pc, code, NULL );
				else if( obj->wsPipe )
					WebSocketPipeClose( obj->wsPipe, code, NULL );
			}
		}
	}
	else {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "InvalidAccessError Code is not a number." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}


}

void wssiObject::write( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject* obj = ObjectWrap::Unwrap<wssiObject>( args.This() );

	if( !obj->pc && !obj->server->wsPipe ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Connection has already been closed." ) ) );
		return;
	}
	while( obj->readyState == wsReadyStates::CONNECTING )
		Relinquish();

	if( args[0]->IsTypedArray() ) {
		Local<TypedArray> ta = Local<TypedArray>::Cast( args[0] );
		Local<ArrayBuffer> ab = ta->Buffer();
		if( obj->pc ) {
#if ( NODE_MAJOR_VERSION >= 14 )
			WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
			WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
#endif
		} else if( obj->wsPipe ) {
#if ( NODE_MAJOR_VERSION >= 14 )
			WebSocketPipeSendBinary( obj->wsPipe, (const uint8_t*)ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
			WebSocketPipeSendBinary( obj->wsPipe, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
#endif

		}
	} else if( args[0]->IsUint8Array() ) {
		Local<Uint8Array> body = args[0].As<Uint8Array>();
		Local<ArrayBuffer> ab = body->Buffer();
		if( obj->pc ) {
#if ( NODE_MAJOR_VERSION >= 14 )
			WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
			WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
#endif
		} else if( obj->wsPipe ) {
#if ( NODE_MAJOR_VERSION >= 14 )
			WebSocketPipeSendBinary( obj->wsPipe, (const uint8_t*)ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
			WebSocketPipeSendBinary( obj->wsPipe, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
#endif
		}
	} else if( args[0]->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
		if( obj->pc ) {
#if ( NODE_MAJOR_VERSION >= 14 )
			WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
			WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
#endif
		} else if( obj->wsPipe ) {
#if ( NODE_MAJOR_VERSION >= 14 )
			WebSocketPipeSendBinary( obj->wsPipe, (const uint8_t*)ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
			WebSocketPipeSendBinary( obj->wsPipe, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
#endif
		}
	} else if( args[0]->IsString() ) {
		String::Utf8Value buf( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		if( obj->pc ) {
			WebSocketSendText( obj->pc, *buf, buf.length() );
		} else if( obj->wsPipe ) {
			WebSocketPipeSendText( obj->wsPipe, *buf, buf.length() );
		}
	} else if( args[0]->IsObject() ) {
		lprintf( "Cannot send argument that is a type of object passed to websocket send." );
	}  else {
		lprintf( "Argument passed to send is not a String, ArrayBuffer or TypeAarray." );
	}
}

void wssiObject::getReadyState( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
	args.GetReturnValue().Set( Integer::New( isolate, (int)obj->readyState ) );
#if 0
	Local<Object> h = args.Holder();
	Local<Object> t = args.This();
	//if( wssObject::tpl. )
		class constructorSet *c = getConstructors( isolate );
	Local<FunctionTemplate> wrapper_tpl = c->wssiTpl.Get( isolate );
	int ht = wrapper_tpl->HasInstance( h );
	int tt = wrapper_tpl->HasInstance( t );
	wssiObject *obj;
	if( ht )
		obj = ObjectWrap::Unwrap<wssiObject>( h );
	else if( tt )
		obj = ObjectWrap::Unwrap<wssiObject>( t );
	else
		obj = NULL;
	if( obj )
		args.GetReturnValue().Set( Integer::New( args.GetIsolate(), (int)obj->readyState ) );
#endif
}


static uintptr_t webSockClientOpen( PCLIENT pc, uintptr_t psv ) {
	wscObject *wsc = (wscObject*)psv;
	wsc->readyState = OPEN;
	struct wscEvent *pevt = GetWscEvent();
	//lprintf( "send open event to JS(2)" );
	(*pevt).eventType = WS_EVENT_OPEN;
	(*pevt)._this = wsc;
	EnqueLink( &wsc->eventQueue, pevt );
#ifdef DEBUG_EVENTS
	lprintf( "Send Open Request %p", &wsc->async );
#endif
	uv_async_send( &wsc->async );
	return psv;
}

static void webSockClientClosed( PCLIENT pc, uintptr_t psv, int code, const char *reason )
{
	wscObject *wsc = (wscObject*)psv;
	wsc->readyState = CLOSED;
	struct wscEvent *pevt = GetWscEvent();
	(*pevt).eventType = WS_EVENT_CLOSE;
	(*pevt)._this = wsc;
	(*pevt).code = code;
	(*pevt).buf = StrDup(reason);
	(*pevt).buflen = StrLen( reason );
	EnqueLink( &wsc->eventQueue, pevt );
#ifdef DEBUG_EVENTS
	lprintf( "Send Close Request %p", &wsc->async );
#endif
	uv_async_send( &wsc->async );
	wsc->pc = NULL;
}

static void webSockClientError( PCLIENT pc, uintptr_t psv, int error ) {
	wscObject *wsc = (wscObject*)psv;

	struct wscEvent *pevt = GetWscEvent();
	(*pevt).eventType = WS_EVENT_ERROR;
	(*pevt)._this = wsc;
	(*pevt).code = error;
	EnqueLink( &wsc->eventQueue, pevt );
#ifdef DEBUG_EVENTS
	lprintf( "Send Error Request %p", &wsc->async );
#endif
	uv_async_send( &wsc->async );
}

static void webSockClientEvent( PCLIENT pc, uintptr_t psv, LOGICAL type, CPOINTER buffer, size_t msglen ) {
	wscObject *wsc = (wscObject*)psv;

	struct wscEvent *pevt = GetWscEvent();
	(*pevt).eventType = WS_EVENT_READ;
	(*pevt).buf = NewArray( uint8_t, msglen );
	memcpy( (char*)(*pevt).buf, buffer, msglen );
	(*pevt).buflen = msglen;
	(*pevt).binary = type;
	(*pevt)._this = wsc;
	EnqueLink( &wsc->eventQueue, pevt );
#ifdef DEBUG_EVENTS
	lprintf( "Send Client Read Request %p", &wsc->async );
#endif
	uv_async_send( &wsc->async );
}

wscObject::wscObject( wscOptions *opts ) {
	eventQueue = CreateLinkQueue();
	readyState = INITIALIZING;
	closed = 0;
	this->resolveMac = opts->getMAC;
	this->resolveAddr = opts->resolveName;
	//lprintf( "Init async handle. (wsc) %p", &async );
	NetworkWait( NULL, 256, 2 );  // 1GB memory

	pc = WebSocketOpen( opts->url, WS_DELAY_OPEN
		, webSockClientOpen
		, webSockClientEvent, webSockClientClosed, webSockClientError, (uintptr_t)this, opts->protocol );
	if( pc ) {
		SetClientKeepAlive( pc, opts->keep_alive);
		if( opts->deflate )
			SetWebSocketDeflate( pc, WEBSOCK_DEFLATE_ENABLE );
		if( !opts->apply_masking )
			SetWebSocketMasking( pc, 0 );
		if( opts->ssl ) {
			if( !ssl_BeginClientSession( pc, opts->key, opts->key_len, opts->pass, opts->pass_len
				, opts->root_cert, opts->root_cert ? strlen( opts->root_cert ) : 0 ) ) {
				isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Error initializing SSL connection (bad key or passphrase?)" ) ) );
				//throw "Error initializing SSL connection (bad key or passphrase?)";
			}
		}
		if( WebSocketConnect( pc ) < 0 ){
			pc = NULL;
		} else {
			readyState = CONNECTING;
		}
	} else {
		lprintf( "Socket returned Null?" );
	}
}

wscObject::~wscObject() {
	//lprintf( "Destruct wscObject" );
	if( !closed ) {
		//lprintf( "destruct, try to generate WebSockClose" );
		RemoveClient( pc );
	}
	//all member would have been reset... in the event close
	DeleteList( &openCallbacks );
	DeleteList( &closeCallbacks );
	DeleteList( &messageCallbacks );
	DeleteList( &errorCallbacks );
	DeleteLinkQueue( &eventQueue );
}


void parseWscOptions( struct wscOptions *wscOpts, Isolate *isolate, Local<Object> opts ) {
	struct optionStrings *strings = getStrings( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	Local<String> optName;

	if( opts->Has( context, optName = strings->caString->Get( isolate ) ).ToChecked() ) {
		wscOpts->ssl = 1;
		Local<Value> opt = GETV( opts, optName );
		if( opt->IsString() ){
			String::Utf8Value rootCa( USE_ISOLATE( isolate ) opt->ToString( context ).ToLocalChecked() );
			wscOpts->root_cert = StrDup( *rootCa );
		}
		else
			wscOpts->root_cert = NULL;
	}

	if( opts->Has( context, optName = strings->keyString->Get( isolate ) ).ToChecked() ) {
		wscOpts->ssl = 1;
		String::Utf8Value rootCa( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( context ).ToLocalChecked() );
		wscOpts->key = StrDup( *rootCa );
		wscOpts->key_len = rootCa.length();
	}

	if( opts->Has( context, optName = strings->passString->Get( isolate ) ).ToChecked() ) {
		wscOpts->ssl = 1;
		String::Utf8Value rootCa( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( context ).ToLocalChecked() );
		wscOpts->pass = StrDup( *rootCa );
				wscOpts->pass_len = rootCa.length();
	}

	if( !opts->Has( context, optName = strings->resolveString->Get( isolate ) ).ToChecked() ) {
		wscOpts->resolveName = false;
	} else
		wscOpts->resolveName = ( GETV( opts, optName )->TOBOOL( isolate ) );

	if( !opts->Has( context, optName = strings->getMacString->Get( isolate ) ).ToChecked() ) {
		wscOpts->getMAC = false;
	} else
		wscOpts->getMAC = ( GETV( opts, optName )->TOBOOL( isolate ) );

	if( opts->Has( context, optName = strings->deflateString->Get( isolate ) ).ToChecked() ) {
		wscOpts->deflate = GETV( opts, optName )->ToBoolean( isolate )->Value();
	}
	else
		wscOpts->deflate = false;
	if( opts->Has( context, optName = strings->deflateAllowString->Get( isolate ) ).ToChecked() ) {
		wscOpts->deflate_allow = GETV( opts, optName )->ToBoolean( isolate )->Value();
	}
	else
		wscOpts->deflate_allow = false;
	if( opts->Has( context, optName = strings->maskingString->Get( isolate ) ).ToChecked() ) {
		wscOpts->apply_masking = GETV( opts, optName )->ToBoolean( isolate )->Value();
	}
	else
		wscOpts->apply_masking = true;

	if( opts->Has( context, optName = strings->keepAliveString->Get( isolate ) ).ToChecked() ) {
		wscOpts->keep_alive = GETV( opts, optName )->ToBoolean( isolate )->Value();
	}
	else
		wscOpts->keep_alive = false;
}

void wscObject::New(const FunctionCallbackInfo<Value>& args){
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Must specify url and optionally protocols or options for client." ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		bool checkArg2;
		// Invoked as constructor: `new MyObject(...)`
		struct wscOptions wscOpts;
		String::Utf8Value url( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		String::Utf8Value *protocol = NULL;
		Local<Object> opts;
		opts.Clear();
		MemSet( &wscOpts, 0, sizeof( wscOpts ) );

		if( args[1]->IsString() ) {
			checkArg2 = true;
			protocol = new String::Utf8Value( USE_ISOLATE( isolate ) args[1]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		}
		else if( args[1]->IsArray() ) {
			checkArg2 = true;
			protocol = new String::Utf8Value( USE_ISOLATE( isolate ) args[1]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		}
		else if( args[1]->IsObject() ) {
			checkArg2 = false;
			parseWscOptions( &wscOpts, isolate, args[1]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() );
		}
		else
			checkArg2 = false;

		if( checkArg2 ) {
			if( args[2]->IsObject() ) {
				opts = args[2]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
				parseWscOptions( &wscOpts, isolate, opts );
			}
		}

		wscOpts.url = *url;
		if( protocol )
			wscOpts.protocol = *(protocol[0]);
		wscOpts.deflate = false;

		Local<Object> _this = args.This();
		wscObject* obj;
		//try {
			obj = new wscObject( &wscOpts );
			obj->isolate = isolate;
			class constructorSet *c = getConstructors(isolate);
			uv_async_init( c->loop, &obj->async, wscAsyncMsg );
			obj->async.data = obj;

			obj->_this.Reset( isolate, _this );
			obj->Wrap( _this );
		//}
		//catch( const char *ex1 ) {
		//	isolate->ThrowException( Exception::Error(
		//		String::NewFromUtf8( isolate, TranslateText( ex1 ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		//}
		if( wscOpts.root_cert )
			Deallocate( char *, wscOpts.root_cert );
		if( wscOpts.key )
			Deallocate( char *, wscOpts.key );
		if( wscOpts.pass )
			Deallocate( char *, wscOpts.pass );
		struct optionStrings *strings = getStrings( isolate );
		SETV( _this, strings->bufferedAmountString->Get(isolate), Number::New( isolate, 0 ) );
		args.GetReturnValue().Set( _this );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		class constructorSet *c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->wscConstructor );
		Local<Object> wsc = cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked();
		SET_READONLY( wsc, "OPEN", Integer::New( isolate, wsReadyStates::OPEN ) );
		SET_READONLY( wsc, "CLOSED", Integer::New( isolate, wsReadyStates::CLOSED ) );
		SET_READONLY( wsc, "CLOSING", Integer::New( isolate, wsReadyStates::CLOSING ) );
		SET_READONLY( wsc, "CONNECTING", Integer::New( isolate, wsReadyStates::CONNECTING ) );
		SET_READONLY( wsc, "INITIALIZING", Integer::New( isolate, wsReadyStates::INITIALIZING ) );

		args.GetReturnValue().Set( wsc );
		delete[] argv;
	}
}

void wscObject::onOpen( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		callbackFunction* c = new callbackFunction();
		c->callback.Reset( isolate, cb );
		AddLink( &obj->openCallbacks, c );
	}
}

void wscObject::onMessage( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );

		callbackFunction* c = new callbackFunction();
		c->callback.Reset( isolate, cb );
		AddLink( &obj->messageCallbacks, c );
	}
}

void wscObject::onClose( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		callbackFunction* c = new callbackFunction();
		c->callback.Reset( isolate, cb );
		AddLink( &obj->closeCallbacks, c );
	}
}

void wscObject::onError( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		callbackFunction* c = new callbackFunction();
		c->callback.Reset( isolate, cb );
		AddLink( &obj->errorCallbacks, c );
	}
}

void wscObject::nodelay( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject* obj = ObjectWrap::Unwrap<wscObject>( args.This() );

		SetTCPNoDelay( obj->pc, args[0]->BooleanValue( isolate ) );
	}
}

void wscObject::getOnOpen( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	//wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	//args.GetReturnValue().Set( obj->openCallback.Get( isolate ) );
}

void wscObject::getOnMessage( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	//wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	//args.GetReturnValue().Set( obj->messageCallback.Get( isolate ) );
}

void wscObject::getOnClose( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	//wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	//args.GetReturnValue().Set( obj->closeCallback.Get( isolate ) );
}

void wscObject::getOnError( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	//wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	//args.GetReturnValue().Set( obj->errorCallback.Get( isolate ) );
}

void wscObject::ping( const v8::FunctionCallbackInfo<Value>& args ) {
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	WebSocketPing( obj->pc, 0 );
}

void wscObject::on( const FunctionCallbackInfo<Value>& args){
	if( args.Length() == 2 ) {
		Isolate* isolate = args.GetIsolate();
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		String::Utf8Value event( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		Local<Function> cb = Local<Function>::Cast( args[1] );
		if( StrCmp( *event, "open" ) == 0 ){
			if( obj->readyState == OPEN ) {
				cb->Call( isolate->GetCurrentContext(), obj->_this.Get( isolate ), 0, NULL );
			}
			else {
				callbackFunction* c = new callbackFunction();
				c->callback.Reset( isolate, cb );
				AddLink( &obj->openCallbacks, c );
			}
		} else if(  StrCmp( *event, "message" ) == 0 ) {
			callbackFunction* c = new callbackFunction();
			c->callback.Reset( isolate, cb );
			AddLink( &obj->messageCallbacks, c );
		} else if(  StrCmp( *event, "error" ) == 0 ) {
			callbackFunction* c = new callbackFunction();
			c->callback.Reset( isolate, cb );
			AddLink( &obj->errorCallbacks, c );
		} else if(  StrCmp( *event, "close" ) == 0 ) {
			callbackFunction* c = new callbackFunction();
			c->callback.Reset( isolate, cb );
			AddLink( &obj->closeCallbacks, c );
		} else
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( "Event name specified is not supported or known." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
	}
}

void wscObject::aggregate( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject* obj = ObjectWrap::Unwrap<wscObject>( args.This() );

		SetTCPWriteAggregation( obj->pc, args[0]->BooleanValue( isolate ) );
	}
}

void wscObject::close( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	if( args.Length() == 0 ) {
		if( obj->pc ) {
			WebSocketClose( obj->pc, 1000, NULL );
		}
		return;
	}
	else {
		if( args[0]->IsNumber() ) {
			int code = args[0]->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
			if( code == 1000 || (code >= 3000 && code <= 4999) ) {
				if( args.Length() > 1 ) {
					String::Utf8Value reason( USE_ISOLATE( isolate ) args[1]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
					if( reason.length() > 123 ) {
						isolate->ThrowException( Exception::Error(
							String::NewFromUtf8( isolate, TranslateText( "SyntaxError (text reason longer than 123)" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
						return;
					}
					if( obj->pc ) {
						WebSocketClose( obj->pc, code, *reason);
					}
				}
			}
			else {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "InvalidAccessError Code must be 1000 or 3000-4999." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
				return;
			}
		}
		else {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( "InvalidAccessError Code is not a number." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
			return;
		}
	}
}

void wscObject::write( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	//lprintf( "Send From JS" );
	if( !obj->pc ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Connection is already closed." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	if( args[0]->IsTypedArray() ) {
		Local<TypedArray> ta = Local<TypedArray>::Cast( args[0] );
		Local<ArrayBuffer> ab = ta->Buffer();
#if ( NODE_MAJOR_VERSION >= 14 )
		WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
		WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
#endif
	} else if( args[0]->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
#if ( NODE_MAJOR_VERSION >= 14 )
		WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
		WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
#endif
	}
	else if( args[0]->IsString() ) {
		String::Utf8Value buf( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		WebSocketSendText( obj->pc, *buf, buf.length() );
	}
	else {
		lprintf( "Unhandled message format" );
	}
}


void wscObject::getReadyState( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	if( !obj->pc )
		args.GetReturnValue().Set( Integer::New( isolate, (int)-1 ) );
	else
	args.GetReturnValue().Set( Integer::New( isolate, (int)obj->readyState ) );
#if 0
	Local<Object> h = args.Holder();
	Local<Object> t = args.This();
	//if( wssObject::tpl. )
		class constructorSet *c = getConstructors( isolate );
	Local<FunctionTemplate> wrapper_tpl = wscTpl.Get( isolate );
	int ht = wrapper_tpl->HasInstance( h );
	int tt = wrapper_tpl->HasInstance( t );
	wscObject *obj;
	if( ht )
		obj = ObjectWrap::Unwrap<wscObject>( h );
	else if( tt )
		obj = ObjectWrap::Unwrap<wscObject>( t );
	else
		obj = NULL;
	if( obj )
		args.GetReturnValue().Set( Integer::New( args.GetIsolate(), (int)obj->readyState ) );
#endif
}


httpRequestObject::httpRequestObject():_this() {
	pc = NULL;
	ssl = false;
	port = 0;
	hostname = NULL;
	method = "GET";
	ca = NULL;
	path = NULL;
	rejectUnauthorized = true;
	firstDispatchDone = false;
	dataDispatch = false;
	endDispatch = false;
	finished = false;
	waiter = NULL;
	result = NULL;
	eventQueue = NULL;

}

httpRequestObject::~httpRequestObject() {
	
}

void httpRequestObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		httpRequestObject *request = new httpRequestObject();
		Local<Object> _this = args.This();
		request->_this.Reset( isolate, _this );
		request->Wrap( _this );

		class constructorSet* c = getConstructors( isolate );
		uv_async_init( c->loop, &request->async, httpRequestAsyncMsg );

		args.GetReturnValue().Set( _this );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		class constructorSet *c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->httpReqConstructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );
	}

}

void httpRequestObject::wait( const FunctionCallbackInfo<Value>& args ) {
	//httpRequestObject *http = ObjectWrap::Unwrap<httpRequestObject>( args.This() );

	args.GetReturnValue().Set( args.This() );

}

void httpRequestObject::on( const FunctionCallbackInfo<Value>& args ) {
	//String::Utf8Value eventName( USE_ISOLATE( args.GetIsolate() ) args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
	//httpRequestObject *http = ObjectWrap::Unwrap<httpRequestObject>( args.This() );

	//Local<Function> cb = Local<Function>::Cast( args[1] );
	/* abort, connect, continue, response, socket, timeout, upgrade, */
	//if( StrCmp( *eventName, "error" ) == 0 ) 
	{

	}
	args.GetReturnValue().Set( args.This() );
}



static void 	readHeaders( Isolate *isolate, Local<Context> context, httpRequestObject* httpRequest, Local<Object> headers ) {
	Local<Array> props = headers->GetPropertyNames( context ).ToLocalChecked();
	for( uint32_t p = 0; p < props->Length(); p++ ) {
		Local<Value> name = props->Get( context, p ).ToLocalChecked();
		Local<Value> value = headers->Get( context, name ).ToLocalChecked();
		String::Utf8Value localName( isolate,  name );
		TEXTCHAR* field = NULL;
		if( value->IsString() ) {
			String::Utf8Value localValue( isolate, value );
			const size_t len = localName.length() + localValue.length() + 2;
			field = NewArray( TEXTCHAR, len );
			// HTTP header requirements dictate NO control characters are meant to be sent.
			snprintf( field, len, "%s:%s", *localName, *localValue );
		} else if( value->IsUndefined() ) {
			const size_t len = localName.length()+2;
			field = NewArray( TEXTCHAR, len );
			snprintf( field, len, "%s~", *localName );
		}
		if( field )
			AddLink( &httpRequest->headers, field );
	}
}


static void requestLongBufferWrite( uintptr_t userData ) {
	//httpRequestObject* httpRequest = (httpRequestObject*)userData;

}

static uintptr_t DoRequest( PTHREAD thread ) {
    uintptr_t psv = GetThreadParam( thread );
    httpRequestObject *req = (httpRequestObject *)psv;
    //struct HTTPRequestOptions *opts = (struct HTTPRequestOptions*)psv;
    HTTPState state = NULL;
    int retries;

    for( retries = 0; !state && retries < 3; retries++ ) {
    	//lprintf( "request: %s  %s", GetText( address ), GetText( url ) );

    	//if( httpRequest->ssl )
    	state = GetHttpsQueryEx( req->opts->address, req->opts->url, req->opts->certChain, req->opts );

    }

    struct httpRequestEvent *pevt = GetHttpRequestEvent();
	//lprintf( "posting request event to JS  %s", GetText( GetHttpRequest( GetWebSocketHttpState( pc ) ) ) );
//	SetWebSocketHttpCloseCallback( req->pc, webSockHttpClose );
//	SetNetworkWriteComplete( req->pc, webSocketWriteComplete );
	if (!req->resultCallback.IsEmpty()) {
		(*pevt).eventType = WS_EVENT_RESPONSE;
		//(*pevt).waiter = MakeThread();
		(*pevt)._this = req;
		(*pevt).state = state;
		EnqueLink(&req->eventQueue, pevt);
#ifdef DEBUG_EVENTS
		lprintf( "socket DoRequest Send %p", &req->async);
#endif		
		uv_async_send(&req->async);
	}
	else {
		req->state = state;
		req->finished = true;
		WakeThread(req->waiter);
	}

    return 0;
}

void httpRequestObject::getRequest( const FunctionCallbackInfo<Value>& args, bool secure ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	httpRequestObject *httpRequest = new httpRequestObject();

	if( secure )
		httpRequest->port = 443;
	else
		httpRequest->port = 80;
	httpRequest->ssl = secure;

	// incoming message events; aborted, close, readableStream (close, data, end, error, readable)
	Local<Object> options = args[0]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
	Local<String> optName;
	optionStrings *strings = getStrings( isolate );

	if( options->Has( context, optName = strings->hostnameString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value value( USE_ISOLATE( isolate ) GETV( options, optName ) );
		httpRequest->hostname = StrDup( *value );
	}

	if( options->Has( context, optName = strings->portString->Get( isolate ) ).ToChecked() ) {
		int32_t x = GETV( options, optName )->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		httpRequest->port = x;
	}
	if (options->Has(context, optName = strings->headerString->Get(isolate)).ToChecked()) {
		Local<Object> headers = GETV( options, optName ).As<Object>();
		readHeaders( isolate, context, httpRequest, headers );
	}

	if( options->Has( context, optName = strings->methodString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value value( USE_ISOLATE( isolate ) GETV( options, optName ) );
		httpRequest->method = StrDup( *value );
	}

	if( options->Has( context, optName = strings->retriesString->Get( isolate ) ).ToChecked() ) {
		int32_t x = GETV( options, optName )->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		httpRequest->retries = x;
	}
	if( options->Has( context, optName = strings->versionString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value value( USE_ISOLATE( isolate ) GETV( options, optName ) );
		httpRequest->httpVersion = StrDup(*value);
	}
	if( options->Has( context, optName = strings->timeoutString->Get( isolate ) ).ToChecked() ) {
		int32_t x = GETV( options, optName )->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		httpRequest->timeout = x;
	}

	if (options->Has(context, optName = strings->contentString->Get(isolate)).ToChecked()) {
		Local<Value> content = GETV( options, optName );
		if( content->IsArrayBuffer() ) {
			// zero copy method... content 
			Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );

#if ( NODE_MAJOR_VERSION >= 14 )
			httpRequest->content = (char*)ab->GetBackingStore()->Data();
			httpRequest->contentLen = ab->GetBackingStore()->ByteLength();
#else
			httpRequest->content = (char*)ab->GetContents().Data();
			httpRequest->contentLen = ab->ByteLength();
#endif

		} else {
			// converted from JS widechar to utf8... (transform+copy)
			String::Utf8Value value(USE_ISOLATE(isolate) GETV(options, optName));
			// this is a tempary value, that we'll need to keep a copy of... (copy 2, copy 3 to nework)
			httpRequest->content = StrDup( *value );
			httpRequest->contentLen = value.length();
		}
        } else {
            httpRequest->content = NULL;
            httpRequest->contentLen = 0;
        }

	if( secure && options->Has( context, optName = strings->caString->Get( isolate ) ).ToChecked() ) {
		if( GETV( options, optName )->IsString() ) {
			String::Utf8Value value( USE_ISOLATE( isolate ) GETV( options, optName ) );
			httpRequest->ca = StrDup( *value );
		}
	}

	if( options->Has( context, optName = strings->rejectUnauthorizedString->Get( isolate ) ).ToChecked() ) {
		httpRequest->rejectUnauthorized = GETV( options, optName )->ToBoolean( isolate )->Value();
	}

	if( options->Has( context, optName = strings->pathString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value value( USE_ISOLATE( isolate ) GETV( options, optName ) );
		httpRequest->path = StrDup( *value );
	}
	if( options->Has( context, optName = strings->onReplyString->Get( isolate ) ).ToChecked() ) {
		httpRequest->_this.Reset( isolate, args.Holder() );
		httpRequest->resultCallback.Reset( isolate, GETV( options, optName ).As<Function>());
	}
	if( options->Has( context, optName = strings->agentString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value value( USE_ISOLATE( isolate ) GETV( options, optName ) );
		httpRequest->agent = StrDup( *value );
	}

	// make sure we have at least 2 words?
	NetworkWait( NULL, 256, 2 );

	if (!httpRequest->resultCallback.IsEmpty()) {
		class constructorSet* c = getConstructors(isolate);
		uv_async_init(c->loop, &httpRequest->async, httpRequestAsyncMsg);
		httpRequest->async.data = httpRequest;
	}
	{
		PVARTEXT pvtAddress = VarTextCreate();
		vtprintf( pvtAddress, "%s:%d", httpRequest->hostname, httpRequest->port );

		PTEXT address = VarTextGet( pvtAddress );
		PTEXT url = SegCreateFromText( httpRequest->path );

		//HTTPState state = NULL;

		struct HTTPRequestOptions *opts = NewPlus( struct HTTPRequestOptions, 0 );
		MemSet(opts, 0, sizeof(*opts ) );
		httpRequest->opts = opts;
		opts->url = url;
		opts->rejectUnauthorized = httpRequest->rejectUnauthorized;
		opts->address = address;
		opts->ssl = httpRequest->ssl;
		opts->headers = httpRequest->headers;
		opts->httpVersion = httpRequest->httpVersion;
		opts->timeout = httpRequest->timeout;
		opts->retries = httpRequest->retries;
		opts->certChain = httpRequest->ca;
		opts->method = httpRequest->method;
		opts->agent = httpRequest->agent;
		opts->contentLen = httpRequest->contentLen;
		opts->content = httpRequest->content;

		opts->writeComplete = requestLongBufferWrite;
		opts->userData = (uintptr_t)httpRequest;

		ThreadTo( DoRequest, (uintptr_t)httpRequest );
		VarTextDestroy( &pvtAddress );
	}

	if (httpRequest->resultCallback.IsEmpty()) {
		// if no callback, block until response is 'finished'
		httpRequest->waiter = MakeThread();
		while (!httpRequest->finished)
			WakeableSleep(1000);

		{
			HTTPState state = httpRequest->state;
			Local<Object> result; result = Object::New(isolate);

			{
				HTTPRequestOptions* opts; opts = httpRequest->opts;
				char* header;
				INDEX idx;
				// cleanup what we allocated.
				LIST_FORALL(opts->headers, idx, char*, header)
					Release(header);
				DeleteList(&opts->headers);
			}
			Deallocate( char*, (char*)(httpRequest->httpVersion) );
			Deallocate( char*, (char*)httpRequest->method );
			if (!state) {
				SET(result, "error",
					state ? String::NewFromUtf8(isolate, "No Content", v8::NewStringType::kNormal).ToLocalChecked() : String::NewFromUtf8(isolate, "Connect Error", v8::NewStringType::kNormal).ToLocalChecked());
			}
			else {
				PTEXT content; content = GetHttpContent( state );
				if( GetHttpResponseCode( state ) ) {
					if( content ) {
						SET( result, "content"
							, String::NewFromUtf8( isolate, GetText( content ), v8::NewStringType::kNormal ).ToLocalChecked() );
					}
					SET( result, "statusCode"
						, Integer::New( isolate, GetHttpResponseCode( state ) ) );
					const char* textCode = GetHttpResponseStatus( state );

					SET( result, "status"
						, String::NewFromUtf8( isolate, textCode ? textCode : "NO RESPONSE", v8::NewStringType::kNormal ).ToLocalChecked() );
					Local<Array> arr = Array::New( isolate );
					PLIST headers = GetHttpHeaderFields( state );
					INDEX idx;
					struct HttpField* header;
					//headers
					LIST_FORALL(headers, idx, struct HttpField*, header) {
						SETT(arr, header->name
							, String::NewFromUtf8(isolate, (const char*)GetText(header->value)
								, NewStringType::kNormal, (int)GetTextSize(header->value)).ToLocalChecked());
					}
					SET( result, "headers", arr );
				} else {
					SET( result, "error", String::NewFromUtf8Literal( isolate, "Bad Parsing State" ) );
				}
				DestroyHttpState( state );
			}
			args.GetReturnValue().Set(result);
		}
		httpRequest->resultObject.Reset();
	}
}

void httpRequestObject::get( const FunctionCallbackInfo<Value>& args ) {
	getRequest( args, false );
}
void httpRequestObject::gets( const FunctionCallbackInfo<Value>& args ) {
	getRequest( args, true );
}


static void promoteSSH_to_Websocket( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	SSH2_RemoteListen* listener = SSH2_RemoteListen::Unwrap<SSH2_RemoteListen>( args[0]->ToObject( context ).ToLocalChecked() );
	Local<Object> wssOpts = Object::New( isolate );
	SET( wssOpts, "pipe", True( isolate ) );
	//getstrings
	class constructorSet* c = getConstructors( isolate );
	Local<Value> argv[1] = { wssOpts };
	Local<Function> cons = Local<Function>::New( isolate, c->wssConstructor );
	MaybeLocal<Object> result = cons->NewInstance( isolate->GetCurrentContext(), 1, argv );
	if( result.IsEmpty() ){
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Failed to create websocket object." ) ) );
		return;
	}	
	args.GetReturnValue().Set( result.ToLocalChecked() );
	wssObject *wss = wssObject::Unwrap<wssObject>( result.ToLocalChecked() );
	
	listener->wss = wss;
}




static void WSReverseChannelData( uintptr_t psv, int stream, const uint8_t* data, size_t length ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	//lprintf( "ReverseChannelData (usually masked so can't see" );
	//LogBinary( data, length );
	WebSocketWrite( (struct html5_web_socket*)channel->wsPipe, data, length );
}

static int WSReverseChannelSend( uintptr_t psv, CPOINTER data, size_t length ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	/* this is probably the only one workring right now */
	//lprintf( "ReverseChannelSend" );
	//LogBinary( data, length );
	sack_ssh_channel_write( channel->channel, 0, (const uint8_t*)data, length );
	return (int)length&0x7FFFFFF;
}

static void WSReverseChannelClose( SSH2_Channel* channel, struct html5_web_socket* wsPipe ) {
	lprintf( "ReverseChannelClose(should generate a SSH event)" );
	if( channel->internal_closeCallback ) {
		channel->internal_closeCallback( channel, wsPipe );
	} else
		sack_ssh_channel_close( channel->channel );
}

static void WSReverseChannelEOF( SSH2_Channel* channel, struct html5_web_socket* wsPipe ) {
	// if I got an EOF, I won't get any more data, I won't get the close code back.
	// this should term-close the callback...
	lprintf( "ReverseChannelEOF (should generate a SSH event)" );
	//sack_ssh_channel_send_eof( channel->channel );
	if( channel->internal_eofCallback ) {
		channel->internal_eofCallback( channel, wsPipe );
	} else
		sack_ssh_channel_eof( channel->channel );
}

static void WSPipeClosed( uintptr_t psv ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	lprintf( "WSPipeClosed (should generate a SSH event)" );
	sack_ssh_channel_close( channel->channel );
}

// creates a enw channel on a listener.
//  this is a connection that is being made to the server side, 
// it's the low level socket type connection.
static uintptr_t WSReverseConnectCallback( uintptr_t psv, struct ssh_listener* ssh_listener, struct ssh_channel* channel ) {
	EnterCriticalSec( &l.csConnect );
	class SSH2_RemoteListen* listener = (class SSH2_RemoteListen*)psv;
	sack_ssh_set_channel_data( channel, WSReverseChannelData );


	//sack_ssh_set_channel_close( channel, ReverseChannelClose );
	//sack_ssh_set_channel_eof( channel, ReverseChannelEOF );
	makeEvent( event );
	lprintf( "WS Reverse Connect Callback (should have new channel)" );
	struct html5_web_socket* newPipe = WebSocketPipeConnect( (struct html5_web_socket*)listener->wss->wsPipe, (uintptr_t)0 );

	event->code = SSH2_EVENT_WS_REVERSE_CONNECT;
	event->data = (void*)channel;
	// needs a new pipe here already
	event->data2 = newPipe;

	event->waiter = MakeThread();
	event->done = 0;
	EnqueLink( &listener->ssh2->eventQueue, event );
	// connect part first goes to JS on Connect, then comes back to here to get the channel.
	uv_async_send( &listener->ssh2->async );
	while( !event->done ) {
		WakeableSleep( 1000 );
	}
	// this would be a PCLIENT in the network implementation.
	SSH2_Channel* newChannel = (SSH2_Channel*)event->data;

	newChannel->internal_closeCallback = WSReverseChannelClose;
	newChannel->internal_eofCallback = WSReverseChannelEOF;
	newChannel->remoteListen = listener;
	/* this needs to be far enough into the system to have a wssiRef*/
	// this sets up for when the remote side has sent data, this is received on in this code */
	WebSocketPipeSetSend( newChannel->wsPipe, WSReverseChannelSend, (uintptr_t)newChannel );
	WebSocketPipeSetClose( newChannel->wsPipe, WSPipeClosed, (uintptr_t)newChannel );

	dropEvent( event );
	LeaveCriticalSec( &l.csConnect );
	// the remaining callbacks should get a newChannel...
	return (uintptr_t)newChannel;
}


uintptr_t WSReverseCallback( uintptr_t psv, struct ssh_listener* listener, int boundPort ) {
	SSH2_Object* ssh = (SSH2_Object*)psv;
	makeEvent( event );
	sack_ssh_set_forward_listen_accept( listener, WSReverseConnectCallback );
	//lprintf( "create a reverse channe..." );
	event->code = SSH2_EVENT_WS_REVERSE_CHANNEL;
	event->data = (void*)listener;
	event->waiter = MakeThread();
	event->done = 0;
	EnqueLink( &ssh->eventQueue, event );
	uv_async_send( &ssh->async );
	while( !event->done ) {
		WakeableSleep( 1000 );
	}
	uintptr_t result = (uintptr_t)event->data;
	dropEvent( event );
	return (uintptr_t)result;

}

