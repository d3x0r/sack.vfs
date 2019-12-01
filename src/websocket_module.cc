
#include "global.h"

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

enum wsReadyStates {
	CONNECTING = 0,
	OPEN = 1,
	CLOSING = 2,
	CLOSED = 3,
	INITIALIZING = -1,
	LISTENING = 4,
};

enum wsEvents {
	WS_EVENT_OPEN,   // onaccept/onopen callback (wss(passes wssi),wsc)
	WS_EVENT_ACCEPT, // onaccept callback (wss)
	WS_EVENT_CLOSE,  // onClose callback (wss(internal only), wssi, wsc)
	WS_EVENT_READ,   // onMessage callback (wssi,wsc)
	WS_EVENT_ERROR,  // error event (unused)  (wssi,wss,wsc)
	WS_EVENT_REQUEST,// websocket non-upgrade request
	WS_EVENT_ERROR_CLOSE, // illegal server connection
	WS_EVENT_LOW_ERROR,
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
	Eternal<String> *headerString;
	Eternal<String> *certString;
	Eternal<String> *CGIString;
	Eternal<String> *contentString;
	Eternal<String> *keyString;
	Eternal<String> *pemString;
	Eternal<String> *passString;
	Eternal<String> *deflateString;
	Eternal<String> *deflateAllowString;
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
};

struct pendingSend {
	LOGICAL binary;
	POINTER buffer;
	size_t buflen;
};

struct wssEvent {
	enum wsEvents eventType;
	class wssObject *_this;
	union {
		struct {
			int accepted;
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
};
typedef struct wssEvent WSS_EVENT;
#define MAXWSS_EVENTSPERSET 64
DeclareSet( WSS_EVENT );

struct wssiEvent {
	enum wsEvents eventType;
	class wssiObject *_this;
	//PLIST send;
	//LOGICAL send_close; // need reason.
	POINTER buf;
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
	POINTER buf;
	int code;
	size_t buflen;
	LOGICAL binary;
};
typedef struct wscEvent WSC_EVENT;
#define MAXWSC_EVENTSPERSET 128
DeclareSet( WSC_EVENT );

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
	PLIST transportDestinations;
} l;


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
		check->readyStateString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "readyState", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->portString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "port", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->urlString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "url", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->localPortString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "localPort", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->remotePortString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "remotePort", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->addressString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "address", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->localAddrString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "localAddress", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->remoteAddrString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "remoteAddress", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->localFamilyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "localFamily", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->remoteFamilyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "remoteFamily", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->headerString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "headers", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->CGIString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "CGI", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->contentString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "content", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->certString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "cert", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->keyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "key", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->pemString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "pem", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->passString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "passphrase", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->deflateString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "perMessageDeflate", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->deflateAllowString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "perMessageDeflateAllow", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->caString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "ca", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->v4String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "IPv4", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->v6String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "IPv6", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->vUString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "unknown", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->connectionString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "connection", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->maskingString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "masking", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->bytesReadString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "bytesRead", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->bytesWrittenString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "bytesWritten", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->requestString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "request", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->bufferedAmountString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "bufferedAmount", v8::NewStringType::kNormal ).ToLocalChecked() );

		check->hostnameString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "hostname", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->hostString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "host", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->hostsString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "hosts", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->pathString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "path", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->methodString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "method", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->redirectString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "redirect", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->rejectUnauthorizedString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "rejectUnauthorized", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->keepAliveString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "keepAlive", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->versionString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "version", v8::NewStringType::kNormal ).ToLocalChecked() );
	}
	return check;
}

// web sock server Object
class wssObject : public node::ObjectWrap {
	LOGICAL closed;
public:
	PCLIENT pc;
	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
	int last_count_handled;
	int closing;
	PTHREAD event_waker;
	PLIST opening;
	PLIST requests;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	//static Persistent<Function> constructor;
	//static Persistent<FunctionTemplate> tpl;
	Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> acceptCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> requestCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorCloseCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorLowCallback; //
	struct wssEvent *eventMessage;
	bool ssl;
	enum wsReadyStates readyState;
	bool immediateEvent;
	Isolate *isolate;
public:

	wssObject( struct wssOptions *opts );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void disableSSL( const FunctionCallbackInfo<Value>& args );
	//static void onMessage( const v8::FunctionCallbackInfo<Value>& args );
	static void onConnect( const v8::FunctionCallbackInfo<Value>& args );
	static void onAccept( const v8::FunctionCallbackInfo<Value>& args );
	static void onRequest( const v8::FunctionCallbackInfo<Value>& args );
	static void onClose( const v8::FunctionCallbackInfo<Value>& args );
	static void getOnClose( const v8::FunctionCallbackInfo<Value>& args );
	static void onError( const v8::FunctionCallbackInfo<Value>& args );
	static void onErrorLow( const v8::FunctionCallbackInfo<Value>& args );
	static void accept( const v8::FunctionCallbackInfo<Value>& args );
	static void reject( const v8::FunctionCallbackInfo<Value>& args );
	static void getReadyState( const FunctionCallbackInfo<Value>& args );

	~wssObject();
};

class httpRequestObject : public node::ObjectWrap {
public:
	PCLIENT pc;
	//static Persistent<Function> constructor;
	Persistent<Object> _this;
	//PVARTEXT pvtResult;
	bool ssl;
	int port;
	char *hostname;
	char *method;
	char *ca;
	char *path;
	bool rejestUnauthorized;

	bool firstDispatchDone;
	bool dataDispatch;
	bool endDispatch;

	bool finished;
	PTHREAD waiter;

	PTEXT result;
public:

	httpRequestObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void wait( const v8::FunctionCallbackInfo<Value>& args );
	static void get( const v8::FunctionCallbackInfo<Value>& args );
	static void gets( const v8::FunctionCallbackInfo<Value>& args );
	static void getRequest( const v8::FunctionCallbackInfo<Value>& args, bool secure );

	Persistent<Function, CopyablePersistentTraits<Function>> resultCallback;
	Persistent<Function, CopyablePersistentTraits<Function>> cbError;

	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback

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
public:

	httpObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void writeHead( const v8::FunctionCallbackInfo<Value>& args );
	static void end( const v8::FunctionCallbackInfo<Value>& args );

	~httpObject();
};

// web sock client Object
class wscObject : public node::ObjectWrap {
	char *serverUrl;
public:
	PCLIENT pc;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
	//wscEvent *eventMessage;
	LOGICAL closed;
	enum wsReadyStates readyState;
	Isolate *isolate;
public:
	//static Persistent<Function> constructor;
	//static Persistent<FunctionTemplate> tpl;
	Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> messageCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; //

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

	~wscObject();
};

// web sock server instance Object  (a connection from a remote)
class wssiObject : public node::ObjectWrap {
public:
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
	PCLIENT pc;
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
	//Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> messageCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; //

public:

	wssiObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void write( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void onmessage( const v8::FunctionCallbackInfo<Value>& args );
	static void onclose( const v8::FunctionCallbackInfo<Value>& args );
	static void getReadyState( const FunctionCallbackInfo<Value>& args );
	static void ping( const v8::FunctionCallbackInfo<Value>& args );

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

static void DropWssEvent( WSS_EVENT *evt ) {
	EnterCriticalSec( &l.csWssEvents );
	DeleteFromSet( WSS_EVENT, l.wssEvents, evt );
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

static void uv_closed_wssi( uv_handle_t* handle ) {
	wssiObject* myself = (wssiObject*)handle->data;
	myself->closeCallback.Reset();
	//myself->openCallback.Reset();
	myself->messageCallback.Reset();
	myself->errorCallback.Reset();
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
	myself->closeCallback.Reset();
	myself->openCallback.Reset();
	myself->messageCallback.Reset();
	myself->errorCallback.Reset();
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

static Local<Object> makeSocket( Isolate *isolate, PCLIENT pc ) {
	Local<Context> context = isolate->GetCurrentContext();
	PLIST headers = GetWebSocketHeaders( pc );
	PTEXT resource = GetWebSocketResource( pc );
	SOCKADDR *remoteAddress = (SOCKADDR *)GetNetworkLong( pc, GNL_REMOTE_ADDRESS );
	SOCKADDR *localAddress = (SOCKADDR *)GetNetworkLong( pc, GNL_LOCAL_ADDRESS );
	Local<String> remote = String::NewFromUtf8( isolate, remoteAddress?GetAddrName( remoteAddress ):"0.0.0.0", v8::NewStringType::kNormal ).ToLocalChecked();
	Local<String> local = String::NewFromUtf8( isolate, localAddress?GetAddrName( localAddress ):"0.0.0.0", v8::NewStringType::kNormal ).ToLocalChecked();
	Local<Object> result = Object::New( isolate );
	Local<Object> arr = Object::New( isolate );
	INDEX idx;
	struct HttpField *header;
	LIST_FORALL( headers, idx, struct HttpField *, header ) {
		SETT( arr, header->name
			, String::NewFromUtf8( isolate, (const char*)GetText( header->value ), NewStringType::kNormal, (int)GetTextSize( header->value ) ).ToLocalChecked() );
	}
	optionStrings *strings = getStrings( isolate );
	SETV( result, strings->headerString->Get( isolate ), arr );
	CTEXTSTR host = ssl_GetRequestedHostName( pc );
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
	SETV( result, strings->remotePortString->Get( isolate ), Integer::New( isolate, (int32_t)GetNetworkLong( pc, GNL_PORT ) ) );
	if( localAddress )
		SETV( result, strings->localFamilyString->Get( isolate )
			, (localAddress->sa_family == AF_INET)?strings->v4String->Get(isolate):
				(localAddress->sa_family == AF_INET6) ? strings->v6String->Get( isolate ) : strings->vUString->Get(isolate)
		);
	SETV( result, strings->localAddrString->Get( isolate ), local );
	SETV( result, strings->localPortString->Get( isolate ), Integer::New( isolate, (int32_t)GetNetworkLong( pc, GNL_MYPORT ) ) );
	return result;
}

static Local<Value> makeRequest( Isolate *isolate, struct optionStrings *strings, PCLIENT pc, int sslRedirect ) {
	// .url
	// .socket
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> req = Object::New( isolate );
	Local<Object> socket;
	struct HttpState *pHttpState = GetWebSocketHttpState( pc );
	if( pHttpState ) {
		struct cgiParams cgi;
		PTEXT content;
		cgi.isolate = isolate;
		cgi.cgi = Object::New( isolate );
		ProcessCGIFields( pHttpState, cgiParamSave, (uintptr_t)&cgi );
		SETV( req, strings->redirectString->Get( isolate ), sslRedirect?True( isolate ):False(isolate) );
		SETV( req, strings->CGIString->Get( isolate ), cgi.cgi );
		SETV( req, strings->versionString->Get( isolate ), Integer::New( isolate, GetHttpVersion( GetWebSocketHttpState( pc ) ) ) );
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
	SETV( req, strings->connectionString->Get( isolate ), socket = makeSocket( isolate, pc ) );
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

static void wssiAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	wssiObject* myself = (wssiObject*)handle->data;

	Local<Context> context = isolate->GetCurrentContext();
	{
		struct wssiEvent* eventMessage;
		while( eventMessage = ( struct wssiEvent* )DequeLink( &myself->eventQueue ) ) {
			Local<Value> argv[1];
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
				size_t length;
				if( !myself->messageCallback.IsEmpty() ) {
					if( eventMessage->binary ) {
						ab =
							ArrayBuffer::New( isolate,
							(void*)eventMessage->buf,
								length = eventMessage->buflen );
						argv[0] = ab;

						PARRAY_BUFFER_HOLDER holder = GetHolder();
						holder->o.Reset( isolate, ab );
						holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
						holder->buffer = eventMessage->buf;

						myself->messageCallback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 1, argv );
					}
					else {
						MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
						argv[0] = buf.ToLocalChecked();
						//lprintf( "Message:', %s", eventMessage->buf );
						myself->messageCallback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 1, argv );
					}
					Deallocate( POINTER, eventMessage->buf );
				}
				break;
			case WS_EVENT_CLOSE:
				if( !myself->closeCallback.IsEmpty() ) {
					myself->closeCallback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 0, argv );
				}
				uv_close( (uv_handle_t*)&myself->async, uv_closed_wssi );
				DropWssiEvent( eventMessage );
				DeleteLinkQueue( &myself->eventQueue );
				myself->closed = 1;
				myself->readyState = CLOSED;
				continue;
				break;
			case WS_EVENT_ERROR:
				myself->errorCallback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 0, argv );
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

static void wssAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	wssObject* myself = (wssObject*)handle->data;
	v8::Isolate* isolate = myself->isolate;//v8::Isolate::GetCurrent();
	HandleScope scope(isolate);
	Local<Context> context = isolate->GetCurrentContext();
	class constructorSet *c = getConstructors( isolate);
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
					argv[1] = makeSocket( isolate, eventMessage->pc );
					if( eventMessage->data.error.buffer )
						argv[2] = ArrayBuffer::New( isolate,
						(void*)eventMessage->data.error.buffer,
							eventMessage->data.error.buflen );
					else
						argv[2] = Null( isolate );
					myself->errorLowCallback.Get( isolate )->Call( context, myself->_this.Get( isolate ), 3, argv );

				}
			}
			else if( eventMessage->eventType == WS_EVENT_REQUEST ) {
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
					SETV( http, strings->connectionString->Get( isolate ), makeSocket( isolate, eventMessage->pc ) );

					httpObject *httpInternal = httpObject::Unwrap<httpObject>( http );
					httpInternal->wss = myself;
					httpInternal->ssl = ssl_IsClientSecure( eventMessage->pc );
					int sslRedirect = (httpInternal->ssl != myself->ssl);
					httpInternal->pc = eventMessage->pc;
					AddLink( &myself->requests, httpInternal );
					//if( myself->requests->Cnt > 200 )
					//	DebugBreak();
					//lprintf( "requests %p is %d", myself->requests, myself->requests->Cnt );
					//lprintf( "New request..." );
					argv[0] = makeRequest( isolate, strings, eventMessage->pc, sslRedirect );
					if( !argv[0]->IsNull() ) {
						argv[1] = http;
						Local<Function> cb = Local<Function>::New( isolate, myself->requestCallback );
						cb->Call( context, eventMessage->_this->_this.Get( isolate ), 2, argv );
					}
					//else
					//	lprintf( "This request was a false alarm, and was empty." );
				}
			}
			else if( eventMessage->eventType == WS_EVENT_ACCEPT ) {
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
				struct optionStrings *strings = getStrings( isolate );
				Local<Object> socket;
				wssiObject *wssiInternal = wssiObject::Unwrap<wssiObject>( wssi );
				if( !eventMessage->pc )
					lprintf( "FATALITY - ACCEPT EVENT IS SETTING SOCKET TO NULL." );
				wssiInternal->pc = eventMessage->pc;
				wssiInternal->server = myself;
				AddLink( &myself->opening, wssiInternal );
				eventMessage->result = wssiInternal;

				SETV( wssi, strings->connectionString->Get(isolate), socket = makeSocket( isolate, eventMessage->pc ) );
				SETV( wssi, strings->headerString->Get( isolate ), GETV( socket, strings->headerString->Get( isolate ) ) );
				SETV( wssi, strings->urlString->Get( isolate )
					, String::NewFromUtf8( isolate
						, GetText( GetHttpRequest( GetWebSocketHttpState( wssiInternal->pc ) ) ), v8::NewStringType::kNormal ).ToLocalChecked() );

				argv[0] = wssi;
				wssiInternal->acceptEventMessage = eventMessage;

				if( !myself->acceptCallback.IsEmpty() ) {
					Local<Function> cb = myself->acceptCallback.Get( isolate );
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
					uv_async_init( c->loop, &wssiInternal->async, wssiAsyncMsg );
				}
				else
					eventMessage->data.request.accepted = 1;
			}
			else if( eventMessage->eventType == WS_EVENT_ERROR_CLOSE ) {
				Local<Object> closingSock = makeSocket( isolate, eventMessage->pc );
				if( !myself->errorCloseCallback.IsEmpty() ) {
					Local<Function> cb = myself->errorCloseCallback.Get( isolate );
					argv[0] = closingSock;
					cb->Call( context, eventMessage->_this->_this.Get( isolate ), 1, argv );
				}

				//myself->readyState = CLOSED;
				//uv_close( (uv_handle_t*)&myself->async, uv_closed_wss );
				DropWssEvent( eventMessage );
				//DeleteLinkQueue( &myself->eventQueue );
				//return;
			}
			else if( eventMessage->eventType == WS_EVENT_CLOSE ) {
				myself->readyState = CLOSED;
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
				cb = Local<Function>::New( isolate, wsc->openCallback );
				if( !cb.IsEmpty() ) {
					struct optionStrings *strings;
					strings = getStrings( isolate );
					SETV( wsc->_this.Get( isolate ), strings->connectionString->Get( isolate ), makeSocket( isolate, wsc->pc ) );
					cb->Call( context, eventMessage->_this->_this.Get( isolate ), 0, argv );
				}
				break;
			case WS_EVENT_READ:
				size_t length;
				if( eventMessage->binary ) {
					ab =
						ArrayBuffer::New( isolate,
						(void*)eventMessage->buf,
							length = eventMessage->buflen );
					argv[0] = ab;

					PARRAY_BUFFER_HOLDER holder = GetHolder();
					holder->o.Reset( isolate, ab );
					holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
					holder->buffer = eventMessage->buf;

					wsc->messageCallback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 1, argv );
				}
				else {
					MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
					argv[0] = buf.ToLocalChecked();
					wsc->messageCallback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 1, argv );
				}
				Deallocate( POINTER, eventMessage->buf );
				break;
			case WS_EVENT_ERROR:
				{
					argv[0] = Integer::New( isolate, eventMessage->code );
					if( eventMessage->buf ) {
						MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
						argv[1] = buf.ToLocalChecked();
						Deallocate( POINTER, eventMessage->buf );
					}
					else
						argv[1] = Undefined( isolate );
					wsc->errorCallback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 2, argv );
				}
				break;
			case WS_EVENT_CLOSE:
				cb = Local<Function>::New( isolate, wsc->closeCallback );
				if( !cb.IsEmpty() )
					cb->Call( context, eventMessage->_this->_this.Get( isolate ), 0, argv );
				uv_close( (uv_handle_t*)&wsc->async, uv_closed_wsc );
				DeleteLinkQueue( &wsc->eventQueue );
				wsc->readyState = CLOSED;
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
	struct socketTransport* trans = new struct socketTransport();
	trans->wssi = obj;
	trans->acceptEventMessage = obj->acceptEventMessage;

	obj->blockReturn = TRUE;
	obj->acceptEventMessage = NULL;
	trans->protocolResponse = obj->protocolResponse;
	obj->protocolResponse = NULL;
	{
		struct socketUnloadStation* station;
		INDEX idx;
		LIST_FORALL( l.transportDestinations, idx, struct socketUnloadStation*, station ) {
			if( memcmp( *station->s[0], *(name[0]), (name[0]).length() ) == 0 ) {
				AddLink( &station->transport, trans );
				//lprintf( "Send Post Request %p", station->clientSocketPoster );
				uv_async_send( &station->clientSocketPoster );

				break;
			}
		}
		if( !station ) {
			return FALSE;
			//isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Failed to find target accepting thread", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		}
	}
	return TRUE;
}

static void postClientSocket( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Required parameter missing: (unique,socket)", v8::NewStringType::kNormal ).ToLocalChecked() ) );
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
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Second paramter is not an accepted socket", v8::NewStringType::kNormal ).ToLocalChecked() ) );
	}
}



static void postClientSocketObject( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 1 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Required parameter missing: (unique)", v8::NewStringType::kNormal ).ToLocalChecked() ) );
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
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Object is not an accepted socket", v8::NewStringType::kNormal ).ToLocalChecked() ) );
	}
}


static void blockClientSocketAccept( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject* obj = wssObject::Unwrap<wssiObject>( args.This() );
	if( obj ) {
		obj->blockReturn = TRUE;
	}
}

static void resumeClientSocketAccept( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
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
		SETV( newThreadSocket, strings->connectionString->Get(isolate), socket = makeSocket( isolate, obj->pc ) );
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
	Local<Object> o = Object::New( isolate );

	Local<Object> wsWebStatesObject = Object::New( isolate );
	SET_READONLY( wsWebStatesObject, "OPEN", Integer::New( isolate, wsReadyStates::OPEN ) );
	SET_READONLY( wsWebStatesObject, "CLOSED", Integer::New( isolate, wsReadyStates::CLOSED ) );
	SET_READONLY( wsWebStatesObject, "CLOSING", Integer::New( isolate, wsReadyStates::CLOSING ) );
	SET_READONLY( wsWebStatesObject, "CONNECTING", Integer::New( isolate, wsReadyStates::CONNECTING ) );
	SET_READONLY( wsWebStatesObject, "INITIALIZING", Integer::New( isolate, wsReadyStates::INITIALIZING ) );
	SET_READONLY( wsWebStatesObject, "LISTENING", Integer::New( isolate, wsReadyStates::LISTENING ) );
	SET_READONLY( o, "readyStates", wsWebStatesObject );

	SET_READONLY( exports, "WebSocket", o );
	class constructorSet *c = getConstructors( isolate );
	{
		Local<FunctionTemplate> httpTemplate;
		httpTemplate = FunctionTemplate::New( isolate, httpObject::New );
		httpTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.http.requestHandler", v8::NewStringType::kNormal ).ToLocalChecked() );
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
		httpRequestTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.HTTP[S]", v8::NewStringType::kNormal ).ToLocalChecked() );
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
		Local<FunctionTemplate> wssTemplate;
		wssTemplate = FunctionTemplate::New( isolate, wssObject::New );
		wssTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.ws.server", v8::NewStringType::kNormal ).ToLocalChecked() );
		wssTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "close", wssObject::close );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "disableSSL", wssObject::disableSSL );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "on", wssObject::on );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onconnect", wssObject::onConnect );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onaccept", wssObject::onAccept );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onrequest", wssObject::onRequest );
		wssTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "onclose", v8::NewStringType::kNormal ).ToLocalChecked()
			, FunctionTemplate::New( isolate, wssObject::getOnClose )
			, FunctionTemplate::New( isolate, wssObject::onClose )
		);
		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onclose", wssObject::onClose );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onerror", wssObject::onError );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onerrorlow", wssObject::onErrorLow );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "accept", wssObject::accept );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "reject", wssObject::reject );

		wssTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "readyState", v8::NewStringType::kNormal ).ToLocalChecked()
			, FunctionTemplate::New( isolate, wssObject::getReadyState )
			, Local<FunctionTemplate>() );
		//wssTemplate->SetNativeDataProperty( String::NewFromUtf8( isolate, "readyState", v8::NewStringType::kNormal ).ToLocalChecked()
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
		wscTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.ws.client", v8::NewStringType::kNormal ).ToLocalChecked() );
		wscTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( wscTemplate, "close", wscObject::close );
		NODE_SET_PROTOTYPE_METHOD( wscTemplate, "send", wscObject::write );
		NODE_SET_PROTOTYPE_METHOD( wscTemplate, "on", wscObject::on );
		NODE_SET_PROTOTYPE_METHOD( wscTemplate, "ping", wscObject::ping );
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "readyState", v8::NewStringType::kNormal ).ToLocalChecked()
			, FunctionTemplate::New( isolate, wscObject::getReadyState )
			, Local<FunctionTemplate>() );
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "onopen", v8::NewStringType::kNormal ).ToLocalChecked()
			, FunctionTemplate::New( isolate, wscObject::getOnOpen )
			, FunctionTemplate::New( isolate, wscObject::onOpen )
		);
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "onmessage", v8::NewStringType::kNormal ).ToLocalChecked()
			, FunctionTemplate::New( isolate, wscObject::getOnMessage )
			, FunctionTemplate::New( isolate, wscObject::onMessage )
		);
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "onclose", v8::NewStringType::kNormal ).ToLocalChecked()
			, FunctionTemplate::New( isolate, wscObject::getOnClose )
			, FunctionTemplate::New( isolate, wscObject::onClose )
		);
		wscTemplate->ReadOnlyPrototype();

		c->wscConstructor.Reset( isolate, wscTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
		c->wscTpl.Reset( isolate, wscTemplate );

		SET_READONLY( o, "Client", wscTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	}

	{
		Local<FunctionTemplate> wssiTemplate;
		wssiTemplate = FunctionTemplate::New( isolate, wssiObject::New );
		wssiTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.ws.connection", v8::NewStringType::kNormal ).ToLocalChecked() );
		wssiTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "post", postClientSocketObject );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "block", blockClientSocketAccept );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "resume", resumeClientSocketAccept );

		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "send", wssiObject::write );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "close", wssiObject::close );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "on", wssiObject::on );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "ping", wssiObject::ping );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "onmessage", wssiObject::onmessage );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "onclose", wssiObject::onclose );
		wssiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "readyState", v8::NewStringType::kNormal ).ToLocalChecked()
			, FunctionTemplate::New( isolate, wssiObject::getReadyState )
			, Local<FunctionTemplate>() );
		wssiTemplate->ReadOnlyPrototype();

		c->wssiConstructor.Reset( isolate, wssiTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
		c->wssiTpl.Reset( isolate, wssiTemplate );
		//SET_READONLY( o, "Client", wscTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	}
}

static void Wait( void ) {
	uint32_t tick = GetTickCount();
	WakeableSleep( 250 );
	if( (GetTickCount() - tick) > 200 ) {
		tick = GetTickCount() - tick;
		lprintf( "slept %d", tick );
	}
}
#define Wait() WakeableSleep( 100 )


static uintptr_t webSockServerOpen( PCLIENT pc, uintptr_t psv ){
	wssObject *wss = (wssObject*)psv;
	while( !wss->eventQueue )
		Relinquish();
	INDEX idx;
	wssiObject *wssi;
	LIST_FORALL( wss->opening, idx, wssiObject*, wssi ) {
		if( wssi->pc == pc ) {
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
		//lprintf( "Send open event?" );
		(*pevt).eventType = WS_EVENT_OPEN;
		(*pevt)._this = wssi;
		EnqueLink( &wssi->eventQueue, pevt );
		//lprintf( "Send Event:%p", &wss->async );
		uv_async_send( &wssi->async );
		// can't change this result later, so need to send 
		// it as a refernence, in case the JS object changes
		return (uintptr_t)wssi->wssiRef;
	}
	if( !wssi )
		lprintf( "FAILED TO HAVE WSSI to open." );
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
	else
		uv_async_send( &wss->async );
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
		wssi->pc = NULL;
		EnqueLink( &wssi->eventQueue, pevt );
		uv_async_send( &wssi->async );
	}
	else {
		uintptr_t psvServer = WebSocketGetServerData( pc );
		wssObject *wss = (wssObject*)psvServer;
		if( wss ) {	
			httpObject *req;
			INDEX idx;
			int tot = 0;
			LOGICAL requested = FALSE;
			// close on wssObjectEvent; may have served HTTP requests
			//lprintf( "requests %p is %d", wss->requests, wss->requests?wss->requests->Cnt:0 );
			LIST_FORALL( wss->requests, idx, httpObject *, req ) {
				tot++;
				if( req->pc == pc ) {
					//lprintf( "Removing request from wss %d %d", idx, tot );
					SetLink( &wss->requests, idx, NULL );
					requested = TRUE;
					tot--;
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
			else
				uv_async_send( &wss->async );

			while( !(*pevt).done )
				Wait();
		} else {
			Isolate *isolate = Isolate::GetCurrent();
			Local<Object> closingSock = makeSocket( isolate, pc );
			if( !wss->errorCloseCallback.IsEmpty() ) {
				Local<Function> cb = wss->errorCloseCallback.Get( isolate );
				Local<Value> argv[1];
				argv[0] = closingSock;
				cb->Call( isolate->GetCurrentContext(), wss->_this.Get( isolate ), 1, argv );
			}
			DropWssEvent( pevt );
		}
	}
}

static void webSockServerError( PCLIENT pc, uintptr_t psv, int error ){
	class wssiObjectReference *wssiRef = (class wssiObjectReference*)psv;
	class wssiObject *wssi = wssiRef->wssi;
	struct wssiEvent *pevt = GetWssiEvent();
	(*pevt).eventType = WS_EVENT_ERROR;
	(*pevt)._this = wssi;
	EnqueLink( &wssi->eventQueue, pevt );
	uv_async_send( &wssi->async );
}

static void webSockServerEvent( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, size_t msglen ){
	//lprintf( "Received:%p %d", buffer, binary );
	class wssiObjectReference *wssiRef = (class wssiObjectReference*)psv;
	class wssiObject *wssi = wssiRef->wssi;
	struct wssiEvent *pevt = GetWssiEvent();
	(*pevt).eventType = WS_EVENT_READ;
	(*pevt)._this = wssi;
	(*pevt).binary = binary;
	(*pevt).buf = NewArray( uint8_t, msglen );
	memcpy( (*pevt).buf, buffer, msglen );
	(*pevt).buflen = msglen;
	EnqueLink( &wssi->eventQueue, pevt );
	uv_async_send( &wssi->async );

}

static LOGICAL webSockServerAccept( PCLIENT pc, uintptr_t psv, const char *protocols, const char *resource, char **protocolReply ) {
	wssObject *wss = (wssObject*)psv;
	struct wssEvent evt;
	evt.data.request.protocol = protocols;
	evt.data.request.resource = resource;
	if( !pc ) lprintf( "FATALITY - ACCEPT EVENT RECEVIED ON A NON SOCKET!?" );

	evt.pc = pc;
	evt.data.request.accepted = 0;
	//lprintf( "Websocket accepted... (blocks until handled.)" );
	evt.eventType = WS_EVENT_ACCEPT;
	evt.done = FALSE;
	evt.waiter = MakeThread();
	evt._this = wss;
	EnqueLink( &wss->eventQueue, &evt );
	if( evt.waiter == l.jsThread ) {
		wssAsyncMsg( &wss->async );
	}
	else
		uv_async_send( &wss->async );

	while( !evt.done )
		Wait();
	if( evt.data.request.protocol != protocols )
		evt.result->protocolResponse = evt.data.request.protocol;
	(*protocolReply) = (char*)evt.data.request.protocol;
	return (uintptr_t)evt.data.request.accepted;
}

httpObject::httpObject() {
	pvtResult = VarTextCreate();
}

httpObject::~httpObject() {
	INDEX idx = FindLink( &this->wss->requests, this );
	if( idx != INVALID_INDEX ) {
		SetLink( &wss->requests, idx, NULL );
	}
}

void httpObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

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
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Headers have already been set; cannot change resulting status or headers", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	int status = 404;
	Local<Object> headers;
	if( args.Length() > 0 ) {
		status = args[0]->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
	}
	HTTPState http = GetWebSocketHttpState( obj->pc );
	if( http ) {
		int vers = GetHttpVersion( http );
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
}

void httpObject::end( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	bool doSend = true;
	httpObject* obj = Unwrap<httpObject>( args.This() );
	int include_close = 1;
	{
		PLIST headers = GetWebSocketHeaders( obj->pc );
		INDEX idx;
		struct HttpField *header;
		LIST_FORALL( headers, idx, struct HttpField *, header ) {
			if( StrCmp( GetText( header->name ), "Connection" ) == 0 ) {
				if( StrCaseCmp( GetText( header->value ), "keep-alive" ) == 0 ) {
					include_close = 0;
				}
			}
		}
		if( !include_close )
			vtprintf( obj->pvtResult, "Connection: keep-alive\r\n" );
	}
	if( args.Length() > 0 ) {
		if( args[0]->IsString() ) {
			String::Utf8Value body( USE_ISOLATE( isolate ) args[0] );
			vtprintf( obj->pvtResult, "content-length:%d\r\n", body.length() );
			vtprintf( obj->pvtResult, "\r\n" );

			vtprintf( obj->pvtResult, "%*.*s", body.length(), body.length(), *body );
		}
		else if( args[0]->IsUint8Array() ) {
			Local<Uint8Array> body = args[0].As<Uint8Array>();
			Local<ArrayBuffer> bodybuf = body->Buffer();
			vtprintf( obj->pvtResult, "content-length:%d\r\n", body->ByteLength() );
			vtprintf( obj->pvtResult, "\r\n" );
			VarTextAddData( obj->pvtResult, (CTEXTSTR)bodybuf->GetContents().Data(), bodybuf->ByteLength() );
		}
		else if( args[0]->IsArrayBuffer() ) {
			Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
			vtprintf( obj->pvtResult, "content-length:%d\r\n", ab->ByteLength() );
			vtprintf( obj->pvtResult, "\r\n" );
			VarTextAddData( obj->pvtResult, (CTEXTSTR)ab->GetContents().Data(), ab->ByteLength() );
		} else if( args[0]->IsObject() ) {
			class constructorSet *c = getConstructors( isolate );
			Local<FunctionTemplate> wrapper_tpl = c->fileTpl.Get( isolate );
			if( (wrapper_tpl->HasInstance( args[0] )) ) {
				FileObject *file = FileObject::Unwrap<FileObject>( args[0]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() );
				lprintf( "Incomplete; streaming file content to socket...." );
				doSend = false;
			}
		}
	}
	else
		vtprintf( obj->pvtResult, "\r\n" );

	if( doSend ) {
		PTEXT buffer = VarTextPeek( obj->pvtResult );
		if( obj->ssl )
			ssl_Send( obj->pc, GetText( buffer ), GetTextSize( buffer ) );
		else
			SendTCP( obj->pc, GetText( buffer ), GetTextSize( buffer ) );
	}
	{
		struct HttpState *pHttpState = GetWebSocketHttpState( obj->pc );
		if( include_close ) {
			//lprintf( "Close is included... is this a reset close?" );
			RemoveClientEx( obj->pc, 0, 1 );
		}
		else {
			if (pHttpState) {
				int result;
				//lprintf( "ending http on %p", obj->pc );
				EndHttp(pHttpState);
				while ((result = ProcessHttp(obj->pc, pHttpState)))
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
						obj->ssl = ssl_IsClientSecure( obj->pc );
						EnqueLink(&obj->wss->eventQueue, pevt);
						//lprintf( "Send Request" );
						if( (*pevt).waiter == l.jsThread ) {
							wssAsyncMsg( &obj->wss->async );
						} else
							uv_async_send(&obj->wss->async);
						break;
					}
				}
				
			}
			else {
				lprintf( "LOST HTTP STATE" );
			}
		}
	}

	VarTextEmpty( obj->pvtResult );
}
static void webSockHttpClose( PCLIENT pc, uintptr_t psv ) {
	wssObject *wss = (wssObject*)psv;
	uintptr_t psvServer = WebSocketGetServerData( pc );
	if( wss ) {
		httpObject *req;
		INDEX idx;
		int tot = 0;
		LOGICAL requested = FALSE;
		// close on wssObjectEvent; may have served HTTP requests
		LIST_FORALL( wss->requests, idx, httpObject *, req ) {
			tot++;
			if( req->pc == pc ) {
				//lprintf( "Removing request from wss %d %d", idx, tot );
				SetLink( &wss->requests, idx, NULL );
				requested = TRUE;
				tot--;
				//return;
			}
		}

		if( requested )
			return;
	}

	lprintf( "(close before accept)Illegal connection" );

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
		lprintf( "Send close Request" );
		uv_async_send( &wss->async );
		while( (*pevt).done )
			Wait();
	}

}

static uintptr_t webSockHttpRequest( PCLIENT pc, uintptr_t psv ) {
	wssObject *wss = (wssObject*)psv;
	if( !wss->requestCallback.IsEmpty() ) {
		struct wssEvent *pevt = GetWssEvent();
		//lprintf( "posting request event to JS  %s", GetText( GetHttpRequest( GetWebSocketHttpState( pc ) ) ) );
		SetWebSocketHttpCloseCallback( pc, webSockHttpClose );
		(*pevt).eventType = WS_EVENT_REQUEST;
		//(*pevt).waiter = MakeThread();
		(*pevt). pc = pc;
		(*pevt)._this = wss;
		EnqueLink( &wss->eventQueue, pevt );
		lprintf( "Send request Request" );
		uv_async_send( &wss->async );
		//while (!(*pevt).done) WakeableSleep(SLEEP_FOREVER);
		//lprintf("queued and evented  request event to JS");
	} else {
		RemoveClient( pc );
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
	EnqueLink( &wss->eventQueue, pevt );
	lprintf( "Send fail " );
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
		if( wss->last_count_handled )
			uv_async_send( &wss->async );
		WakeableSleep( 1000 );
	}
	wss->event_waker = NULL;
	return 0;
}

wssObject::wssObject( struct wssOptions *opts ) {
	char tmp[256];
	int clearUrl = 0;
	last_count_handled = 0;
	closing = 0;
	readyState = INITIALIZING;
	eventQueue = NULL;
	requests = NULL;
	opening = FALSE;
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
	NetworkWait( NULL, 256, 2 );  // 1GB memory

	pc = WebSocketCreate_v2( opts->url, webSockServerOpen, webSockServerEvent, webSockServerClosed, webSockServerError, (uintptr_t)this, WEBSOCK_SERVER_OPTION_WAIT );
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
		SetWebSocketAcceptCallback( pc, webSockServerAccept );
		readyState = LISTENING;
		SetNetworkListenerReady( pc );
	} else {

	}
	if( clearUrl )
		opts->url = NULL;
}

wssObject::~wssObject() {
	//lprintf( "Destruct wssObject" );
	RemoveClient( pc );
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

	if( !opts->Has( context, optName = strings->deflateString->Get( isolate ) ).ToChecked() ) {
		wssOpts->deflate = false;
	}
	else
		wssOpts->deflate = (GETV( opts, optName )->TOBOOL( isolate ));
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
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must specify options for server.", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		struct wssOptions wssOpts;
		int argOfs = 0;
		wssOpts.hostList = NULL;
		wssOpts.url = NULL;
		wssOpts.port = 0;
		wssOpts.address = NULL;
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
		if( !obj->pc ) {
			delete obj;
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Failed to create listener.", v8::NewStringType::kNormal ).ToLocalChecked() ) );
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
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
	if( obj->eventMessage && obj->eventMessage->eventType == WS_EVENT_LOW_ERROR ) {
		// delay until we return to the thread that dispatched this error.
		obj->eventMessage->data.error.fallback_ssl = 1;
		//ssl_EndSecure( obj->eventMessage->pc, (POINTER)obj->eventMessage->data.error.buffer, obj->eventMessage->data.error.buflen );
	}
}

void wssObject::close( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
	obj->readyState = CLOSING;
	lprintf( "remove client." );
	RemoveClient( obj->pc );
	webSockServerCloseEvent( obj );
}

void wssObject::on( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() == 2 ) {
		Isolate* isolate = args.GetIsolate();
		String::Utf8Value event( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		Local<Function> cb = Local<Function>::Cast( args[1] );
		if( !cb->IsFunction() ) {
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Argument is not a function", v8::NewStringType::kNormal ).ToLocalChecked() ) );
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
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Argument is not a function", v8::NewStringType::kNormal ).ToLocalChecked() ) );
	}
}

void wssObject::onAccept( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->acceptCallback.Reset( isolate, Local<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Argument is not a function", v8::NewStringType::kNormal ).ToLocalChecked() ) );
	}
}

void wssObject::onRequest( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->requestCallback.Reset( isolate, Local<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Argument is not a function", v8::NewStringType::kNormal ).ToLocalChecked() ) );
	}
}

void wssObject::onError( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->errorCloseCallback.Reset( isolate, Local<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Argument is not a function", v8::NewStringType::kNormal ).ToLocalChecked() ) );
	}
}

void wssObject::onErrorLow( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->errorLowCallback.Reset( isolate, Local<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Argument is not a function", v8::NewStringType::kNormal ).ToLocalChecked() ) );
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
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Reject cannot be used outside of connection callback.", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	if( args.Length() > 0 ) {
		String::Utf8Value protocol( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		obj->eventMessage->data.request.protocol = StrDup( *protocol );
	}
	obj->eventMessage->data.request.accepted = 1;
}

void wssObject::reject( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( !obj->eventMessage ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Reject cannot be used outside of connection callback.", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	obj->eventMessage->data.request.accepted = 0;
}

void wssObject::getReadyState( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
	args.GetReturnValue().Set( Integer::New( args.GetIsolate(), (int)obj->readyState ) );
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
	readyState = wsReadyStates::OPEN;
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
	}
}

void wssiObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.IsConstructCall() ) {
		wssiObject *obj = new wssiObject();
		obj->isolate = isolate;
		class constructorSet *c = getConstructors(isolate);
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
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), 0, argv ).ToLocalChecked() );
		delete[] argv;
	}
}

void wssiObject::ping( const v8::FunctionCallbackInfo<Value>& args ) {
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
	WebSocketPing( obj->pc, 0 );
}

void wssiObject::on( const FunctionCallbackInfo<Value>& args){
	Isolate* isolate = args.GetIsolate();

	if( args.Length() == 2 ) {
		wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
		String::Utf8Value event( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		Local<Function> cb = Local<Function>::Cast( args[1] );
		if(  StrCmp( *event, "message" ) == 0 ) {
			obj->messageCallback.Reset( isolate, cb);
		} else if(  StrCmp( *event, "error" ) == 0 ) {
			obj->errorCallback.Reset(isolate,cb);
		} else if(  StrCmp( *event, "close" ) == 0 ){
			obj->closeCallback.Reset(isolate,cb);
		}
	}
}

void wssiObject::onmessage( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		obj->messageCallback.Reset( isolate, cb );
	}
}

void wssiObject::onclose( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		obj->closeCallback.Reset( isolate, cb );
	}
}

void wssiObject::close( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
	if( obj->pc && !obj->closed )
		WebSocketClose( obj->pc, 1000, NULL );
}

void wssiObject::write( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
	if( !obj->pc ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Connection has already been closed.", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	if( args[0]->IsTypedArray() ) {
		Local<TypedArray> ta = Local<TypedArray>::Cast( args[0] );
		Local<ArrayBuffer> ab = ta->Buffer();
		WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
	} else if( args[0]->IsUint8Array() ) {
		Local<Uint8Array> body = args[0].As<Uint8Array>();
		Local<ArrayBuffer> ab = body->Buffer();
		WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
	} else if( args[0]->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
		WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
	}
	else if( args[0]->IsString() ) {
		String::Utf8Value buf( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		WebSocketSendText( obj->pc, *buf, buf.length() );
	}
	else {
		lprintf( "Unhandled message format" );
	}
}

void wssiObject::getReadyState( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
	args.GetReturnValue().Set( Integer::New( args.GetIsolate(), (int)obj->readyState ) );
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
	lprintf( "Send Open Request" );
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
	EnqueLink( &wsc->eventQueue, pevt );
	lprintf( "Send Close Request" );
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
	lprintf( "Send Error Request" );
	uv_async_send( &wsc->async );
}

static void webSockClientEvent( PCLIENT pc, uintptr_t psv, LOGICAL type, CPOINTER buffer, size_t msglen ) {
	wscObject *wsc = (wscObject*)psv;

	struct wscEvent *pevt = GetWscEvent();
	(*pevt).eventType = WS_EVENT_READ;
	(*pevt).buf = NewArray( uint8_t, msglen );
	memcpy( (*pevt).buf, buffer, msglen );
	(*pevt).buflen = msglen;
	(*pevt).binary = type;
	(*pevt)._this = wsc;
	EnqueLink( &wsc->eventQueue, pevt );
	lprintf( "Send Client Read Request" );
	uv_async_send( &wsc->async );
}

wscObject::wscObject( wscOptions *opts ) {
	eventQueue = CreateLinkQueue();
	readyState = INITIALIZING;
	closed = 0;
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
				throw "Error initializing SSL connection (bad key or passphrase?)";
			}
		}
		WebSocketConnect( pc );
		readyState = CONNECTING;
	}
}

wscObject::~wscObject() {
	//lprintf( "Destruct wscObject" );
	if( !closed ) {
		//lprintf( "destruct, try to generate WebSockClose" );
		RemoveClient( pc );
	}
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
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must specify url and optionally protocols or options for client.", v8::NewStringType::kNormal ).ToLocalChecked() ) );
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
		wscOpts.ssl = FALSE;
		wscOpts.key = NULL;
		wscOpts.key_len = 0;
		wscOpts.pass = NULL;
		wscOpts.pass_len = 0;
		wscOpts.root_cert = NULL;
		wscOpts.protocol = NULL;

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
		try {
			obj = new wscObject( &wscOpts );
			obj->isolate = isolate;
			class constructorSet *c = getConstructors(isolate);
			uv_async_init( c->loop, &obj->async, wscAsyncMsg );
			obj->async.data = obj;

			obj->_this.Reset( isolate, _this );
			obj->Wrap( _this );
		}
		catch( const char *ex1 ) {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( ex1 ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		}
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
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete[] argv;
	}
}

void wscObject::onOpen( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		obj->openCallback.Reset( isolate, cb );
	}
}

void wscObject::onMessage( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		obj->messageCallback.Reset( isolate, cb );
	}
}

void wscObject::onClose( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		obj->closeCallback.Reset( isolate, cb );
	}
}

void wscObject::onError( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Local<Function>::Cast( args[0] );
		obj->errorCallback.Reset( isolate, cb );
	}
}


void wscObject::getOnOpen( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	args.GetReturnValue().Set( obj->openCallback.Get( isolate ) );
}

void wscObject::getOnMessage( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	args.GetReturnValue().Set( obj->messageCallback.Get( isolate ) );
}

void wscObject::getOnClose( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	args.GetReturnValue().Set( obj->closeCallback.Get( isolate ) );
}

void wscObject::getOnError( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	args.GetReturnValue().Set( obj->errorCallback.Get( isolate ) );
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
				obj->openCallback.Reset( isolate, cb );
			}
		} else if(  StrCmp( *event, "message" ) == 0 ) {
			obj->messageCallback.Reset(isolate,cb);
		} else if(  StrCmp( *event, "error" ) == 0 ) {
			obj->errorCallback.Reset(isolate,cb);
		} else if(  StrCmp( *event, "close" ) == 0 ) {
			obj->closeCallback.Reset(isolate,cb);
		} else
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( "Event name specified is not supported or known." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
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
							String::NewFromUtf8( isolate, TranslateText( "SyntaxError (text reason too long)" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
						return;
					}
					if( obj->pc ) {
						WebSocketClose( obj->pc, 1000, NULL );
					}
				}
			}
			else {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "InvalidAccessError" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
				return;
			}
		}
		else {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( "InvalidAccessError" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
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
		WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
	} else if( args[0]->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
		WebSocketSendBinary( obj->pc, (const uint8_t*)ab->GetContents().Data(), ab->ByteLength() );
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
	args.GetReturnValue().Set( Integer::New( args.GetIsolate(), (int)obj->readyState ) );
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
	method = NULL;
	ca = NULL;
	path = NULL;
	rejestUnauthorized = false;
	firstDispatchDone = false;
	dataDispatch = false;
	endDispatch = false;
	finished = false;
	waiter = NULL;
	result = NULL;
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
	httpRequestObject *http = ObjectWrap::Unwrap<httpRequestObject>( args.This() );

	args.GetReturnValue().Set( args.This() );

}

void httpRequestObject::on( const FunctionCallbackInfo<Value>& args ) {
	String::Utf8Value eventName( USE_ISOLATE( args.GetIsolate() ) args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
	httpRequestObject *http = ObjectWrap::Unwrap<httpRequestObject>( args.This() );

	Local<Function> cb = Local<Function>::Cast( args[1] );
	/* abort, connect, continue, response, socket, timeout, upgrade, */
	if( StrCmp( *eventName, "error" ) == 0 ) {

	}
	args.GetReturnValue().Set( args.This() );
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

	if( options->Has( context, optName = strings->methodString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value value( USE_ISOLATE( isolate ) GETV( options, optName ) );
		httpRequest->method = StrDup( *value );
	}

	if( secure && options->Has( context, optName = strings->caString->Get( isolate ) ).ToChecked() ) {
		if( GETV( options, optName )->IsString() ) {
			String::Utf8Value value( USE_ISOLATE( isolate ) GETV( options, optName ) );
			httpRequest->ca = StrDup( *value );
		}
	}

	if( options->Has( context, optName = strings->rejectUnauthorizedString->Get( isolate ) ).ToChecked() ) {
		httpRequest->rejestUnauthorized = GETV( options, optName )->ToBoolean( isolate )->Value();
	}

	if( options->Has( context, optName = strings->pathString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value value( USE_ISOLATE( isolate ) GETV( options, optName ) );
		httpRequest->path = StrDup( *value );
	}

	Local<Object> result = Object::New( isolate );
	NetworkWait( NULL, 256, 2 );  // 1GB memory

	{
		PVARTEXT pvtAddress = VarTextCreate();
		vtprintf( pvtAddress, "%s:%d", httpRequest->hostname, httpRequest->port );

		PTEXT address = VarTextPeek( pvtAddress );
		PTEXT url = SegCreateFromText( httpRequest->path );

		HTTPState state = NULL;
		int retries;
		for( retries = 0; !state && retries < 3; retries++ ) {
			//lprintf( "request: %s  %s", GetText( address ), GetText( url ) );
			if( httpRequest->ssl )
				state = GetHttpsQuery( address, url, httpRequest->ca );
			else
				state = GetHttpQuery( address, url );
		}
		if( !state ) {
			SET( result, "error",
				state ? String::NewFromUtf8( isolate, "No Content", v8::NewStringType::kNormal ).ToLocalChecked() : String::NewFromUtf8( isolate, "Connect Error", v8::NewStringType::kNormal ).ToLocalChecked() );
			args.GetReturnValue().Set( result );
			return;
		}
		PTEXT content = GetHttpContent(state);
		if( state && content )
		{
			SET( result, "content"
				, String::NewFromUtf8( isolate, GetText( content ), v8::NewStringType::kNormal ).ToLocalChecked() );
			SET( result, "statusCode"
				, Integer::New( isolate, GetHttpResponseCode( state ) ) );
			SET( result, "status"
				, String::NewFromUtf8( isolate, GetText( GetHttpResponce( state ) ), v8::NewStringType::kNormal ).ToLocalChecked() );
			Local<Array> arr = Array::New( isolate );
			PLIST headers = GetHttpHeaderFields( state );
			INDEX idx;
			struct HttpField *header;
			//headers
			LIST_FORALL( headers, idx, struct HttpField *, header ) {
				SET( arr, (const char*)GetText( header->name )
					, String::NewFromUtf8( isolate, (const char*)GetText( header->value )
						, NewStringType::kNormal, (int)GetTextSize( header->value ) ).ToLocalChecked() );
			}
			SET( result, "headers", arr );

			DestroyHttpState( state );
		}
		else
		{
			SET( result, "error",
				state?String::NewFromUtf8( isolate, "No Content", v8::NewStringType::kNormal ).ToLocalChecked() 
					:String::NewFromUtf8( isolate, "Connect Error", v8::NewStringType::kNormal ).ToLocalChecked() );

		}

		VarTextDestroy( &pvtAddress );
		LineRelease( url );
	}


	args.GetReturnValue().Set( result );
}

void httpRequestObject::get( const FunctionCallbackInfo<Value>& args ) {
	getRequest( args, false );
}
void httpRequestObject::gets( const FunctionCallbackInfo<Value>& args ) {
	getRequest( args, true );
}
