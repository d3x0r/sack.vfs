
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
	Eternal<String> *rejectUnauthorizedString;
	Eternal<String> *pathString;
	Eternal<String> *methodString;
};

static PLIST strings;

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
};

struct pendingSend {
	LOGICAL binary;
	POINTER buffer;
	size_t buflen;
};

struct wssEvent {
	enum wsEvents eventType;
	class wssObject *_this;
	const char *protocol;
	const char *resource;
	int accepted;
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

static struct local {
	int data;
	uv_loop_t* loop;
	int waiting;
	PTHREAD jsThread;
	CRITICALSECTION csWssEvents;
	PWSS_EVENTSET wssEvents;
	CRITICALSECTION csWssiEvents;
	PWSSI_EVENTSET wssiEvents;
	CRITICALSECTION csWscEvents;
	PWSC_EVENTSET wscEvents;
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
		check->readyStateString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "readyState" ) );
		check->portString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "port" ) );
		check->urlString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "url" ) );
		check->localPortString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "localPort" ) );
		check->remotePortString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "remotePort" ) );
		check->addressString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "address" ) );
		check->localAddrString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "localAddress" ) );
		check->remoteAddrString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "remoteAddress" ) );
		check->localFamilyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "localFamily" ) );
		check->remoteFamilyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "remoteFamily" ) );
		check->headerString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "headers" ) );
		check->certString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "cert" ) );
		check->keyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "key" ) );
		check->pemString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "pem" ) );
		check->passString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "passphrase" ) );
		check->deflateString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "perMessageDeflate" ) );
		check->deflateAllowString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "perMessageDeflateAllow" ) );
		check->caString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "ca" ) );
		check->v4String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "IPv4" ) );
		check->v6String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "IPv6" ) );
		check->vUString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "unknown" ) );
		check->connectionString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "connection" ) );
		check->maskingString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "masking" ) );
		check->bytesReadString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "bytesRead" ) );
		check->bytesWrittenString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "bytesWritten" ) );
		check->requestString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "request" ) );
		check->bufferedAmountString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "bufferedAmount" ) );

		check->hostnameString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "hostname" ) );
		check->pathString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "path" ) );
		check->methodString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "method" ) );
		check->rejectUnauthorizedString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "rejectUnauthorized" ) );
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
	PLIST opening;
	PLIST requests;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	static Persistent<Function> constructor;
	static Persistent<FunctionTemplate> tpl;
	Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> acceptCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> requestCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorCloseCallback; //
	struct wssEvent *eventMessage;
	bool ssl;
	enum wsReadyStates readyState;
public:

	wssObject( struct wssOptions *opts );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	//static void onMessage( const v8::FunctionCallbackInfo<Value>& args );
	static void onConnect( const v8::FunctionCallbackInfo<Value>& args );
	static void onAccept( const v8::FunctionCallbackInfo<Value>& args );
	static void onRequest( const v8::FunctionCallbackInfo<Value>& args );
	static void onError( const v8::FunctionCallbackInfo<Value>& args );
	static void accept( const v8::FunctionCallbackInfo<Value>& args );
	static void reject( const v8::FunctionCallbackInfo<Value>& args );
	static void getReadyState( const FunctionCallbackInfo<Value>& args );

	~wssObject();
};

class httpRequestObject : public node::ObjectWrap {
public:
	PCLIENT pc;
	static Persistent<Function> constructor;
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
	static Persistent<Function> constructor;
	PVARTEXT pvtResult;
	bool ssl;

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
public:
	static Persistent<Function> constructor;
	static Persistent<FunctionTemplate> tpl;
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

	~wscObject();
};

// web sock server instance Object  (a connection from a remote)
class wssiObject : public node::ObjectWrap {
public:
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
	//struct wssiEvent *eventMessage;
	PCLIENT pc;
	LOGICAL closed;
	const char *protocolResponse;
	enum wsReadyStates readyState;
	wssObject *server;
public:
	static Persistent<Function> constructor;
	static Persistent<FunctionTemplate> tpl;
	//Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> messageCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; //

public:

	wssiObject( );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void write( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void onmessage( const v8::FunctionCallbackInfo<Value>& args );
	static void onclose( const v8::FunctionCallbackInfo<Value>& args );
	static void getReadyState( const FunctionCallbackInfo<Value>& args );

	~wssiObject();
};

Persistent<Function> httpObject::constructor;
Persistent<Function> httpRequestObject::constructor;
Persistent<Function> wssObject::constructor;
Persistent<Function> wssiObject::constructor;
Persistent<Function> wscObject::constructor;
Persistent<FunctionTemplate> wssObject::tpl;
Persistent<FunctionTemplate> wssiObject::tpl;
Persistent<FunctionTemplate> wscObject::tpl;

static WSS_EVENT *GetWssEvent() {
	EnterCriticalSec( &l.csWssEvents );
	WSS_EVENT *evt = GetFromSet( WSS_EVENT, &l.wssEvents );
	LeaveCriticalSec( &l.csWssEvents );
	return evt;
}

static WSSI_EVENT *GetWssiEvent() {
	EnterCriticalSec( &l.csWssiEvents );
	WSSI_EVENT *evt = GetFromSet( WSSI_EVENT, &l.wssiEvents );
	LeaveCriticalSec( &l.csWssiEvents );
	return evt;
}

static WSC_EVENT *GetWscEvent() {
	EnterCriticalSec( &l.csWscEvents );
	WSC_EVENT *evt = GetFromSet( WSC_EVENT, &l.wscEvents );
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

static Local<Object> makeSocket( Isolate *isolate, PCLIENT pc ) {
	PLIST headers = GetWebSocketHeaders( pc );
	PTEXT resource = GetWebSocketResource( pc );
	SOCKADDR *remoteAddress = (SOCKADDR *)GetNetworkLong( pc, GNL_REMOTE_ADDRESS );
	SOCKADDR *localAddress = (SOCKADDR *)GetNetworkLong( pc, GNL_LOCAL_ADDRESS );
	Local<String> remote = String::NewFromUtf8( isolate, GetAddrName( remoteAddress ) );
	Local<String> local = String::NewFromUtf8( isolate, GetAddrName( localAddress ) );
	Local<Object> result = Object::New( isolate );
	Local<Object> arr = Object::New( isolate );
	INDEX idx;
	struct HttpField *header;
	LIST_FORALL( headers, idx, struct HttpField *, header ) {
		arr->Set( String::NewFromUtf8( isolate, (const char*)GetText( header->name ), NewStringType::kNormal, (int)GetTextSize( header->name ) ).ToLocalChecked()
			, String::NewFromUtf8( isolate, (const char*)GetText( header->value ), NewStringType::kNormal, (int)GetTextSize( header->value ) ).ToLocalChecked() );
	}
	optionStrings *strings = getStrings( isolate );
	result->Set( strings->headerString->Get( isolate ), arr );
	result->Set( strings->remoteFamilyString->Get( isolate )
			, (remoteAddress->sa_family == AF_INET) ? strings->v4String->Get( isolate ) :
				(remoteAddress->sa_family == AF_INET6) ? strings->v6String->Get( isolate ) : strings->vUString->Get( isolate )
		);
	result->Set( strings->remoteAddrString->Get( isolate ), remote );
	result->Set( strings->remotePortString->Get( isolate ), Integer::New( isolate, (int32_t)GetNetworkLong( pc, GNL_PORT ) ) );
	result->Set( strings->localFamilyString->Get( isolate )
			, (localAddress->sa_family == AF_INET)?strings->v4String->Get(isolate):
				(localAddress->sa_family == AF_INET6) ? strings->v6String->Get( isolate ) : strings->vUString->Get(isolate)
		);
	result->Set( strings->localAddrString->Get( isolate ), local );
	result->Set( strings->localPortString->Get( isolate ), Integer::New( isolate, (int32_t)GetNetworkLong( pc, GNL_MYPORT ) ) );
	return result;
}

static Local<Object> makeRequest( Isolate *isolate, struct optionStrings *strings, PCLIENT pc ) {
	// .url
	// .socket
	Local<Object> req = Object::New( isolate );
	Local<Object> socket;
	req->Set( strings->connectionString->Get( isolate ), socket = makeSocket( isolate, pc ) );
	req->Set( strings->headerString->Get( isolate ), socket->Get( strings->headerString->Get( isolate ) ) );
	if( !GetText( GetHttpRequest( GetWebSocketHttpState( pc ) ) ) )
		DebugBreak();
	req->Set( strings->urlString->Get( isolate )
		, String::NewFromUtf8( isolate
			, GetText( GetHttpRequest( GetWebSocketHttpState( pc ) ) ) ) );
	return req;
}

static void wssAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	wssObject* myself = (wssObject*)handle->data;

	HandleScope scope(isolate);
	{
		struct wssEvent *eventMessage;
		while( eventMessage = (struct wssEvent *)DequeLink( &myself->eventQueue ) ) {
			Local<Value> argv[3];
			myself->eventMessage = eventMessage;
			if( eventMessage->eventType == WS_EVENT_REQUEST ) {
				if( !myself->requestCallback.IsEmpty() ) {
					Local<Function> cons = Local<Function>::New( isolate, httpObject::constructor );
					MaybeLocal<Object> _http = cons->NewInstance( isolate->GetCurrentContext(), 0, argv );
					if( _http.IsEmpty() ) {
						isolate->ThrowException( Exception::Error(
							String::NewFromUtf8( isolate, TranslateText( "Internal Error request instance." ) ) ) );
						break;
					}
					Local<Object> http = _http.ToLocalChecked();
					struct optionStrings *strings = getStrings( isolate );
					http->Set( strings->connectionString->Get( isolate ), makeSocket( isolate, eventMessage->pc ) );

					httpObject *httpInternal = httpObject::Unwrap<httpObject>( http );
					AddLink( &myself->requests, httpInternal );
					httpInternal->ssl = myself->ssl;
					httpInternal->pc = eventMessage->pc;
               //lprintf( "New request..." );
					argv[0] = makeRequest( isolate, strings, eventMessage->pc );
					argv[1] = http;
					Local<Function> cb = Local<Function>::New( isolate, myself->requestCallback );
					cb->Call( eventMessage->_this->_this.Get( isolate ), 2, argv );
				}
			}
			else if( eventMessage->eventType == WS_EVENT_ACCEPT ) {

				Local<Function> cons = Local<Function>::New( isolate, wssiObject::constructor );
				MaybeLocal<Object> _wssi = cons->NewInstance( isolate->GetCurrentContext(), 0, argv );
				if( _wssi.IsEmpty() ) {
					isolate->ThrowException( Exception::Error(
								String::NewFromUtf8( isolate, TranslateText( "Internal Error creating client connection instance." ) ) ) );
					lprintf( "Internal error. I wonder what exception looks like" );
					break;
				}
				Local<Object> wssi = _wssi.ToLocalChecked();
				struct optionStrings *strings = getStrings( isolate );
				Local<Object> socket;
				wssiObject *wssiInternal = wssiObject::Unwrap<wssiObject>( wssi );
				wssiInternal->pc = eventMessage->pc;
				wssiInternal->server = myself;
				AddLink( &myself->opening, wssiInternal );

				wssi->Set( strings->connectionString->Get(isolate), socket = makeSocket( isolate, eventMessage->pc ) );
				wssi->Set( strings->headerString->Get( isolate ), socket->Get( strings->headerString->Get( isolate ) ) );
				wssi->Set( strings->urlString->Get( isolate )
					, String::NewFromUtf8( isolate
						, GetText( GetHttpRequest( GetWebSocketHttpState( wssiInternal->pc ) ) ) ) );

				argv[0] = wssi;

				if( !myself->acceptCallback.IsEmpty() ) {
					Local<Function> cb = myself->acceptCallback.Get( isolate );
					cb->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
				}
				else
					eventMessage->accepted = 1;
				eventMessage->result = wssiInternal;
			}
			else if( eventMessage->eventType == WS_EVENT_ERROR_CLOSE ) {
				Local<Object> closingSock = makeSocket( isolate, eventMessage->pc );
				if( !myself->errorCloseCallback.IsEmpty() ) {
					Local<Function> cb = myself->errorCloseCallback.Get( isolate );
					argv[0] = closingSock;
					cb->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
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
				DropWssEvent( eventMessage );
				DeleteLinkQueue( &myself->eventQueue );
				return;
			}

			myself->eventMessage = NULL;
			eventMessage->done = TRUE;
			WakeThread( eventMessage->waiter );
		}
	}
}

static void wssiAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	wssiObject* myself = (wssiObject*)handle->data;

	HandleScope scope(isolate);

	{
		struct wssiEvent *eventMessage;
		while( eventMessage = (struct wssiEvent *)DequeLink( &myself->eventQueue ) ) {
			Local<Value> argv[1];
			Local<ArrayBuffer> ab;
			switch( eventMessage->eventType ) {
			case WS_EVENT_OPEN:
				if( !myself->server->openCallback.IsEmpty() ) {
					Local<Function> cb = Local<Function>::New( isolate, myself->server->openCallback );
					argv[0] = myself->_this.Get( isolate );
					cb->Call( myself->_this.Get( isolate ), 1, argv );
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

						myself->messageCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
					}
					else {
						MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
						argv[0] = buf.ToLocalChecked();
						myself->messageCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
					}
					Deallocate( POINTER, eventMessage->buf );
				}
				break;
			case WS_EVENT_CLOSE:
				if( !myself->closeCallback.IsEmpty() ) {
					myself->closeCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 0, argv );
				}
				uv_close( (uv_handle_t*)&myself->async, uv_closed_wssi );
				DropWssiEvent( eventMessage );
				DeleteLinkQueue( &myself->eventQueue );
				myself->closed = 1;
				myself->readyState = CLOSED;
				continue;
				break;
			case WS_EVENT_ERROR:
				myself->errorCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 0, argv );
				break;
			}
			DropWssiEvent( eventMessage );
		}
	}
	//lprintf( "done calling message notice." );
}

static void wscAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	wscObject* wsc = (wscObject*)handle->data;
	wscEvent *eventMessage;
	HandleScope scope(isolate);

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
					wsc->_this.Get( isolate )->Set( strings->connectionString->Get( isolate ), makeSocket( isolate, wsc->pc ) );
					cb->Call( eventMessage->_this->_this.Get( isolate ), 0, argv );
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

					wsc->messageCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
				}
				else {
					MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
					argv[0] = buf.ToLocalChecked();
					wsc->messageCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
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
					wsc->errorCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 2, argv );
				}
				break;
			case WS_EVENT_CLOSE:
				cb = Local<Function>::New( isolate, wsc->closeCallback );
				if( !cb.IsEmpty() )
					cb->Call( eventMessage->_this->_this.Get( isolate ), 0, argv );
				uv_close( (uv_handle_t*)&wsc->async, uv_closed_wsc );
				DeleteLinkQueue( &wsc->eventQueue );
				wsc->readyState = CLOSED;
				break;
			}
			DropWscEvent( eventMessage );
		}
	}
}

int accepted = 0;

void InitWebSocket( Isolate *isolate, Handle<Object> exports ){

	if( !l.loop )
		l.loop = uv_default_loop();
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
	{
		Local<FunctionTemplate> httpTemplate;
		httpTemplate = FunctionTemplate::New( isolate, httpObject::New );
		httpTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.http.requestHandler" ) );
		httpTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( httpTemplate, "writeHead", httpObject::writeHead );
		NODE_SET_PROTOTYPE_METHOD( httpTemplate, "end", httpObject::end );
		httpTemplate->ReadOnlyPrototype();

		httpObject::constructor.Reset( isolate, httpTemplate->GetFunction() );

		// this is not exposed as a class that javascript can create.
		//SET_READONLY( o, "R", wscTemplate->GetFunction() );
	}
	{
		Local<FunctionTemplate> httpRequestTemplate;
		httpRequestTemplate = FunctionTemplate::New( isolate, httpRequestObject::New );
		httpRequestTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.HTTP[S]" ) );
		httpRequestTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( httpRequestTemplate, "on", httpRequestObject::on );
		NODE_SET_PROTOTYPE_METHOD( httpRequestTemplate, "wait", httpRequestObject::wait );
		httpRequestTemplate->ReadOnlyPrototype();
		httpRequestObject::constructor.Reset( isolate, httpRequestTemplate->GetFunction() );

		Local<Object> oHttps = Object::New( isolate );
		SET_READONLY( exports, "HTTPS", oHttps );
		SET_READONLY_METHOD( oHttps, "get", httpRequestObject::gets );

		Local<Object> oHttp = Object::New( isolate );
		SET_READONLY( exports, "HTTP", oHttp );
		SET_READONLY_METHOD( oHttp, "get", httpRequestObject::get );

		// this is not exposed as a class that javascript can create.
		//SET_READONLY( o, "R", wscTemplate->GetFunction() );
	}
	{
		Local<FunctionTemplate> wssTemplate;
		wssTemplate = FunctionTemplate::New( isolate, wssObject::New );
		wssTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.ws.server" ) );
		wssTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "close", wssObject::close );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "on", wssObject::on );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onconnect", wssObject::onConnect );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onaccept", wssObject::onAccept );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onrequest", wssObject::onRequest );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onerror", wssObject::onError );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "accept", wssObject::accept );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "reject", wssObject::reject );

		wssTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "readyState" )
			, FunctionTemplate::New( isolate, wssObject::getReadyState )
			, Local<FunctionTemplate>() );
		//wssTemplate->SetNativeDataProperty( String::NewFromUtf8( isolate, "readyState" )
		//	, wssObject::getReadyState
		//	, NULL );
		wssTemplate->ReadOnlyPrototype();
		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onmessage", wsObject::on );
		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "on", wsObject::on );

		wssObject::constructor.Reset( isolate, wssTemplate->GetFunction() );
		wssObject::tpl.Reset( isolate, wssTemplate );

		SET_READONLY( o, "Server", wssTemplate->GetFunction() );
	}

	{
		Local<FunctionTemplate> wscTemplate;
		wscTemplate = FunctionTemplate::New( isolate, wscObject::New );
		wscTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.ws.client" ) );
		wscTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( wscTemplate, "close", wscObject::close );
		NODE_SET_PROTOTYPE_METHOD( wscTemplate, "send", wscObject::write );
		NODE_SET_PROTOTYPE_METHOD( wscTemplate, "on", wscObject::on );
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "readyState" )
			, FunctionTemplate::New( isolate, wscObject::getReadyState )
			, Local<FunctionTemplate>() );
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "onopen" )
			, FunctionTemplate::New( isolate, wscObject::getOnOpen )
			, FunctionTemplate::New( isolate, wscObject::onOpen )
		);
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "onmessage" )
			, FunctionTemplate::New( isolate, wscObject::getOnMessage )
			, FunctionTemplate::New( isolate, wscObject::onMessage )
		);
		wscTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "onclose" )
			, FunctionTemplate::New( isolate, wscObject::getOnClose )
			, FunctionTemplate::New( isolate, wscObject::onClose )
		);
		wscTemplate->ReadOnlyPrototype();

		wscObject::constructor.Reset( isolate, wscTemplate->GetFunction() );
		wscObject::tpl.Reset( isolate, wscTemplate );

		SET_READONLY( o, "Client", wscTemplate->GetFunction() );
	}

	{
		Local<FunctionTemplate> wssiTemplate;
		wssiTemplate = FunctionTemplate::New( isolate, wssiObject::New );
		wssiTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.ws.connection" ) );
		wssiTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "send", wssiObject::write );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "close", wssiObject::close );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "on", wssiObject::on );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "onmessage", wssiObject::onmessage );
		NODE_SET_PROTOTYPE_METHOD( wssiTemplate, "onclose", wssiObject::onclose );
		wssiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "readyState" )
			, FunctionTemplate::New( isolate, wssiObject::getReadyState )
			, Local<FunctionTemplate>() );
		wssiTemplate->ReadOnlyPrototype();

		wssiObject::constructor.Reset( isolate, wssiTemplate->GetFunction() );
		wssiObject::tpl.Reset( isolate, wssiTemplate );
		//SET_READONLY( o, "Client", wscTemplate->GetFunction() );
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
	if( wssi ) {
		struct wssiEvent *pevt = GetWssiEvent();
		if( wssi->protocolResponse ) {
			Deallocate( const char *, wssi->protocolResponse );
			wssi->protocolResponse = NULL;
		}
		(*pevt).eventType = WS_EVENT_OPEN;
		(*pevt)._this = wssi;
		EnqueLink( &wssi->eventQueue, pevt );
		uv_async_send( &wssi->async );
		return (uintptr_t)wssi;
	}
	lprintf( "FAILED TO HAVE WSSI to open." );
	return 0;
}

static void webSockServerCloseEvent( wssObject *wss ) {
	struct wssEvent *pevt = GetWssEvent();
	//lprintf( "Server Websocket closed; post to javascript %p", wss );
	(*pevt).eventType = WS_EVENT_CLOSE;
	(*pevt)._this = wss;
	EnqueLink( &wss->eventQueue, pevt );
	uv_async_send( &wss->async );
}

static void webSockServerClosed( PCLIENT pc, uintptr_t psv, int code, const char *reason )
{
	wssiObject *wssi = (wssiObject*)psv;
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
		//lprintf( "(close before accept)Illegal connection" );
		SOCKADDR *ip = (SOCKADDR*)GetNetworkLong( pc, GNL_REMOTE_ADDRESS );
		uintptr_t psvServer = WebSocketGetServerData( pc );
		wssObject *wss = (wssObject*)psvServer;
		if( wss ) {	
			httpObject *req;
			INDEX idx;
			// close on wssObjectEvent; may have served HTTP requests
			LIST_FORALL( wss->requests, idx, httpObject *, req ) {
				if( req->pc == pc ) {
					SetLink( &wss->requests, idx, NULL );
					return;
				}
			}
		}
		lprintf( "(close before accept)Illegal connection" );
		//DumpAddr( "IP", ip );
		struct wssEvent *pevt = GetWssEvent();
		if( ( (*pevt).waiter = MakeThread() ) != l.jsThread )  {
			//lprintf( "Server Websocket closed; post to javascript %p", wss );
			(*pevt).eventType = WS_EVENT_ERROR_CLOSE;
			(*pevt).waiter = MakeThread();

			(*pevt).pc = pc;
			(*pevt)._this = wss;
			EnqueLink( &wss->eventQueue, pevt );
			uv_async_send( &wss->async );

			while( !(*pevt).done )
				Wait();
		} else {
			Isolate *isolate = Isolate::GetCurrent();
			Local<Object> closingSock = makeSocket( isolate, pc );
			if( !wss->errorCloseCallback.IsEmpty() ) {
				Local<Function> cb = wss->errorCloseCallback.Get( isolate );
				Local<Value> argv[1];
				argv[1] = closingSock;
				cb->Call( wss->_this.Get( isolate ), 1, argv );
			}
			DropWssEvent( pevt );
		}
	}
}

static void webSockServerError( PCLIENT pc, uintptr_t psv, int error ){
	wssiObject *wssi = (wssiObject*)psv;
	struct wssiEvent *pevt = GetWssiEvent();
	(*pevt).eventType = WS_EVENT_ERROR;
	(*pevt)._this = wssi;
	EnqueLink( &wssi->eventQueue, pevt );
	uv_async_send( &wssi->async );
}

static void webSockServerEvent( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, size_t msglen ){
	//lprintf( "Received:%p %d", buffer, binary );
	wssiObject *wssi = (wssiObject*)psv;
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
	evt.protocol = protocols;
	evt.resource = resource;
	evt.pc = pc;
	evt.accepted = 0;
	evt.eventType = WS_EVENT_ACCEPT;
	evt.done = FALSE;
	evt.waiter = MakeThread();
	evt._this = wss;
	EnqueLink( &wss->eventQueue, &evt );
	uv_async_send( &wss->async );

	while( !evt.done )
		Wait();
	if( evt.protocol != protocols )
		evt.result->protocolResponse = evt.protocol;
	(*protocolReply) = (char*)evt.protocol;
	return (uintptr_t)evt.accepted;
}

httpObject::httpObject() {
	pvtResult = VarTextCreate();
}

httpObject::~httpObject() {

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
	httpObject* obj = Unwrap<httpObject>( args.This() );
	PTEXT tmp;
	tmp = VarTextPeek( obj->pvtResult );
	if( tmp && GetTextSize( tmp ) ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Headers have already been set; cannot change resulting status or headers" ) ) );
		return;
	}
	int status = 404;
	Local<Object> headers;
	if( args.Length() > 0 ) {
		status = args[0]->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
	}
	HTTPState http = GetWebSocketHttpState( obj->pc );
	int vers = GetHttpVersion( http );
	vtprintf( obj->pvtResult, WIDE( "HTTP/%d.%d %d %s\r\n" ), vers/100, vers%100, status, "OK" );

	if( args.Length() > 1 ) {
		headers = args[1]->ToObject();
		Local<Array> keys = headers->GetPropertyNames();
		int len = keys->Length();
		int i;
		for( i = 0; i < len; i++ ) {
			Local<Value> key = keys->Get( i );
			Local<Value> val = headers->Get( key );
			String::Utf8Value keyname( USE_ISOLATE(isolate) key );
			String::Utf8Value keyval( USE_ISOLATE( isolate ) val );
			vtprintf( obj->pvtResult, WIDE( "%s:%s\r\n" ), *keyname, *keyval );
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
			vtprintf( obj->pvtResult, WIDE( "\r\n" ) );

			vtprintf( obj->pvtResult, "%*.*s", body.length(), body.length(), *body );
		}
		else if( args[0]->IsUint8Array() ) {
			Local<Uint8Array> body = args[0].As<Uint8Array>();
			Local<ArrayBuffer> bodybuf = body->Buffer();
			vtprintf( obj->pvtResult, "content-length:%d\r\n", body->ByteLength() );
			vtprintf( obj->pvtResult, WIDE( "\r\n" ) );
			VarTextAddData( obj->pvtResult, (CTEXTSTR)bodybuf->GetContents().Data(), bodybuf->ByteLength() );
		}
		else if( args[0]->IsArrayBuffer() ) {
			Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
			vtprintf( obj->pvtResult, "content-length:%d\r\n", ab->ByteLength() );
			vtprintf( obj->pvtResult, WIDE( "\r\n" ) );
			VarTextAddData( obj->pvtResult, (CTEXTSTR)ab->GetContents().Data(), ab->ByteLength() );
		} else if( args[0]->IsObject() ) {
			Local<FunctionTemplate> wrapper_tpl = FileObject::tpl.Get( isolate );
			if( (wrapper_tpl->HasInstance( args[0] )) ) {
				FileObject *file = FileObject::Unwrap<FileObject>( args[0]->ToObject() );
				lprintf( "Incomplete; streaming file content to socket...." );
				doSend = false;
			}
		}
	}
	else
		vtprintf( obj->pvtResult, WIDE( "\r\n" ) );

	if( doSend ) {
		PTEXT buffer = VarTextPeek( obj->pvtResult );
		if( obj->ssl )
			ssl_Send( obj->pc, GetText( buffer ), GetTextSize( buffer ) );
		else
			SendTCP( obj->pc, GetText( buffer ), GetTextSize( buffer ) );
	}
	{
      if( include_close )
			RemoveClientEx( obj->pc, 0, 1 );
		else {
         //lprintf( "End a request..." );
			EndHttp( GetWebSocketHttpState( obj->pc ) );
		}
	}

	VarTextEmpty( obj->pvtResult );
}

static uintptr_t webSockHttpRequest( PCLIENT pc, uintptr_t psv ) {
	wssObject *wss = (wssObject*)psv;
	if( !wss->requestCallback.IsEmpty() ) {
		struct wssEvent *pevt = GetWssEvent();
		//lprintf( "posting request event to JS" );
		(*pevt).eventType = WS_EVENT_REQUEST;
		(*pevt). pc = pc;
		(*pevt)._this = wss;
		EnqueLink( &wss->eventQueue, pevt );
		uv_async_send( &wss->async );
	} else {
		RemoveClient( pc );
	}
	return 0;
}

wssObject::wssObject( struct wssOptions *opts ) {
	char tmp[256];
	int clearUrl = 0;
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

	pc = WebSocketCreate( opts->url, webSockServerOpen, webSockServerEvent, webSockServerClosed, webSockServerError, (uintptr_t)this );
	if( pc ) {
		SetSocketReusePort( pc, TRUE );
		SetSocketReuseAddress( pc, TRUE );
		uv_async_init( l.loop, &async, wssAsyncMsg );
		async.data = this;

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
			ssl_BeginServer( pc
				, opts->cert_chain, opts->cert_chain_len
				, opts->key, opts->key_len
				, opts->pass, opts->pass_len );
		}
		SetWebSocketAcceptCallback( pc, webSockServerAccept );
		eventQueue = CreateLinkQueue();
		readyState = LISTENING;
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

static void ParseWssOptions( struct wssOptions *wssOpts, Isolate *isolate, Local<Object> opts ) {

	Local<String> optName;
	struct optionStrings *strings = getStrings( isolate );
	wssOpts->cert_chain = NULL;
	wssOpts->cert_chain_len = 0;

	if( opts->Has( optName = strings->addressString->Get( isolate ) ) ) {
		String::Utf8Value address( USE_ISOLATE( isolate ) opts->Get( optName )->ToString() );
		wssOpts->address = StrDup( *address );
	}

	if( !opts->Has( optName = strings->portString->Get( isolate ) ) ) {
		wssOpts->port = 8080;
	}
	else {
		wssOpts->port = (int)opts->Get( optName )->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
	}
	if( opts->Has( optName = strings->certString->Get( isolate ) ) ) {
		String::Utf8Value cert( USE_ISOLATE( isolate ) opts->Get( optName )->ToString() );
		wssOpts->cert_chain = StrDup( *cert );
		wssOpts->cert_chain_len = cert.length();
	}
	if( opts->Has( optName = strings->caString->Get( isolate ) ) ) {
		String::Utf8Value ca( USE_ISOLATE( isolate ) opts->Get( optName )->ToString() );
		if( wssOpts->cert_chain ) {
			wssOpts->cert_chain = (char*)Reallocate( wssOpts->cert_chain, wssOpts->cert_chain_len + ca.length() + 1 );
			strcpy( wssOpts->cert_chain + wssOpts->cert_chain_len, *ca );
			wssOpts->cert_chain_len += ca.length();
		} else {
			wssOpts->cert_chain = StrDup( *ca );
			wssOpts->cert_chain_len = ca.length();
		}
	}

	if( !opts->Has( optName = strings->keyString->Get( isolate ) ) ) {
		wssOpts->key = NULL;
		wssOpts->key_len = 0;
	}
	else {
		String::Utf8Value cert( USE_ISOLATE( isolate ) opts->Get( optName )->ToString() );
		wssOpts->key = StrDup( *cert );
		wssOpts->key_len = cert.length();
	}
	if( !opts->Has( optName = strings->passString->Get( isolate ) ) ) {
		wssOpts->pass = NULL;
		wssOpts->pass_len = 0;
	}
	else {
		String::Utf8Value cert( USE_ISOLATE( isolate ) opts->Get( optName )->ToString() );
		wssOpts->pass = StrDup( *cert );
		wssOpts->pass_len = cert.length();
	}

	if( wssOpts->key || wssOpts->cert_chain ) {
		wssOpts->ssl = 1;
	}
	else
		wssOpts->ssl = 0;

	if( !opts->Has( optName = strings->deflateString->Get( isolate ) ) ) {
		wssOpts->deflate = false;
	}
	else
		wssOpts->deflate = (opts->Get( optName )->ToBoolean()->Value());
	if( !opts->Has( optName = strings->deflateAllowString->Get( isolate ) ) ) {
		wssOpts->deflate_allow = false;
	}
	else {
		wssOpts->deflate_allow = (opts->Get( optName )->ToBoolean()->Value());
		//lprintf( "deflate allow:%d", wssOpts->deflate_allow );
	}

	if( opts->Has( optName = strings->maskingString->Get( isolate ) ) ) {
		wssOpts->apply_masking = opts->Get( optName )->ToBoolean()->Value();
	}
	else
		wssOpts->apply_masking = false;

}

void wssObject::New(const FunctionCallbackInfo<Value>& args){
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must specify options for server." ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		struct wssOptions wssOpts;
		int argOfs = 0;
		wssOpts.url = NULL;
		wssOpts.port = 0;
		wssOpts.address = NULL;
		if( args[argOfs]->IsString() ) {
			String::Utf8Value url( USE_ISOLATE( isolate ) args[argOfs]->ToString() );
			wssOpts.url = StrDup( *url );
			argOfs++;
		}
		if( args[argOfs]->IsNumber() ) {
			wssOpts.port = (int)args[0]->IntegerValue();
			argOfs++;
		}
		if( args[argOfs]->IsObject() ) {
			Local<Object> opts = args[0]->ToObject();
			ParseWssOptions( &wssOpts, isolate, opts );
		}

		Local<Object> _this = args.This();
		wssObject* obj = new wssObject( &wssOpts );
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
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Failed to create listener." ) ) );
			return;
		}
		if( args.Length() > 1 && args[1]->IsFunction() ) {
			Handle<Function> arg0 = Handle<Function>::Cast( args[1] );
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

		Local<Function> cons = Local<Function>::New( isolate, constructor );
		MaybeLocal<Object> result = cons->NewInstance( isolate->GetCurrentContext(), argc, argv );
		delete[] argv;
		if( !result.IsEmpty() )
			args.GetReturnValue().Set( result.ToLocalChecked() );
	}
}

void wssObject::close( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
	obj->readyState = CLOSING;
	RemoveClient( obj->pc );
	webSockServerCloseEvent( obj );
}

void wssObject::on( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() == 2 ) {
		Isolate* isolate = args.GetIsolate();
		String::Utf8Value event( USE_ISOLATE( isolate ) args[0]->ToString() );
		Local<Function> cb = Handle<Function>::Cast( args[1] );
		if( StrCmp( *event, "request" ) == 0 ) {
			if( cb->IsFunction() )
				obj->requestCallback.Reset( isolate, cb );
		}
		if( StrCmp( *event, "connect" ) == 0 ) {
			if( cb->IsFunction() )
				obj->openCallback.Reset( isolate, cb );
		}
		if( StrCmp( *event, "accept" ) == 0 ) {
			if( cb->IsFunction() )
				obj->acceptCallback.Reset( isolate, cb );
		}
		if( StrCmp( *event, "error" ) == 0 ) {
			if( cb->IsFunction() )
				obj->errorCloseCallback.Reset( isolate, cb );
		}
	}
}

void wssObject::onConnect( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->openCallback.Reset( isolate, Handle<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Argument is not a function" ) ) );
	}
}

void wssObject::onAccept( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->acceptCallback.Reset( isolate, Handle<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Argument is not a function" ) ) );
	}
}

void wssObject::onRequest( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->requestCallback.Reset( isolate, Handle<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Argument is not a function" ) ) );
	}
}

void wssObject::onError( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		if( args[0]->IsFunction() )
			obj->errorCloseCallback.Reset( isolate, Handle<Function>::Cast( args[0] ) );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Argument is not a function" ) ) );
	}
}

void wssObject::accept( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( !obj->eventMessage ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Reject cannot be used outside of connection callback." ) ) );
		return;
	}
	if( args.Length() > 0 ) {
		String::Utf8Value protocol( USE_ISOLATE( isolate ) args[0]->ToString() );
		obj->eventMessage->protocol = StrDup( *protocol );
	}
	obj->eventMessage->accepted = 1;
}

void wssObject::reject( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( !obj->eventMessage ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Reject cannot be used outside of connection callback." ) ) );
		return;
	}
	obj->eventMessage->accepted = 0;
}

void wssObject::getReadyState( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
	args.GetReturnValue().Set( Integer::New( args.GetIsolate(), (int)obj->readyState ) );
#if 0
	Local<Object> h = args.Holder();
	Local<Object> t = args.This();
	//if( wssObject::tpl. )
	Local<FunctionTemplate> wrapper_tpl = wssObject::tpl.Get( isolate );
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

	uv_async_init( l.loop, &async, wssiAsyncMsg );
	async.data = this;
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
		obj->_this.Reset( isolate, args.This() );
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Value> *argv = new Local<Value>[args.Length()];

		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), 0, argv ).ToLocalChecked() );
		delete[] argv;
	}
}

void wssiObject::on( const FunctionCallbackInfo<Value>& args){
	Isolate* isolate = args.GetIsolate();

	if( args.Length() == 2 ) {
		wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
		String::Utf8Value event( USE_ISOLATE( isolate ) args[0]->ToString() );
		Local<Function> cb = Handle<Function>::Cast( args[1] );
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
		Local<Function> cb = Handle<Function>::Cast( args[0] );
		obj->messageCallback.Reset( isolate, cb );
	}
}

void wssiObject::onclose( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
		Local<Function> cb = Handle<Function>::Cast( args[0] );
		obj->closeCallback.Reset( isolate, cb );
	}
}

void wssiObject::close( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
	WebSocketClose( obj->pc, 1000, NULL );
}

void wssiObject::write( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
	if( !obj->pc ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Connection has already been closed." ) ) );
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
		String::Utf8Value buf( USE_ISOLATE( isolate ) args[0]->ToString() );
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
	Local<FunctionTemplate> wrapper_tpl = wssiObject::tpl.Get( isolate );
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
	(*pevt).eventType = WS_EVENT_OPEN;
	(*pevt)._this = wsc;
	EnqueLink( &wsc->eventQueue, pevt );
	uv_async_send( &wsc->async );
	return psv;
}

static void webSockClientClosed( PCLIENT pc, uintptr_t psv, int code, const char *reason )
{
	wscObject *wsc = (wscObject*)psv;
	struct wscEvent *pevt = GetWscEvent();
	(*pevt).eventType = WS_EVENT_CLOSE;
	(*pevt)._this = wsc;
	EnqueLink( &wsc->eventQueue, pevt );
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
	uv_async_send( &wsc->async );
}

wscObject::wscObject( wscOptions *opts ) {
	eventQueue = CreateLinkQueue();
	readyState = INITIALIZING;
	closed = 0;
	//lprintf( "Init async handle. (wsc) %p", &async );
	NetworkWait( NULL, 256, 2 );  // 1GB memory
	uv_async_init( l.loop, &async, wscAsyncMsg );
	async.data = this;

	pc = WebSocketOpen( opts->url, WS_DELAY_OPEN
		, webSockClientOpen
		, webSockClientEvent, webSockClientClosed, webSockClientError, (uintptr_t)this, opts->protocol );
	if( pc ) {
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
	lprintf( "Destruct wscObject" );
	if( !closed ) {
		lprintf( "destruct, try to generate WebSockClose" );
		RemoveClient( pc );
	}
	DeleteLinkQueue( &eventQueue );
}


void parseWscOptions( struct wscOptions *wscOpts, Isolate *isolate, Local<Object> opts ) {
	struct optionStrings *strings = getStrings( isolate );
	Local<String> optName;

	if( opts->Has( optName = strings->caString->Get( isolate ) ) ) {
		wscOpts->ssl = 1;
		String::Utf8Value rootCa( USE_ISOLATE( isolate ) opts->Get( optName )->ToString() );
		wscOpts->root_cert = StrDup( *rootCa );
	}

	if( opts->Has( optName = strings->keyString->Get( isolate ) ) ) {
		wscOpts->ssl = 1;
		String::Utf8Value rootCa( USE_ISOLATE( isolate ) opts->Get( optName )->ToString() );
		wscOpts->key = StrDup( *rootCa );
		wscOpts->key_len = rootCa.length();
	}

	if( opts->Has( optName = strings->passString->Get( isolate ) ) ) {
		wscOpts->ssl = 1;
		String::Utf8Value rootCa( USE_ISOLATE( isolate ) opts->Get( optName )->ToString() );
		wscOpts->pass = StrDup( *rootCa );
				wscOpts->pass_len = rootCa.length();
	}

	if( opts->Has( optName = strings->deflateString->Get( isolate ) ) ) {
		wscOpts->deflate = opts->Get( optName )->ToBoolean()->Value();
	}
	else
		wscOpts->deflate = false;
	if( opts->Has( optName = strings->deflateAllowString->Get( isolate ) ) ) {
		wscOpts->deflate_allow = opts->Get( optName )->ToBoolean()->Value();
	}
	else
		wscOpts->deflate_allow = false;
	if( opts->Has( optName = strings->maskingString->Get( isolate ) ) ) {
		wscOpts->apply_masking = opts->Get( optName )->ToBoolean()->Value();
	}
	else
		wscOpts->apply_masking = true;
}

void wscObject::New(const FunctionCallbackInfo<Value>& args){
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must specify url and optionally protocols or options for client." ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		bool checkArg2;
		// Invoked as constructor: `new MyObject(...)`
		struct wscOptions wscOpts;
		String::Utf8Value url( USE_ISOLATE( isolate ) args[0]->ToString() );
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
			protocol = new String::Utf8Value( USE_ISOLATE( isolate ) args[1]->ToString() );
		}
		else if( args[1]->IsArray() ) {
			checkArg2 = true;
			protocol = new String::Utf8Value( USE_ISOLATE( isolate ) args[1]->ToString() );
		}
		else if( args[1]->IsObject() ) {
			checkArg2 = false;
			parseWscOptions( &wscOpts, isolate, args[1]->ToObject() );
		}
		else
			checkArg2 = false;

		if( checkArg2 ) {
			if( args[2]->IsObject() ) {
				opts = args[2]->ToObject();
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
			obj->_this.Reset( isolate, _this );
			obj->Wrap( _this );
		}
		catch( const char *ex1 ) {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( ex1 ) ) ) );
		}
		if( wscOpts.root_cert )
			Deallocate( char *, wscOpts.root_cert );
		if( wscOpts.key )
			Deallocate( char *, wscOpts.key );
		if( wscOpts.pass )
			Deallocate( char *, wscOpts.pass );
		struct optionStrings *strings = getStrings( isolate );
		_this->Set( strings->bufferedAmountString->Get(isolate), Number::New( isolate, 0 ) );
		args.GetReturnValue().Set( _this );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete[] argv;
	}
}

void wscObject::onOpen( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Handle<Function>::Cast( args[0] );
		obj->openCallback.Reset( isolate, cb );
	}
}

void wscObject::onMessage( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Handle<Function>::Cast( args[0] );
		obj->messageCallback.Reset( isolate, cb );
	}
}

void wscObject::onClose( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Handle<Function>::Cast( args[0] );
		obj->closeCallback.Reset( isolate, cb );
	}
}

void wscObject::onError( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.Length() > 0 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		Local<Function> cb = Handle<Function>::Cast( args[0] );
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


void wscObject::on( const FunctionCallbackInfo<Value>& args){
	if( args.Length() == 2 ) {
		Isolate* isolate = args.GetIsolate();
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		String::Utf8Value event( USE_ISOLATE( isolate ) args[0]->ToString() );
		Local<Function> cb = Handle<Function>::Cast( args[1] );
		if( StrCmp( *event, "open" ) == 0 ){
			if( obj->readyState == OPEN ) {
				cb->Call( obj->_this.Get( isolate ), 0, NULL );
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
				String::NewFromUtf8( isolate, TranslateText( "Event name specified is not supported or known." ) ) ) );
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
					String::Utf8Value reason( USE_ISOLATE( isolate ) args[1]->ToString() );
					if( reason.length() > 123 ) {
						isolate->ThrowException( Exception::Error(
							String::NewFromUtf8( isolate, TranslateText( "SyntaxError (text reason too long)" ) ) ) );
						return;
					}
					if( obj->pc ) {
						WebSocketClose( obj->pc, 1000, NULL );
					}
				}
			}
			else {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "InvalidAccessError" ) ) ) );
				return;
			}
		}
		else {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( "InvalidAccessError" ) ) ) );
			return;
		}
	}
}

void wscObject::write( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	if( !obj->pc ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Connection is already closed." ) ) ) );
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
		String::Utf8Value buf( USE_ISOLATE( isolate ) args[0]->ToString() );
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
	Local<FunctionTemplate> wrapper_tpl = wscObject::tpl.Get( isolate );
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


httpRequestObject::httpRequestObject() {
	pc = NULL;
	memset( &_this, 0, sizeof( _this ) );
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
		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );
	}

}

void httpRequestObject::wait( const FunctionCallbackInfo<Value>& args ) {
	httpRequestObject *http = ObjectWrap::Unwrap<httpRequestObject>( args.This() );

	args.GetReturnValue().Set( args.This() );

}

void httpRequestObject::on( const FunctionCallbackInfo<Value>& args ) {
	String::Utf8Value eventName( USE_ISOLATE( args.GetIsolate() ) args[0]->ToString() );
	httpRequestObject *http = ObjectWrap::Unwrap<httpRequestObject>( args.This() );

	Local<Function> cb = Local<Function>::Cast( args[1] );
	/* abort, connect, continue, response, socket, timeout, upgrade, */
	if( StrCmp( *eventName, "error" ) == 0 ) {

	}
	args.GetReturnValue().Set( args.This() );
}


void httpRequestObject::getRequest( const FunctionCallbackInfo<Value>& args, bool secure ) {
	Isolate* isolate = args.GetIsolate();

	httpRequestObject *httpRequest = new httpRequestObject();

	if( secure )
		httpRequest->port = 443;
	else
		httpRequest->port = 80;
	httpRequest->ssl = secure;

	// incoming message events; aborted, close, readableStream (close, data, end, error, readable)
	Local<Object> options = args[0]->ToObject();
	Local<String> optName;
	optionStrings *strings = getStrings( isolate );

	if( options->Has( optName = strings->hostnameString->Get( isolate ) ) ) {
		String::Utf8Value value( USE_ISOLATE( isolate ) options->Get( optName ) );
		httpRequest->hostname = StrDup( *value );
	}

	if( options->Has( optName = strings->portString->Get( isolate ) ) ) {
		int32_t x = options->Get( optName )->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		httpRequest->port = x;
	}

	if( options->Has( optName = strings->methodString->Get( isolate ) ) ) {
		String::Utf8Value value( USE_ISOLATE( isolate ) options->Get( optName ) );
		httpRequest->method = StrDup( *value );
	}

	if( secure && options->Has( optName = strings->caString->Get( isolate ) ) ) {
		if( options->Get( optName )->IsString() ) {
			String::Utf8Value value( USE_ISOLATE( isolate ) options->Get( optName ) );
			httpRequest->ca = StrDup( *value );
		}
	}

	if( options->Has( optName = strings->rejectUnauthorizedString->Get( isolate ) ) ) {
		httpRequest->rejestUnauthorized = options->Get( optName )->ToBoolean()->Value();
	}

	if( options->Has( optName = strings->pathString->Get( isolate ) ) ) {
		String::Utf8Value value( USE_ISOLATE( isolate ) options->Get( optName ) );
		httpRequest->path = StrDup( *value );
	}

	Local<Object> result = Object::New( isolate );
	NetworkWait( NULL, 256, 2 );  // 1GB memory

	{
		PVARTEXT pvtAddress = VarTextCreate();
		vtprintf( pvtAddress, "%s:%d", httpRequest->hostname, httpRequest->port );

		PTEXT address = VarTextPeek( pvtAddress );
		PTEXT url = SegCreateFromText( httpRequest->path );

		HTTPState state;
		//lprintf( "request: %s  %s", GetText( address ), GetText( url ) );
		if( httpRequest->ssl )
			state = GetHttpsQuery( address, url, httpRequest->ca );
		else
			state = GetHttpQuery( address, url );

		PTEXT content = GetHttpContent(state);
		if( state && content )
		{
			result->Set( String::NewFromUtf8( isolate, "content" ), String::NewFromUtf8( isolate, GetText( content ) ) );
			result->Set( String::NewFromUtf8( isolate, "statusCode" ), Integer::New( isolate, GetHttpResponseCode( state ) ) );
			result->Set( String::NewFromUtf8( isolate, "status" ), String::NewFromUtf8( isolate, GetText( GetHttpResponce( state ) ) ) );
			Local<Array> arr = Array::New( isolate );
			PLIST headers = GetHttpHeaderFields( state );
			INDEX idx;
			struct HttpField *header;
			//headers
			LIST_FORALL( headers, idx, struct HttpField *, header ) {
				arr->Set( String::NewFromUtf8( isolate, (const char*)GetText( header->name ), NewStringType::kNormal, (int)GetTextSize( header->name ) ).ToLocalChecked()
					, String::NewFromUtf8( isolate, (const char*)GetText( header->value ), NewStringType::kNormal, (int)GetTextSize( header->value ) ).ToLocalChecked() );
			}
			result->Set( String::NewFromUtf8( isolate, "headers" ), arr );

			DestroyHttpState( state );
		}
		else
		{
			result->Set( String::NewFromUtf8( isolate, "error" ),
							state?String::NewFromUtf8( isolate, "No Content" ):String::NewFromUtf8( isolate, "Connect Error" ) );

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
