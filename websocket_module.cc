
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
	WS_EVENT_CLOSE,
	WS_EVENT_READ,
	WS_EVENT_ERROR
};

struct optionStrings {
	Isolate *isolate;

	Eternal<String> *portString;
	Eternal<String> *urlString;
};

static PLIST strings;

struct wssOptions {
	char *url;
	int port;
};

struct wscOptions {
	char *url;
	char *protocol;
};

struct pendingSend {
	LOGICAL binary;
	POINTER buffer;
	size_t buflen;
};

struct wssEvent {
	class wssObject *_this;
	PCLIENT pc;
	PTHREAD waiter;
	LOGICAL done;
	class wssiObject *result;
};

struct wssiEvent {
	enum wsEvents eventType;
	class wssiObject *_this;
	PLIST send;
	CPOINTER buf;
	size_t buflen;
	LOGICAL binary;
	PTHREAD waiter;
	LOGICAL done;
};

struct wscEvent {
	enum wsEvents eventType;
	class wscObject *_this;
	CPOINTER buf;
	size_t buflen;
	PTHREAD waiter;
	LOGICAL done;
};


static struct local {
	int data;
	uv_loop_t* loop;
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
		check->isolate = isolate;
		check->portString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "port" ) );
		//check->certString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "cert" ) );
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

	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	static Persistent<Function> constructor;
	Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	
public:

	wssObject( struct wssOptions *opts );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void onMessage( const v8::FunctionCallbackInfo<Value>& args );
	static void onConnect( const v8::FunctionCallbackInfo<Value>& args );

	~wssObject();
};

// web sock client Object
class wscObject : public node::ObjectWrap {
	PCLIENT pc;
	char *serverUrl;
public:
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
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


static void wssAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	wssObject* myself = (wssObject*)handle->data;

	HandleScope scope(isolate);
	{
		struct wssEvent *msg;
		while( msg = (struct wssEvent *)DequeLink( &myself->eventQueue ) ) {
			Local<Value> argv[1];
			
			Local<Function> cons = Local<Function>::New( isolate, wssiObject::constructor );
			MaybeLocal<Object> _wssi = cons->NewInstance( isolate->GetCurrentContext(), 0, argv );
			if( _wssi.IsEmpty() ) {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "Internal Error creating client connection instance." ) ) ) );
				lprintf( "Internal error. I wonder what exception looks like" );
				break;
			}
			Local<Object> wssi = _wssi.ToLocalChecked();
			
			wssiObject *wssiInternal = new wssiObject( msg->pc, wssi );
			wssiInternal->_this.Reset( isolate, wssi );

			argv[0] = wssi;
			Local<Function> cb = Local<Function>::New( isolate, myself->openCallback );
			cb->Call( msg->_this->_this.Get(isolate), 1, argv );

			msg->done = TRUE;
			msg->result = wssiInternal;
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
	wscObject* myself = (wscObject*)handle->data;

	HandleScope scope(isolate);

	{
		Local<Value> argv[2];
		struct wscEvent* eventMessage;
		while( eventMessage = (struct wscEvent*)DequeLink( &myself->eventQueue ) ) {
			Local<Function> cb;
			Local<ArrayBuffer> ab;
			switch( eventMessage->eventType ) {
			case WS_EVENT_OPEN:
				cb = Local<Function>::New( isolate, myself->openCallback );
				cb->Call( eventMessage->_this->_this.Get(isolate), 0, argv );
				break;
			case WS_EVENT_READ:
				size_t length;
				ab = ArrayBuffer::New( isolate,
					(void*)eventMessage->buf,
						length = eventMessage->buflen );
				cb = Local<Function>::New( isolate, myself->messageCallback );
				argv[1] = ab;
				cb->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
				break;
			case WS_EVENT_CLOSE:
				cb = Local<Function>::New( isolate, myself->closeCallback );
				cb->Call( eventMessage->_this->_this.Get( isolate ), 0, argv );
				break;
			}
			eventMessage->done = 1;
			WakeThread( eventMessage->waiter );
		}
	}
}



void InitWebSocket( Isolate *isolate, Handle<Object> exports ){

	if( !l.loop )
		l.loop = uv_default_loop();
	NetworkWait( NULL, 2000000, 16 );
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

static uintptr_t webSockServerOpen( PCLIENT pc, uintptr_t psv ){
	wssObject *wss = (wssObject*)psv;

	struct wssEvent evt;
	evt.done = FALSE;
	evt.waiter = MakeThread();
	evt._this = wss;
	evt.pc = pc;
	EnqueLink( &wss->eventQueue, &evt );
	uv_async_send( &wss->async );

	while( !evt.done )
		WakeableSleep( 250 );

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
		WakeableSleep( 250 );

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
		WakeableSleep( 250 );

}

static void webSockServerEvent( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, size_t msglen ){
	//lprintf( "Received:%p %d", buffer, binary );
	wssiObject *wssi = (wssiObject*)psv;
	struct wssiEvent evt;
	evt.send = NULL;
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
		WakeableSleep( 250 );
	{
		INDEX idx;
		struct pendingSend *send;
		LIST_FORALL( evt.send, idx, struct pendingSend *, send ) {
			if( send->binary )
				WebSocketSendBinary( wssi->pc, send->buffer, send->buflen );
			else
				WebSocketSendText( wssi->pc, send->buffer, send->buflen );
			Deallocate( POINTER, send->buffer );
			Deallocate( struct pendingSend*, send );
		}
	}
}



wssObject::wssObject( struct wssOptions *opts ) {
	char tmp[256];
	if( !opts->url ) {
		//snprintf( tmp, 256, "ws://[::]:%d/", opts->port ? opts->port : 8080 );
		snprintf( tmp, 256, "ws://0.0.0.0:%d/", opts->port ? opts->port : 8080 );
		opts->url = tmp;
	}
	closed = 0;
	pc = WebSocketCreate( opts->url, webSockServerOpen, webSockServerEvent, webSockServerClosed, webSockServerError, (uintptr_t)this );
	eventQueue = CreateLinkQueue();

	uv_async_init( l.loop, &async, wssAsyncMsg );
	async.data = this;
}

wssObject::~wssObject() {
	RemoveClient( pc );
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
	}
}

void wssObject::onConnect( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.Holder() );
	if( args.Length() > 0 ) {
		Local<Function> cb = Handle<Function>::Cast( args[0] );
		obj->openCallback.Reset( isolate, cb );
	}
}

wssiObject::wssiObject( PCLIENT pc, Local<Object> obj ) {
	eventQueue = CreateLinkQueue();
	this->pc = pc;
	this->closed = 0;
	uv_async_init( l.loop, &async, wssiAsyncMsg );
	async.data = this;
	this->Wrap( obj );
}

wssiObject::~wssiObject() {
	if( !closed )
		RemoveClient( pc );
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
	RemoveClient( obj->pc );
}

void wssiObject::write( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
	if( args[0]->IsArrayBuffer() ) {
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



static uintptr_t webSockClientOpen( PCLIENT pc, uintptr_t psv ) {
	wscObject *wsc = (wscObject*)psv;

	struct wscEvent evt;
	evt.done = FALSE;
	evt.waiter = MakeThread();
	evt._this = wsc;
	EnqueLink( &wsc->eventQueue, &evt );
	uv_async_send( &wsc->async );
	while( !evt.done )
		WakeableSleep( 250 );

	return (uintptr_t)psv;
}


static void webSockClientClosed( PCLIENT pc, uintptr_t psv )
{

}

static void webSockClientError( PCLIENT pc, uintptr_t psv, int error ) {

}

static void webSockClientEvent( PCLIENT pc, uintptr_t psv, LOGICAL type, CPOINTER buffer, size_t msglen ) {

}



wscObject::wscObject( wscOptions *opts ) {
	pc = WebSocketOpen( opts->url, 0
		, webSockClientOpen
		, webSockClientEvent, webSockClientClosed, webSockClientError, (uintptr_t)this, opts->protocol );
	eventQueue = CreateLinkQueue();
	closed = 0;
	uv_async_init( l.loop, &async, wscAsyncMsg );
	async.data = this;
}

wscObject::~wscObject() {
	if( !closed )
		RemoveClient( pc );
}


void wscObject::New(const FunctionCallbackInfo<Value>& args){
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must specify url and optionally protocols or options for client." ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		String::Utf8Value url( args[0]->ToString() );

		Local<Object> opts = args[1]->ToObject();
		struct wscOptions wscOpts;

		wscObject* obj = new wscObject( &wscOpts );
		Handle<Function> arg0 = Handle<Function>::Cast( args[0] );
		obj->messageCallback.Reset( isolate, arg0 );

		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
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
		}
	}
}



void wscObject::close( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
}

void wscObject::write( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
}


