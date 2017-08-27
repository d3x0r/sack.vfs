
#include "global.h"

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

struct wssEvent {
	PCLIENT pc;
	class wssObject *_this;
	PTHREAD waiter;
	LOGICAL done;
	class wssiObject *result;
};

struct wssiEvent {
	enum wsEvents eventType;
	Persistent<Object> _this;
	POINTER buf;
	size_t buflen;
	PTHREAD waiter;
	LOGICAL done;
};

struct wscEvent {
	enum wsEvents eventType;
	class wscObject *_this;
	POINTER buf;
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
	PCLIENT pc;
	char *hostUrl;
public:
	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
	LOGICAL closed;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	static Persistent<Function> constructor;
	Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	
public:

	static void Init( Handle<Object> exports );
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
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
public:
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

	static void Init( Handle<Object> exports );
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
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
public:
	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
	PCLIENT pc;
	LOGICAL closed;
public:
	static Persistent<Function> constructor;
	Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> messageCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; //
	
public:

	static void Init( Handle<Object> exports );
	wssiObject( PCLIENT pc, Local<Object> _this );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void write( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );

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
			Local<Object> wssi = Nan::NewInstance( cons, 0, argv ).ToLocalChecked();
			
			wssiObject *wssiInternal = new wssiObject( msg->pc, wssi );
			wssiInternal->_this.Reset( isolate, wssi );

			argv[0] = wssi;
			Local<Function> cb = Local<Function>::New( isolate, myself->openCallback );
			cb->Call( msg->_this->_this.Get(isolate), 1, argv );

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
			switch( eventMessage->eventType ) {
			case WS_EVENT_READ:
				size_t length;
				ab =
					ArrayBuffer::New( isolate,
						eventMessage->buf,
						length = eventMessage->buflen );
				argv[0] = ab;
				myself->messageCallback.Get( isolate )->Call( eventMessage->_this.Get(isolate), 1, argv );
				break;
			case WS_EVENT_CLOSE:
				myself->closeCallback.Get( isolate )->Call( eventMessage->_this.Get( isolate ), 0, argv );
				break;
			case WS_EVENT_ERROR:
				myself->errorCallback.Get( isolate )->Call( eventMessage->_this.Get( isolate ), 0, argv );
				break;
			}
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
		struct wssiEvent* eventMessage;
		while( eventMessage = (struct wssiEvent*)DequeLink( &myself->eventQueue ) ) {
			Local<Function> cb;
			Local<ArrayBuffer> ab;
			switch( eventMessage->eventType ) {
			case WS_EVENT_OPEN:
				cb = Local<Function>::New( isolate, myself->openCallback );
				cb->Call( eventMessage->_this.Get(isolate), 0, argv );
				break;
			case WS_EVENT_READ:
				size_t length;
				ab = ArrayBuffer::New( isolate,
						eventMessage->buf,
						length = eventMessage->buflen );
				cb = Local<Function>::New( isolate, myself->messageCallback );
				argv[1] = ab;
				cb->Call( eventMessage->_this.Get( isolate ), 1, argv );
				break;
			case WS_EVENT_CLOSE:
				cb = Local<Function>::New( isolate, myself->closeCallback );
				cb->Call( eventMessage->_this.Get( isolate ), 0, argv );
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


	Local<Object> o = Object::New( isolate );
	SET_READONLY( exports, "WebSock", o );

	{
		Local<FunctionTemplate> wssTemplate;
		wssTemplate = FunctionTemplate::New( isolate, wssObject::New );
		wssTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.ws.server" ) );
		wssTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "close", wssObject::close );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "on", wssObject::on );
		NODE_SET_PROTOTYPE_METHOD( wssTemplate, "onconnect", wssObject::onConnect );
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
		
		wssiObject::constructor.Reset( isolate, wssiTemplate->GetFunction() );
		//SET_READONLY( o, "Client", wscTemplate->GetFunction() );
	}
}

static uintptr_t webSockServerOpen( PCLIENT pc, uintptr_t psv ){
	wssObject *wss = (wssObject*)psv;

	struct wssEvent evt;
	evt.done = FALSE;
	evt.waiter = MakeThread();
	evt.pc = pc;
	evt._this = wss;
	EnqueLink( &wss->eventQueue, &evt );
	while( !evt.done )
		WakeableSleep( 250 );

	return (uintptr_t)evt.result;
}


static void webSockServerClosed( PCLIENT pc, uintptr_t psv )
{

}

static void webSockServerError( PCLIENT pc, uintptr_t psv, int error ){

}

static void webSockServerEvent( PCLIENT pc, uintptr_t psv, CPOINTER buffer, size_t msglen ){

}



wssObject::wssObject( struct wssOptions *opts ) {
	pc = WebSocketCreate( hostUrl, webSockServerOpen, webSockServerEvent, webSockServerClosed, webSockServerError, (uintptr_t)this );
	eventQueue = CreateLinkQueue();

	uv_async_init( l.loop, &async, wssAsyncMsg );
	async.data = this;
}

wssObject::~wssObject() {
	RemoveClient( pc );
	Deallocate( char*, hostUrl);
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

		Local<Object> opts = args[0]->ToObject();

		Local<String> optName;
		struct optionStrings *strings = getStrings( isolate );
		if( !opts->Has( optName = strings->portString->Get( isolate ) ) ) {
			wssOpts.port = 8080;
		}else {
			wssOpts.port = (int)opts->Get( optName )->ToInteger()->Value();
		}

		wssObject* obj = new wssObject( &wssOpts );
		Handle<Function> arg0 = Handle<Function>::Cast( args[0] );
		Persistent<Function> cb( isolate, arg0 );
		obj->openCallback.Reset( isolate, cb);
		
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

void wssObject::close( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
}

void wssObject::on( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
}

void wssObject::onConnect( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssObject *obj = ObjectWrap::Unwrap<wssObject>( args.This() );
}

wssiObject::wssiObject( PCLIENT pc, Local<Object> obj ) {
	eventQueue = CreateLinkQueue();

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
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must specify options for server." ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		args.GetReturnValue().Set( args.This() );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Value> *argv = new Local<Value>[args.Length()];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( Nan::NewInstance( cons, argc, argv ).ToLocalChecked() );
		delete[] argv;
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


void wssiObject::on( const FunctionCallbackInfo<Value>& args){
	Isolate* isolate = args.GetIsolate();

	if( args.Length() == 2 ) {
		wscObject *obj = ObjectWrap::Unwrap<wscObject>( args.This() );
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


static uintptr_t webSockClientOpen( PCLIENT pc, uintptr_t psv ) {
	wscObject *wsc = (wscObject*)psv;

	struct wscEvent evt;
	evt.done = FALSE;
	evt.waiter = MakeThread();
	evt._this = wsc;
	EnqueLink( &wsc->eventQueue, &evt );
	while( !evt.done )
		WakeableSleep( 250 );

	return (uintptr_t)psv;
}


static void webSockClientClosed( PCLIENT pc, uintptr_t psv )
{

}

static void webSockClientError( PCLIENT pc, uintptr_t psv, int error ) {

}

static void webSockClientEvent( PCLIENT pc, uintptr_t psv, CPOINTER buffer, size_t msglen ) {

}



wscObject::wscObject( wscOptions *opts ) {
	pc = WebSocketOpen( opts->url, 0
		, webSockClientOpen
		, webSockClientEvent, webSockClientClosed, webSockClientError, (uintptr_t)this, opts->protocol );
	eventQueue = CreateLinkQueue();

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

void wssiObject::close( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
}

void wssiObject::write( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	wssiObject *obj = ObjectWrap::Unwrap<wssiObject>( args.This() );
}

