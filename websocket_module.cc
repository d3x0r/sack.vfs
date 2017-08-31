
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

enum wsEvents {
	WS_EVENT_OPEN,
	WS_EVENT_ACCEPT,
	WS_EVENT_CLOSE,
	WS_EVENT_READ,
	WS_EVENT_ERROR
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
	Eternal<String> *socketString;
	Eternal<String> *maskingString;
	Eternal<String> *bytesReadString;
	Eternal<String> *bytesWrittenString;
};

static PLIST strings;

struct wssOptions {
	char *url;
	int port;
	bool ssl;
	char *cert_chain;
	int cert_chain_len;
	char *key;
	int key_len;
	bool deflate;
	bool deflate_allow;
	bool apply_masking;
};

struct wscOptions {
	char *url;
	char *protocol;
	bool ssl;
	char *key;
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
	PLIST send;
	LOGICAL send_close; // need reason.
	CPOINTER buf;
	size_t buflen;
	LOGICAL binary;
	PTHREAD waiter;
	LOGICAL done;
};
typedef struct wssiEvent WSSI_EVENT;
#define MAXWSSI_EVENTSPERSET 128
DeclareSet( WSSI_EVENT );

struct wscEvent {
	enum wsEvents eventType;
	class wscObject *_this;
	PLIST send;
	LOGICAL send_close;
	CPOINTER buf;
	size_t buflen;
	LOGICAL binary;
	PTHREAD waiter;
	LOGICAL done;
};
typedef struct wscEvent WSC_EVENT;
#define MAXWSC_EVENTSPERSET 128
DeclareSet( WSC_EVENT );

static struct local {
	int data;
	uv_loop_t* loop;
	int waiting;
	PWSS_EVENTSET wssEvents;
	PWSSI_EVENTSET wssiEvents;
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
		check->portString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "port" ) );
		check->localPortString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "localPort" ) );
		check->remotePortString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "remotePort" ) );
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
		check->socketString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "connection" ) );
		check->maskingString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "masking" ) );
		check->bytesReadString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "bytesRead" ) );
		check->bytesWrittenString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "bytesWritten" ) );
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
	const char *protocolResponse;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	static Persistent<Function> constructor;
	Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> acceptCallback; //
	struct wssEvent *eventMessage;

public:

	wssObject( struct wssOptions *opts );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	//static void onMessage( const v8::FunctionCallbackInfo<Value>& args );
	static void onConnect( const v8::FunctionCallbackInfo<Value>& args );
	static void onAccept( const v8::FunctionCallbackInfo<Value>& args );
	static void accept( const v8::FunctionCallbackInfo<Value>& args );
	static void reject( const v8::FunctionCallbackInfo<Value>& args );

	~wssObject();
};

// web sock client Object
class wscObject : public node::ObjectWrap {
	char *serverUrl;
public:
	PCLIENT pc;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
	wscEvent *eventMessage;
	LOGICAL closed;
public:
	static Persistent<Function> constructor;
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
	static void write( const v8::FunctionCallbackInfo<Value>& args );

	~wscObject();
};

// web sock server instance Object  (a connection from a remote)
class wssiObject : public node::ObjectWrap {
public:
	wssObject *listener;  // have to check this for pending event messages too to prevent blocks.
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
	struct wssiEvent *eventMessage;
	PCLIENT pc;
	LOGICAL closed;
public:
	static Persistent<Function> constructor;
	Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> messageCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; //
	
public:

	wssiObject( PCLIENT pc, Local<Object> _this );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void write( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void onmessage( const v8::FunctionCallbackInfo<Value>& args );
	static void onclose( const v8::FunctionCallbackInfo<Value>& args );

	~wssiObject();
};

Persistent<Function> wssObject::constructor;
Persistent<Function> wssiObject::constructor;
Persistent<Function> wscObject::constructor;

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


static void wssAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	wssObject* myself = (wssObject*)handle->data;

	HandleScope scope(isolate);
	{
		struct wssEvent *msg;
		while( msg = (struct wssEvent *)DequeLink( &myself->eventQueue ) ) {
			Local<Value> argv[2];
			myself->eventMessage = msg;
			if( msg->eventType == WS_EVENT_OPEN ) {
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
				wssi->Set( strings->socketString->Get(isolate), makeSocket( isolate, msg->pc ) );
				wssiObject *wssiInternal = new wssiObject( msg->pc, wssi );
				wssiInternal->_this.Reset( isolate, wssi );

				argv[0] = wssi;
				Local<Function> cb = Local<Function>::New( isolate, myself->openCallback );
				cb->Call( msg->_this->_this.Get( isolate ), 1, argv );
				msg->result = wssiInternal;
			}
			else if( msg->eventType == WS_EVENT_ACCEPT ) {
				if( msg->protocol )
					argv[0] = Local<String>::New( isolate, String::NewFromUtf8( isolate, msg->protocol ) );
				else
					argv[0] = Null( isolate );
				argv[1] = Local<String>::New( isolate, String::NewFromUtf8( isolate, msg->resource ) );

				Local<Function> cb = myself->acceptCallback.Get( isolate );
				Local<Value> result = cb->Call( msg->_this->_this.Get( isolate ), 2, argv );
				//msg->accepted = (int)result->ToBoolean()->Value();

			}
			myself->eventMessage = NULL;
			msg->done = TRUE;
			WakeThread( msg->waiter );
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
			myself->eventMessage = eventMessage;
			switch( eventMessage->eventType ) {
			case WS_EVENT_READ:
				size_t length;
				if( eventMessage->binary ) {
					ab =
						ArrayBuffer::New( isolate,
						(void*)eventMessage->buf,
							length = eventMessage->buflen );
					argv[0] = ab;
					myself->messageCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
				}
				else {
					MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
					argv[0] = buf.ToLocalChecked();
					myself->messageCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
				}
				break;
			case WS_EVENT_CLOSE:
				myself->closeCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 0, argv );
				uv_close( (uv_handle_t*)&myself->async, NULL );
				DeleteLinkQueue( &myself->eventQueue );
				break;
			case WS_EVENT_ERROR:
				myself->errorCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 0, argv );
				break;
			}
			myself->eventMessage = NULL;
			eventMessage->done = 1;
			WakeThread( eventMessage->waiter );
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
				struct optionStrings *strings;
				strings = getStrings( isolate );
				wsc->_this.Get(isolate)->Set( strings->socketString->Get( isolate ), makeSocket( isolate, wsc->pc ) );
				cb->Call( eventMessage->_this->_this.Get(isolate), 0, argv );
				DeleteFromSet( WSC_EVENT, l.wscEvents, eventMessage );
				break;
			case WS_EVENT_READ:
				size_t length;
				if( eventMessage->binary ) {
					ab =
						ArrayBuffer::New( isolate,
						(void*)eventMessage->buf,
							length = eventMessage->buflen );
					argv[0] = ab;
					wsc->messageCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
				}
				else {
					MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
					argv[0] = buf.ToLocalChecked();
					wsc->messageCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
				}
				Deallocate( CPOINTER, eventMessage->buf );
				DeleteFromSet( WSC_EVENT, l.wscEvents, eventMessage );
				break;
			case WS_EVENT_CLOSE:
				cb = Local<Function>::New( isolate, wsc->closeCallback );
				if( !cb.IsEmpty() )
					cb->Call( eventMessage->_this->_this.Get( isolate ), 0, argv );
				uv_close( (uv_handle_t*)&wsc->async, NULL );
				DeleteLinkQueue( &wsc->eventQueue );
				DeleteFromSet( WSC_EVENT, l.wscEvents, eventMessage );
				break;
			}
			if( eventMessage->waiter ) {
				eventMessage->done = 1;
				WakeThread( eventMessage->waiter );
			}
		}
	}
}

int accepted = 0;

void InitWebSocket( Isolate *isolate, Handle<Object> exports ){

	if( !l.loop )
		l.loop = uv_default_loop();
	//NetworkWait( NULL, 2000000, 16 );  // 1GB memory
	NetworkWait( NULL, 4000, 2 );  // 1GB memory
	Local<Object> o = Object::New( isolate );
	SET_READONLY( exports, "WebSocket", o );

	{
		Local<FunctionTemplate> wssTemplate;
		wssTemplate = FunctionTemplate::New( isolate, wssObject::New );
		wssTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.ws.server" ) );
		wssTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "close", wssObject::close );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "on", wssObject::on );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onconnect", wssObject::onConnect );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onaccept", wssObject::onAccept );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "accept", wssObject::accept );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "reject", wssObject::reject );
		wssTemplate->ReadOnlyPrototype();
		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onmessage", wsObject::on );
		//NODE_SET_PROTOTYPE_METHOD( wssTemplate, "on", wsObject::on );
		
		wssObject::constructor.Reset( isolate, wssTemplate->GetFunction() );

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
		wscTemplate->ReadOnlyPrototype();

		wscObject::constructor.Reset( isolate, wscTemplate->GetFunction() );

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
		wssiTemplate->ReadOnlyPrototype();
		
		wssiObject::constructor.Reset( isolate, wssiTemplate->GetFunction() );
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
	if( wss->protocolResponse ) {
		Deallocate( const char *, wss->protocolResponse );
		wss->protocolResponse = NULL;
	}
	struct wssEvent evt;
	evt.eventType = WS_EVENT_OPEN;
	evt.send = NULL;
	evt.done = FALSE;
	evt.waiter = MakeThread();
	evt._this = wss;
	evt.pc = pc;
	EnqueLink( &wss->eventQueue, &evt );
	uv_async_send( &wss->async );

	while( !evt.done )
		Wait();

	{
		INDEX idx;
		struct pendingSend *send;
		if( evt.send ) {
			LIST_FORALL( evt.send, idx, struct pendingSend *, send ) {
				if( send->binary )
					WebSocketSendBinary( pc, send->buffer, send->buflen );
				else
					WebSocketSendText( pc, send->buffer, send->buflen );
				Deallocate( POINTER, send->buffer );
				Deallocate( struct pendingSend*, send );
			}
			DeleteList( &evt.send );
		}
	}

	return (uintptr_t)evt.result;
}

static void webSockServerClosed( PCLIENT pc, uintptr_t psv )
{
	wssiObject *wssi = (wssiObject*)psv;
	struct wssiEvent evt;
	evt.eventType = WS_EVENT_CLOSE;
	evt.done = FALSE;
	evt.waiter = MakeThread();
	evt._this = wssi;
	EnqueLink( &wssi->eventQueue, &evt );
	uv_async_send( &wssi->async );
	while( !evt.done )
		Wait();

}

static void webSockServerError( PCLIENT pc, uintptr_t psv, int error ){
	wssiObject *wssi = (wssiObject*)psv;
	struct wssiEvent evt;
	evt.eventType = WS_EVENT_ERROR;
	evt.done = FALSE;
	evt.waiter = MakeThread();
	evt._this = wssi;
	EnqueLink( &wssi->eventQueue, &evt );
	uv_async_send( &wssi->async );

	while( !evt.done )
		Wait();

}

static void webSockServerEvent( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, size_t msglen ){
	//lprintf( "Received:%p %d", buffer, binary );
	wssiObject *wssi = (wssiObject*)psv;
	struct wssiEvent evt;
	evt.send = NULL;
	evt.send_close = FALSE;
	evt.eventType = WS_EVENT_READ;
	evt.done = FALSE;
	evt.waiter = MakeThread();
	evt._this = wssi;
	//WebSocket
	evt.binary = binary;
	evt.buf = buffer;
	evt.buflen = msglen;
	EnqueLink( &wssi->eventQueue, &evt );
	uv_async_send( &wssi->async );

	while( !evt.done )
		Wait();
	{
		INDEX idx;
		struct pendingSend *send; 
		if( evt.send ) {
			LIST_FORALL( evt.send, idx, struct pendingSend *, send ) {
				if( send->binary )
					WebSocketSendBinary( wssi->pc, send->buffer, send->buflen );
				else
					WebSocketSendText( wssi->pc, send->buffer, send->buflen );
				Deallocate( POINTER, send->buffer );
				Deallocate( struct pendingSend*, send );
			}
			DeleteList( &evt.send );
		}
		if( evt.send_close ) {
			WebSocketClose( pc );
		}
	}
}

static LOGICAL webSockServerAccept( PCLIENT pc, uintptr_t psv, const char *protocols, const char *resource, char **protocolReply ) {
	wssObject *wss = (wssObject*)psv;
	if( accepted++ > 1000 ) {
		DebugDumpMem();
		accepted = 0;
	}
	if( !wss->acceptCallback.IsEmpty() ) {
		struct wssEvent evt;
		evt.protocol = protocols;
		evt.resource = resource;
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
			wss->protocolResponse = evt.protocol;
		(*protocolReply) = (char*)evt.protocol;
		return (uintptr_t)evt.accepted;
	}
	return 1;
}

wssObject::wssObject( struct wssOptions *opts ) {
	char tmp[256];
	if( !opts->url ) {
		snprintf( tmp, 256, "ws://[::]:%d/", opts->port ? opts->port : 8080 );
		//snprintf( tmp, 256, "ws://0.0.0.0:%d/", opts->port ? opts->port : 8080 );
		opts->url = tmp;
	}
	protocolResponse = NULL;
	closed = 0;
	pc = WebSocketCreate( opts->url, webSockServerOpen, webSockServerEvent, webSockServerClosed, webSockServerError, (uintptr_t)this );
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
			, opts->cert_chain, opts->cert_chain ? strlen( opts->cert_chain ) : 0
			, opts->key, opts->key ? strlen( opts->key ) : 0 );
	}
	SetWebSocketAcceptCallback( pc, webSockServerAccept );
	eventQueue = CreateLinkQueue();

	//lprintf( "Init async handle. (wss)" );
	uv_async_init( l.loop, &async, wssAsyncMsg );
	async.data = this;
}

wssObject::~wssObject() {
	//lprintf( "Destruct wssObject" );
	RemoveClient( pc );
	DeleteLinkQueue( &eventQueue );
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
		wssOpts.url = NULL;
		wssOpts.port = 0;
		if( args[0]->IsObject() ) {
			Local<Object> opts = args[0]->ToObject();

			Local<String> optName;
			struct optionStrings *strings = getStrings( isolate );
			if( !opts->Has( optName = strings->portString->Get( isolate ) ) ) {
				wssOpts.port = 8080;
			}
			else {
				wssOpts.port = (int)opts->Get( optName )->ToInteger()->Value();
			}
			if( !opts->Has( optName = strings->certString->Get( isolate ) ) ) {
				wssOpts.cert_chain = NULL;
				wssOpts.cert_chain_len =0;
			}
			else {
				String::Utf8Value cert( opts->Get( optName )->ToString() );
				wssOpts.cert_chain = *cert;
				wssOpts.cert_chain_len = cert.length();
			}

			if( !opts->Has( optName = strings->keyString->Get( isolate ) ) ) {
				wssOpts.key = NULL;
				wssOpts.key_len = 0;
			}
			else {
				String::Utf8Value cert( opts->Get( optName )->ToString() );
				wssOpts.key = *cert;
				wssOpts.key_len = cert.length();
			}
			if( wssOpts.key || wssOpts.cert_chain ) {
				wssOpts.ssl = 1;
			}
			else
				wssOpts.ssl = 0;

			if( !opts->Has( optName = strings->deflateString->Get( isolate ) ) ) {
				wssOpts.deflate = false;
			}
			else
				wssOpts.deflate = (opts->Get( optName )->ToBoolean()->Value());
			if( !opts->Has( optName = strings->deflateAllowString->Get( isolate ) ) ) {
				wssOpts.deflate_allow = false;
			}
			else {
				wssOpts.deflate_allow = (opts->Get( optName )->ToBoolean()->Value());
				//lprintf( "deflate allow:%d", wssOpts.deflate_allow );
			}

			if( opts->Has( optName = strings->maskingString->Get( isolate ) ) ) {
				wssOpts.apply_masking = opts->Get( optName )->ToBoolean()->Value();
			}
			else
				wssOpts.apply_masking = false;
		}
		else if( args[0]->IsNumber() ) {
			wssOpts.port = (int)args[0]->IntegerValue();
		}
		Local<Object> _this = args.This();
		wssObject* obj = new wssObject( &wssOpts );
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
		args.GetReturnValue().Set( Nan::NewInstance( cons, argc, argv ).ToLocalChecked() );
		delete argv;
	}
}

void wssObject::close( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
}

void wssObject::on( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() == 2 ) {
		Isolate* isolate = args.GetIsolate();
		String::Utf8Value event( args[0]->ToString() );
		Local<Function> cb = Handle<Function>::Cast( args[1] );
		if( StrCmp( *event, "connect" ) == 0 ) {
			if( cb->IsFunction() )
				obj->openCallback.Reset( isolate, cb );
		}
		if( StrCmp( *event, "accept" ) == 0 ) {
			if( cb->IsFunction() )
				obj->acceptCallback.Reset( isolate, cb );
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

void wssObject::accept( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( !obj->eventMessage ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Reject cannot be used outside of connection callback." ) ) );
		return;
	}
	if( args.Length() > 0 ) {
		String::Utf8Value protocol( args[0]->ToString() );
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

wssiObject::wssiObject( PCLIENT pc, Local<Object> obj ) {
	eventQueue = CreateLinkQueue();
	this->pc = pc;
	this->closed = 0;
	//lprintf( "Init async handle. (wsi)" );
	uv_async_init( l.loop, &async, wssiAsyncMsg );
	async.data = this;
	this->Wrap( obj );
}

wssiObject::~wssiObject() {
	lprintf( "Destruct wssiObject" );
	if( !closed ) {
		RemoveClient( pc );
	}
}

void wssiObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	if( args.IsConstructCall() ) {
		args.GetReturnValue().Set( args.This() );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Value> *argv = new Local<Value>[args.Length()];

		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( Nan::NewInstance( cons, 0, argv ).ToLocalChecked() );
		delete[] argv;
	}
}

void wssiObject::on( const FunctionCallbackInfo<Value>& args){
	Isolate* isolate = args.GetIsolate();

	if( args.Length() == 2 ) {
		wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
		String::Utf8Value event( args[0]->ToString() );
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
	if( obj->eventMessage )
		obj->eventMessage->send_close = TRUE;
	else
		WebSocketClose( obj->pc );
}

void wssiObject::write( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
	PLIST *sendQueue;
	if( obj->eventMessage )
		sendQueue = &obj->eventMessage->send;
	else if( obj->listener->eventMessage )
		sendQueue = &obj->listener->eventMessage->send;
	else
		sendQueue = NULL;

	if( args[0]->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
		if( sendQueue )
		{
			struct pendingSend *pend = NewArray( struct pendingSend, 1 );
			pend->binary = true;
			pend->buflen = ab->ByteLength();
			pend->buffer = NewArray( uint8_t, pend->buflen );
			MemCpy( pend->buffer, ab->GetContents().Data(), pend->buflen );
			AddLink( sendQueue, pend );
		}
		else {
			WebSocketSendBinary( obj->pc, ab->GetContents().Data(), ab->ByteLength() );
		}
	}
	else if( args[0]->IsString() ) {
		String::Utf8Value buf( args[0]->ToString() );
		if( sendQueue ) {
			struct pendingSend *pend = NewArray( struct pendingSend, 1 );
			pend->binary = false;
			pend->buflen = buf.length();
			pend->buffer = NewArray( uint8_t, pend->buflen );
			MemCpy( pend->buffer, *buf, pend->buflen );
			AddLink( sendQueue, pend );
		}
		else
			WebSocketSendText( obj->pc, *buf, buf.length() );
	}
	else {
		lprintf( "Unhandled message format" );
	}
}



static uintptr_t webSockClientOpen( PCLIENT pc, uintptr_t psv ) {
	wscObject *wsc = (wscObject*)psv;

	struct wscEvent *pevt = GetFromSet( WSC_EVENT, &l.wscEvents );
	(*pevt).eventType = WS_EVENT_OPEN;
	(*pevt).send = NULL;
	(*pevt).done = FALSE;
	(*pevt).waiter = NULL;// MakeThread();
	(*pevt)._this = wsc;
	EnqueLink( &wsc->eventQueue, pevt );
	uv_async_send( &wsc->async );
	return psv;

	while( !(*pevt).done )
		Wait();
	if( (*pevt).send )
	{
		INDEX idx;
		struct pendingSend *send;
		LIST_FORALL( (*pevt).send, idx, struct pendingSend *, send ) {
			if( send->binary )
				WebSocketSendBinary( pc, send->buffer, send->buflen );
			else
				WebSocketSendText( pc, send->buffer, send->buflen );
			Deallocate( POINTER, send->buffer );
			Deallocate( struct pendingSend*, send );
		}
		DeleteList( &(*pevt).send );
	}
	return (uintptr_t)psv;
}

static void webSockClientClosed( PCLIENT pc, uintptr_t psv )
{
	wscObject *wsc = (wscObject*)psv;

	struct wscEvent *pevt = GetFromSet( WSC_EVENT, &l.wscEvents );
	(*pevt).eventType = WS_EVENT_CLOSE;
	(*pevt).done = FALSE;
	(*pevt).waiter = NULL;// MakeThread();
	(*pevt)._this = wsc;
	EnqueLink( &wsc->eventQueue, pevt );
	uv_async_send( &wsc->async );
	return;
	while( !(*pevt).done )
		Wait();

}

static void webSockClientError( PCLIENT pc, uintptr_t psv, int error ) {
	wscObject *wsc = (wscObject*)psv;

	struct wscEvent evt;
	evt.eventType = WS_EVENT_ERROR;
	evt.done = FALSE;
	evt.waiter = MakeThread();
	evt._this = wsc;
	EnqueLink( &wsc->eventQueue, &evt );
	uv_async_send( &wsc->async );
	while( !evt.done )
		Wait();
}

static void webSockClientEvent( PCLIENT pc, uintptr_t psv, LOGICAL type, CPOINTER buffer, size_t msglen ) {
	wscObject *wsc = (wscObject*)psv;

	struct wscEvent *pevt = GetFromSet( WSC_EVENT, &l.wscEvents );
	(*pevt).eventType = WS_EVENT_READ;
	(*pevt).buf = NewArray( uint8_t*, msglen );
	memcpy( (POINTER)(*pevt).buf, buffer, msglen );
	(*pevt).buflen = msglen;
	(*pevt).binary = type;
	(*pevt).send = NULL;
	(*pevt).send_close = FALSE;
	(*pevt).done = FALSE;
	(*pevt).waiter = NULL;// MakeThread();
	(*pevt)._this = wsc;
	EnqueLink( &wsc->eventQueue, pevt );
	uv_async_send( &wsc->async );

	return;
	while( !(*pevt).done )
		Wait();
	/*
	{
		INDEX idx;
		struct pendingSend *send;
		if( evt.send ) {
			LIST_FORALL( evt.send, idx, struct pendingSend *, send ) {
				if( send->binary )
					WebSocketSendBinary( pc, send->buffer, send->buflen );
				else
					WebSocketSendText( pc, send->buffer, send->buflen );
				Deallocate( POINTER, send->buffer );
				Deallocate( struct pendingSend*, send );
			}
			DeleteList( &evt.send );
		}
		if( evt.send_close ) {
			WebSocketClose( pc );
		}
	}
	*/
}

wscObject::wscObject( wscOptions *opts ) {
	pc = WebSocketOpen( opts->url, WS_DELAY_OPEN
		, webSockClientOpen
		, webSockClientEvent, webSockClientClosed, webSockClientError, (uintptr_t)this, opts->protocol );
	if( opts->deflate )
		SetWebSocketDeflate( pc, WEBSOCK_DEFLATE_ENABLE );
	if( !opts->apply_masking )
		SetWebSocketMasking( pc, 0 );
	if( opts->ssl ) {
		ssl_BeginClientSession( pc, opts->key, opts->key?strlen( opts->key ):0
			, opts->root_cert, opts->root_cert ?strlen( opts->root_cert ):0 );
	}
	WebSocketConnect( pc );
	eventQueue = CreateLinkQueue();
	eventMessage = NULL;
	closed = 0;
	//lprintf( "Init async handle. (wsc) %p", &async );
	uv_async_init( l.loop, &async, wscAsyncMsg );
	async.data = this;
}

wscObject::~wscObject() {
	if( !closed ) {
		//lprintf( "Destruct wscObject" );
		RemoveClient( pc );
	}
	DeleteLinkQueue( &eventQueue );
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
		String::Utf8Value url( args[0]->ToString() );
		String::Utf8Value *protocol = NULL;
		Local<Object> opts;
		opts.Clear();
		wscOpts.ssl = FALSE;
		wscOpts.key = NULL;
		wscOpts.root_cert = NULL;
		wscOpts.protocol = NULL;

		if( args[1]->IsString() ) {
			checkArg2 = true;
			protocol = new String::Utf8Value( args[1]->ToString() );
		}
		else if( args[1]->IsArray() ) {
			checkArg2 = true;
			protocol = new String::Utf8Value( args[1]->ToString() );
		}
		else if( args[1]->IsObject() ) {
			lprintf( "args1 is object..." );
			checkArg2 = false;
			opts = args[1]->ToObject();
			Local<String> optName;
			struct optionStrings *strings = getStrings( isolate );

			if( opts->Has( optName = strings->caString->Get( isolate ) ) ) {
				String::Utf8Value rootCa( opts->Get( optName )->ToString() );
				wscOpts.root_cert = *rootCa;
			}
			else 
				wscOpts.root_cert = NULL;
			if( opts->Has( optName = strings->deflateString->Get( isolate ) ) ) {
				wscOpts.deflate = opts->Get( optName )->ToBoolean()->Value();
			}
			else
				wscOpts.deflate = false;
			if( opts->Has( optName = strings->deflateAllowString->Get( isolate ) ) ) {
				wscOpts.deflate_allow = opts->Get( optName )->ToBoolean()->Value();
			}
			else
				wscOpts.deflate_allow = false;
			if( opts->Has( optName = strings->maskingString->Get( isolate ) ) ) {
				wscOpts.apply_masking = opts->Get( optName )->ToBoolean()->Value();
			}
			else
				wscOpts.apply_masking = true;
		}
		else
			checkArg2 = false;

		if( checkArg2 ) {
			if( args[2]->IsObject() ) {
				opts = args[1]->ToObject();
			}
		}

		wscOpts.url = *url;
		if( protocol )
			wscOpts.protocol = *(protocol[0]);
		wscOpts.deflate = false;

		Local<Object> _this = args.This();
		wscObject* obj = new wscObject( &wscOpts );
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
		args.GetReturnValue().Set( Nan::NewInstance( cons, argc, argv ).ToLocalChecked() );
		delete argv;
	}
}

void wscObject::on( const FunctionCallbackInfo<Value>& args){
	if( args.Length() == 2 ) {
		Isolate* isolate = args.GetIsolate();
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
		String::Utf8Value event( args[0]->ToString() );
		Local<Function> cb = Handle<Function>::Cast( args[1] );
		if( StrCmp( *event, "open" ) == 0 ){
			obj->openCallback.Reset(isolate,cb);			
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
	//lprintf( "never did do anything with wscObject close. (until now)" );
	if( obj->eventMessage )
		obj->eventMessage->send_close = TRUE;
	else
		WebSocketClose( obj->pc );
}

void wscObject::write( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
	if( args[0]->IsTypedArray() ) {
		lprintf( "Typed array (unhandled)" );
	} else if( args[0]->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
		if( obj->eventMessage ) {
			struct pendingSend *pend = NewArray( struct pendingSend, 1 );
			pend->binary = true;
			pend->buflen = ab->ByteLength();
			pend->buffer = NewArray( uint8_t, pend->buflen );
			MemCpy( pend->buffer, ab->GetContents().Data(), pend->buflen );
			AddLink( &obj->eventMessage->send, pend );
		}
		else {
			WebSocketSendBinary( obj->pc, ab->GetContents().Data(), ab->ByteLength() );
		}
	}
	else if( args[0]->IsString() ) {
		String::Utf8Value buf( args[0]->ToString() );
		if( obj->eventMessage ) {
			struct pendingSend *pend = NewArray( struct pendingSend, 1 );
			pend->binary = false;
			pend->buflen = buf.length();
			pend->buffer = NewArray( uint8_t, pend->buflen );
			MemCpy( pend->buffer, *buf, pend->buflen );
			AddLink( &obj->eventMessage->send, pend );
		}
		else
			WebSocketSendText( obj->pc, *buf, buf.length() );
	}
	else {
		lprintf( "Unhandled message format" );
	}
}



