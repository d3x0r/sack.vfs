
#include "global.h"

struct optionStrings {
	Isolate *isolate;

	Eternal<String> *portString;
	Eternal<String> *addressString;
	Eternal<String> *familyString;
	Eternal<String> *v4String;
	Eternal<String> *v6String;
	Eternal<String> *broadcastString;
	Eternal<String> *messageString;
	Eternal<String> *closeString;

	Eternal<String> *toPortString;
	Eternal<String> *toAddressString;

};

struct udpOptions {
	int port;
	char *address;
	bool reuseAddr;
	bool broadcast;
	int toPort;
	char *toAddress;
	bool addressDefault;
	bool v6;
	Persistent<Function, CopyablePersistentTraits<Function>> messageCallback;
};

class addrObject : public node::ObjectWrap {
public:
	static Persistent<Function> constructor;
	char *address;  // to later lookup cached address....
	int port;
	SOCKADDR *addr;
	addrObject( char *address, int port );
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	~addrObject();
};

// web sock server Object
class udpObject : public node::ObjectWrap {
public:
	LOGICAL closed;
	PCLIENT pc;
	POINTER buffer;
	Persistent<Object> _this;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE eventQueue;
	static Persistent<Function> constructor;
	Persistent<Function, CopyablePersistentTraits<Function>> messageCallback; 
	Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; 
	struct udpEvent *eventMessage;

public:

	udpObject( struct udpOptions *opts );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void send( const v8::FunctionCallbackInfo<Value>& args );
	static void setBroadcast( const FunctionCallbackInfo<Value>& args );

	~udpObject();
};

enum udpEvents {
	UDP_EVENT_READ,
	UDP_EVENT_CLOSE,
};

struct udpEvent {
	enum udpEvents eventType;
	class udpObject *_this;
	CPOINTER buf;
	size_t buflen;
	LOGICAL binary;
};
typedef struct udpEvent UDP_EVENT;
#define MAXUDP_EVENTSPERSET 128
DeclareSet( UDP_EVENT );

Persistent<Function> udpObject::constructor;
Persistent<Function> addrObject::constructor;

static struct local {
	int data;
	uv_loop_t* loop;
	PLIST strings;
	PUDP_EVENTSET *udpEvents;
} l;


static struct optionStrings *getStrings( Isolate *isolate ) {
	INDEX idx;
	struct optionStrings * check;
	LIST_FORALL( l.strings, idx, struct optionStrings *, check ) {
		if( check->isolate == isolate )
			break;
	}
	if( !check ) {
		check = NewArray( struct optionStrings, 1 );
		AddLink( &l.strings, check );
		check->isolate = isolate;
		check->portString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "port" ) );
		check->addressString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "address" ) );
		check->broadcastString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "broadcast" ) );
		check->messageString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "message" ) );
		check->closeString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "close" ) );
		check->familyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "family" ) );
		check->v4String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "IPv4" ) );
		check->v6String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "IPv6" ) );
		check->toPortString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "toPort" ) );
		check->toAddressString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "toAddress" ) );
	}
	return check;
}
void InitUDPSocket( Isolate *isolate, Handle<Object> exports ) {
	if( !l.loop )
		l.loop = uv_default_loop();

	Local<Object> o = Object::New( isolate );
	SET_READONLY( exports, "dgram", o );
	Local<Object> oNet = Object::New( isolate );
	SET_READONLY( exports, "Network", oNet );

	{
		Local<FunctionTemplate> udpTemplate;
		udpTemplate = FunctionTemplate::New( isolate, udpObject::New );
		udpTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.dgram.socket" ) );
		udpTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "close", udpObject::close );
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "on", udpObject::on );
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "send", udpObject::send );
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "setBroadcast", udpObject::setBroadcast );
		udpTemplate->ReadOnlyPrototype();

		udpObject::constructor.Reset( isolate, udpTemplate->GetFunction() );

		SET_READONLY( o, "Socket", udpTemplate->GetFunction() );
		SET_READONLY( o, "createSocket", udpTemplate->GetFunction() );
	}
	{
		Local<FunctionTemplate> addrTemplate;
		addrTemplate = FunctionTemplate::New( isolate, addrObject::New );
		addrTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.network.address" ) );
		addrTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		addrTemplate->ReadOnlyPrototype();

		addrObject::constructor.Reset( isolate, addrTemplate->GetFunction() );

		SET_READONLY( oNet, "Address", addrTemplate->GetFunction() );
	}
}

static void udpAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	udpObject* obj = (udpObject*)handle->data;
	udpEvent *eventMessage;
	HandleScope scope( isolate );

	{
		Local<Value> argv[2];
		while( eventMessage = (struct udpEvent*)DequeLink( &obj->eventQueue ) ) {
			Local<Function> cb;
			Local<ArrayBuffer> ab;
			switch( eventMessage->eventType ) {
			case UDP_EVENT_READ:
				size_t length;
				if( eventMessage->binary ) {
					ab =
						ArrayBuffer::New( isolate,
						(void*)eventMessage->buf,
							length = eventMessage->buflen );
					argv[0] = ab;
					obj->messageCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
				}
				else {
					MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
					argv[0] = buf.ToLocalChecked();
					obj->messageCallback.Get( isolate )->Call( eventMessage->_this->_this.Get( isolate ), 1, argv );
				}
				Deallocate( CPOINTER, eventMessage->buf );
				break;
			case UDP_EVENT_CLOSE:
				cb = Local<Function>::New( isolate, obj->closeCallback );
				if( !cb.IsEmpty() )
					cb->Call( eventMessage->_this->_this.Get( isolate ), 0, argv );
				uv_close( (uv_handle_t*)&obj->async, NULL );
				DeleteLinkQueue( &obj->eventQueue );
				break;
			}
			DeleteFromSet( UDP_EVENT, l.udpEvents, eventMessage );
		}
	}
}


static void CPROC ReadComplete( uintptr_t psv, CPOINTER buffer, size_t buflen, SOCKADDR *from ) {
	udpObject *_this = (udpObject*)psv;
	if( !buffer ) {
	}
	else {

		struct udpEvent *pevt = GetFromSet( UDP_EVENT, &l.udpEvents );
		(*pevt).eventType = UDP_EVENT_READ;
		(*pevt).buf = NewArray( uint8_t*, buflen );
		memcpy( (POINTER)(*pevt).buf, buffer, buflen );
		(*pevt).buflen = buflen;
		(*pevt).binary = true;
		(*pevt)._this = _this;
		EnqueLink( &_this->eventQueue, pevt );
		uv_async_send( &_this->async );
		doUDPRead( _this->pc, (POINTER)buffer, 4096 );
	}
}

static void CPROC Closed( uintptr_t psv ) {
	udpObject *_this = (udpObject*)psv;
	_this->closed = true;

	struct udpEvent *pevt = GetFromSet( UDP_EVENT, &l.udpEvents );
	(*pevt).eventType = UDP_EVENT_CLOSE;
	(*pevt)._this = _this;
	EnqueLink( &_this->eventQueue, pevt );
	uv_async_send( &_this->async );
}

udpObject::udpObject( struct udpOptions *opts ) {
	SOCKADDR *addr = CreateSockAddress( opts->address, opts->port );
	pc = NULL;
	pc = CPPServeUDPAddrEx( addr, (cReadCompleteEx)ReadComplete, (uintptr_t)this, (cCloseCallback)Closed, (uintptr_t)this, TRUE DBG_SRC );
	if( pc ) {
		buffer = NewArray( uint8_t, 4096 );
		if( opts->toAddress )
			GuaranteeAddr( pc, CreateSockAddress( opts->toAddress, opts->toPort ) );
		if( opts->broadcast )
			UDPEnableBroadcast( pc, TRUE );

		eventQueue = CreateLinkQueue();
		//lprintf( "Init async handle. (wss)" );
		async.data = this;
		uv_async_init( l.loop, &async, udpAsyncMsg );
		doUDPRead( pc, (POINTER)buffer, 4096 );
		if( !opts->messageCallback.IsEmpty() )
			this->messageCallback = opts->messageCallback;
	}

}

udpObject::~udpObject() {
	if( !closed )
		RemoveClient( pc );
}

void udpObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Must specify either type or options for server." ) ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		struct udpOptions udpOpts;
		int argBase = 0;
		udpOpts.address = NULL;
		udpOpts.port = 0;
		udpOpts.broadcast = false;
		udpOpts.addressDefault = false;
		udpOpts.messageCallback.Empty();

		if( args[argBase]->IsString() ) {
			udpOpts.address = StrDup( *String::Utf8Value( args[argBase]->ToString() ) );
			argBase++;
		}
		if( ( args.Length() >= argBase ) && args[argBase]->IsObject() ) {
			Local<Object> opts = args[argBase]->ToObject();

			Local<String> optName;
			struct optionStrings *strings = getStrings( isolate );
			// ---- get port 
			if( !opts->Has( optName = strings->portString->Get( isolate ) ) ) {
				udpOpts.port = 0;
			}
			else {
				udpOpts.port = (int)opts->Get( optName )->ToInteger()->Value();
			}
			// ---- get family
			if( opts->Has( optName = strings->familyString->Get( isolate ) ) ) {
				String::Utf8Value family( opts->Get( optName )->ToString() );
				udpOpts.v6 = (StrCmp( *family, "IPv6" ) == 0);
				if( udpOpts.addressDefault ) {
					Deallocate( char *, udpOpts.address );
					if( udpOpts.v6 )
						udpOpts.address = StrDup( "[::]" );
					else
						udpOpts.address = StrDup( "0.0.0.0" );
				}
			}
			// ---- get address
			if( !opts->Has( optName = strings->addressString->Get( isolate ) ) ) {
				udpOpts.addressDefault = true;
				if( udpOpts.v6 )
					udpOpts.address = StrDup( "[::]" );
				else
					udpOpts.address = StrDup( "0.0.0.0" );
			}
			else {
				udpOpts.address = StrDup( *String::Utf8Value(opts->Get( optName )->ToString()) );
			}
			// ---- get to port 
			if( opts->Has( optName = strings->toPortString->Get( isolate ) ) ) {
				udpOpts.toPort = (int)opts->Get( optName )->ToInteger()->Value();
			}
			else
				udpOpts.toPort = 0;
			// ---- get toAddress
			if( opts->Has( optName = strings->addressString->Get( isolate ) ) ) {
				udpOpts.toAddress = StrDup( *String::Utf8Value( opts->Get( optName )->ToString() ) );
			}
			else 
				udpOpts.toAddress = NULL;
			// ---- get broadcast
			if( opts->Has( optName = strings->broadcastString->Get( isolate ) ) ) {
				udpOpts.broadcast = opts->Get( optName )->ToBoolean()->Value();
			}
			// ---- get message callback
			if( opts->Has( optName = strings->messageString->Get( isolate ) ) ) {
				udpOpts.messageCallback.Reset( isolate, Handle<Function>::Cast( opts->Get( optName ) ) );
			}
			argBase++;
		}

		if( args.Length() >= argBase && args[argBase]->IsFunction() ) {
			Handle<Function> arg0 = Handle<Function>::Cast( args[argBase] );
			udpOpts.messageCallback.Reset( isolate, arg0 );
		}

		Local<Object> _this = args.This();
		udpObject* obj = new udpObject( &udpOpts );
		Deallocate( char*, udpOpts.address );
		Deallocate( char*, udpOpts.toAddress );
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

void udpObject::setBroadcast( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( !args.Length() ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Missing boolean in call to setBroadcast." ) ) ) );
		return;
	}
	udpObject *obj = ObjectWrap::Unwrap<udpObject>( args.This() );
	UDPEnableBroadcast( obj->pc, args[0]->ToBoolean()->Value() );
}

void udpObject::on( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	udpObject *obj = ObjectWrap::Unwrap<udpObject>( args.Holder() );
	if( args.Length() == 2 ) {
		Isolate* isolate = args.GetIsolate();
		String::Utf8Value event( args[0]->ToString() );
		Local<Function> cb = Handle<Function>::Cast( args[1] );
		if( StrCmp( *event, "message" ) == 0 ) {
			if( cb->IsFunction() )
				obj->messageCallback.Reset( isolate, cb );
		}
		if( StrCmp( *event, "close" ) == 0 ) {
			if( cb->IsFunction() )
				obj->closeCallback.Reset( isolate, cb );
		}
	}

}

void udpObject::close( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	udpObject *obj = ObjectWrap::Unwrap<udpObject>( args.This() );
	RemoveClient( obj->pc );
}

void udpObject::send( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	udpObject *obj = ObjectWrap::Unwrap<udpObject>( args.This() );
	SOCKADDR *dest = NULL;
	if( args.Length() > 1 ) {
		addrObject *obj = ObjectWrap::Unwrap<addrObject>( args[1]->ToObject() );
		if( obj )
			dest = obj->addr;
	}
	if( args[0]->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
		SendUDPEx( obj->pc, ab->GetContents().Data(), ab->ByteLength(), dest );
	}
	else if( args[0]->IsString() ) {
		String::Utf8Value buf( args[0]->ToString() );
		SendUDPEx( obj->pc, *buf, buf.length(), dest );
	}
	else {
		lprintf( "Unhandled message format" );
	}
}


addrObject::addrObject( char *address, int port ) {
	this->address = address;
	this->port = port;
	addr = CreateSockAddress( address, port );
}

addrObject::~addrObject() {
	ReleaseAddress( addr );
}

void addrObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Must specify address string to create an address." ) ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		struct udpOptions udpOpts;
		int argBase = 0;
		char *address = NULL;
		int port = 0;

		address = StrDup( *String::Utf8Value( args[argBase]->ToString() ) );
		argBase++;

		if( (args.Length() >= argBase) && args[argBase]->IsNumber() ) {
			port = args[argBase]->ToInt32()->Value();
		}

		Local<Object> _this = args.This();
		addrObject* obj = new addrObject( address, port );

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

