
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
	Eternal<String> *readStringsString;
	Eternal<String> *reuseAddrString;
	Eternal<String> *reusePortString;
};

struct udpOptions {
	Isolate *isolate;
	int port;
	char *address;
	bool reuseAddr;
	bool reusePort;
	bool broadcast;
	int toPort;
	char *toAddress;
	bool addressDefault;
	bool v6;
	bool readStrings;
	Persistent<Function, CopyablePersistentTraits<Function>> messageCallback;
};

struct addrFinder {
	char *addr;
	int port;
};

class addrObject : public node::ObjectWrap {
public:
	struct addrFinder key;
	SOCKADDR *addr;
	Persistent<Object> _this;
	addrObject( char *address, int port );
	addrObject();
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	//static void getAddress( const v8::FunctionCallbackInfo<Value>& args );
	//static void getAddress(	Local<String> property, const PropertyCallbackInfo<Value>& info );
	//static void getPort( Local<String> property, const PropertyCallbackInfo<Value>& info );
	//static void getFamily( Local<String> property, const PropertyCallbackInfo<Value>& info );
	//static void toString( const v8::FunctionCallbackInfo<Value>& args );
	static addrObject *internalNew( Isolate *isolate, SOCKADDR *sa );
	static addrObject *internalNew( Isolate *isolate, Local<Object> *_this );

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
	bool readStrings;  // return a string instead of a buffer
//	static Persistent<Function> constructor;
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
	SOCKADDR *from;
};
typedef struct udpEvent UDP_EVENT;
#define MAXUDP_EVENTSPERSET 128
DeclareSet( UDP_EVENT );

static struct local {
	int data;
	PLIST strings;
	PUDP_EVENTSET udpEvents;
	BinaryTree::PTREEROOT addresses;
	BinaryTree::PTREEROOT addressesBySA;
} l;

static int addrSACompare( uintptr_t oldnode, uintptr_t newnode ) {
	SOCKADDR *oldAddr = (SOCKADDR*)oldnode;
	SOCKADDR *newAddr = (SOCKADDR*)newnode;
	if( oldAddr->sa_family > newAddr->sa_family )
		return 1;
	if( oldAddr->sa_family < newAddr->sa_family )
		return -1;
#define SOCKADDR_LENGTH(sa) ( (int)*(uintptr_t*)( ( (uintptr_t)(sa) ) - 2*sizeof(uintptr_t) ) )

	size_t len = SOCKADDR_LENGTH( oldAddr );
	//lprintf( "compare %d", len );
	return memcmp( oldAddr->sa_data, newAddr->sa_data, len );
}

static int addrCompare( uintptr_t oldnode, uintptr_t newnode ) {
	addrFinder *oldAddr = (addrFinder*)oldnode;
	addrFinder *newAddr = (addrFinder*)newnode;
	if( oldAddr->port < newAddr->port )
		return -1;
	if( oldAddr->port > newAddr->port )
		return -1;
	return StrCaseCmp( oldAddr->addr, newAddr->addr );
}

static void addrDestroy( CPOINTER user, uintptr_t key ) {
	addrObject *oldAddr = (addrObject*)key;
	Deallocate( char*, oldAddr->key.addr );
}

static addrObject *getAddress( Isolate *isolate, char *addr, int port ) {
	if( !l.addresses ) {
		l.addresses = CreateBinaryTreeEx( addrCompare, addrDestroy );
		l.addressesBySA = CreateBinaryTreeEx( addrSACompare, NULL );
	}
	addrFinder finder;
	finder.addr = addr;
	finder.port = port;
	addrObject * obj = (addrObject *)FindInBinaryTree( l.addresses, (uintptr_t)&finder );
	if( !obj ) {
		Local<Object> o;
		obj = addrObject::internalNew( isolate, &o );
		obj->key.addr = addr;
		obj->key.port = port;
		obj->addr = CreateSockAddress( addr, port );
		uint16_t realPort;
		GetAddressParts( obj->addr, NULL, &realPort );
		AddBinaryNode( l.addressesBySA, obj, (uintptr_t)obj->addr );
		AddBinaryNode( l.addresses, obj, (uintptr_t)&obj->key );
		if( obj->addr )
			SET_READONLY( o, "family", String::NewFromUtf8( isolate, obj->addr->sa_family == AF_INET ? "IPv4" : obj->addr->sa_family == AF_INET6 ? "IPv6" : "unknown", v8::NewStringType::kNormal ).ToLocalChecked() );
		else
			SET_READONLY( o, "family", Undefined(isolate) );
		SET_READONLY( o, "address", String::NewFromUtf8( isolate, addr, v8::NewStringType::kNormal ).ToLocalChecked() );
		if( obj->addr )
			SET_READONLY( o, "IP", String::NewFromUtf8( isolate, GetAddrString( obj->addr ), v8::NewStringType::kNormal ).ToLocalChecked() );
		else
			SET_READONLY( o, "IP", Undefined( isolate ) );
		SET_READONLY( o, "port", Number::New( isolate, realPort ) );

	}
	return obj;
}

static Local<Object> getAddressBySA( Isolate *isolate, SOCKADDR *sa ) {
	if( !l.addresses ) {
		l.addresses = CreateBinaryTreeEx( addrCompare, addrDestroy );
		l.addressesBySA = CreateBinaryTreeEx( addrSACompare, NULL );
	}
	addrObject * obj = (addrObject *)FindInBinaryTree( l.addressesBySA, (uintptr_t)sa );
	if( !obj ) {
		obj = addrObject::internalNew( isolate, sa );
		AddBinaryNode( l.addressesBySA, obj, (uintptr_t)obj->addr );
		AddBinaryNode( l.addresses, obj, (uintptr_t)&obj->key );
	}
	return obj->_this.Get( isolate );
}


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
		check->portString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "port", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->addressString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "address", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->broadcastString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "broadcast", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->messageString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "message", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->closeString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "close", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->familyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "family", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->v4String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "IPv4", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->v6String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "IPv6", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->toPortString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "toPort", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->toAddressString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "toAddress", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->readStringsString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "readStrings", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->reusePortString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "reusePort", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->reuseAddrString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "reuseAddress", v8::NewStringType::kNormal ).ToLocalChecked() );
	}
	return check;
}
void InitUDPSocket( Isolate *isolate, Local<Object> exports ) {

	Local<Object> oNet = Object::New( isolate );
	SET_READONLY( exports, "Network", oNet );
	class constructorSet *c = getConstructors(isolate); 
	{
		Local<FunctionTemplate> udpTemplate;
		udpTemplate = FunctionTemplate::New( isolate, udpObject::New );
		udpTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.dgram.socket", v8::NewStringType::kNormal ).ToLocalChecked() );
		udpTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "close", udpObject::close );
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "on", udpObject::on );
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "send", udpObject::send );
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "setBroadcast", udpObject::setBroadcast );
		udpTemplate->ReadOnlyPrototype();

		c->udpConstructor.Reset( isolate, udpTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

		SET_READONLY( oNet, "UDP", udpTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	}
	{
		Local<FunctionTemplate> addrTemplate;
		addrTemplate = FunctionTemplate::New( isolate, addrObject::New );
		c->addrTpl.Reset( isolate, addrTemplate );
		addrTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.network.address", v8::NewStringType::kNormal ).ToLocalChecked() );
		addrTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		//NODE_SET_PROTOTYPE_METHOD( addrTemplate, "toString", addrObject::toString );
		addrTemplate->ReadOnlyPrototype();

		c->addrConstructor.Reset( isolate, addrTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

		SET_READONLY( oNet, "Address", addrTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	}
}

void FreeCallback( char* data, void* hint ) {
	Deallocate( char*, data );
}

static void udpAsyncMsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	udpObject* obj = (udpObject*)handle->data;
	udpEvent *eventMessage;

	{
		Local<Value> argv[2];
		while( eventMessage = (struct udpEvent*)DequeLink( &obj->eventQueue ) ) {
			Local<Function> cb;
			Local<Object> ab;
			switch( eventMessage->eventType ) {
			case UDP_EVENT_READ:
				argv[1] = ::getAddressBySA( isolate, eventMessage->from );
				if( !obj->readStrings ) {
					ab = ArrayBuffer::New( isolate, (POINTER)eventMessage->buf, eventMessage->buflen );

					PARRAY_BUFFER_HOLDER holder = GetHolder();
					holder->o.Reset( isolate, ab );
					holder->o.SetWeak< ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
					holder->buffer = (void*)eventMessage->buf;

					argv[0] = ab;
					obj->messageCallback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 2, argv );
				}
				else {
					MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
					//lprintf( "built string from %p", eventMessage->buf );
					argv[0] = buf.ToLocalChecked();
					obj->messageCallback.Get( isolate )->Call( context, eventMessage->_this->_this.Get( isolate ), 2, argv );
					Deallocate( CPOINTER, eventMessage->buf );
				}
				ReleaseAddress( eventMessage->from );
				break;
			case UDP_EVENT_CLOSE:
				cb = Local<Function>::New( isolate, obj->closeCallback );
				if( !cb.IsEmpty() )
					cb->Call( context, eventMessage->_this->_this.Get( isolate ), 0, argv );
				uv_close( (uv_handle_t*)&obj->async, NULL );
				DeleteLinkQueue( &obj->eventQueue );
				break;
			}
			DeleteFromSet( UDP_EVENT, l.udpEvents, eventMessage );
		}
	}
	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}


static void CPROC ReadComplete( uintptr_t psv, CPOINTER buffer, size_t buflen, SOCKADDR *from ) {
	udpObject *_this = (udpObject*)psv;
	if( !buffer ) {
		// skip init read; buffer is allocated later and then this callback is triggered
	}
	else {
		struct udpEvent *pevt = GetFromSet( UDP_EVENT, &l.udpEvents );
		(*pevt).eventType = UDP_EVENT_READ;
		(*pevt).buf = NewArray( uint8_t*, buflen );
		//lprintf( "Send buffer %p", (*pevt).buf );
		memcpy( (POINTER)(*pevt).buf, buffer, buflen );
		(*pevt).buflen = buflen;
		(*pevt)._this = _this;
		(*pevt).from = DuplicateAddress( from );
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
	NetworkWait( NULL, 256, 2 );  // 1GB memory

	pc = NULL;
	pc = CPPServeUDPAddrEx( addr, (cReadCompleteEx)ReadComplete, (uintptr_t)this, (cCloseCallback)Closed, (uintptr_t)this, TRUE DBG_SRC );
	if( pc ) {
		buffer = NewArray( uint8_t, 4096 );
		if( opts->toAddress )
			GuaranteeAddr( pc, CreateSockAddress( opts->toAddress, opts->toPort ) );
		if( opts->broadcast )
			UDPEnableBroadcast( pc, TRUE );
		if( opts->reuseAddr )
			SetSocketReuseAddress( pc, TRUE );
		if( opts->reusePort )
			SetSocketReusePort( pc, TRUE );
		this->readStrings = opts->readStrings;
		eventQueue = CreateLinkQueue();
		//lprintf( "Init async handle. (wss)" );
		async.data = this;
		class constructorSet *c = getConstructors( opts->isolate );
		uv_async_init( c->loop, &async, udpAsyncMsg );
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
	Local<Context> context = isolate->GetCurrentContext();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Must specify either type or options for server." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		struct udpOptions udpOpts;
		int argBase = 0;
		udpOpts.isolate= isolate;
		udpOpts.readStrings = false;
		udpOpts.address = NULL;
		udpOpts.port = 0;
		udpOpts.broadcast = false;
		udpOpts.addressDefault = false;
		udpOpts.messageCallback.Empty();

		if( args[argBase]->IsString() ) {
			udpOpts.address = StrDup( *String::Utf8Value( isolate,  args[argBase]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ) );
			argBase++;
		}
		if( ( args.Length() >= argBase ) && args[argBase]->IsObject() ) {
			Local<Object> opts = args[argBase]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();

			Local<String> optName;
			struct optionStrings *strings = getStrings( isolate );
			// ---- get port
			if( !opts->Has( context, optName = strings->portString->Get( isolate ) ).ToChecked() ) {
				udpOpts.port = 0;
			}
			else {
				udpOpts.port = (int)GETV( opts, optName )->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
			}
			// ---- get family
			if( opts->Has( context, optName = strings->familyString->Get( isolate ) ).ToChecked() ) {
				String::Utf8Value family( isolate,  GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
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
			if( !opts->Has( context, optName = strings->addressString->Get( isolate ) ).ToChecked() ) {
				udpOpts.addressDefault = true;
				if( udpOpts.v6 )
					udpOpts.address = StrDup( "[::]" );
				else
					udpOpts.address = StrDup( "0.0.0.0" );
			}
			else {
				udpOpts.address = StrDup( *String::Utf8Value( isolate,  GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked()) );
			}
			// ---- get to port
			if( opts->Has( context, optName = strings->toPortString->Get( isolate ) ).ToChecked() ) {
				udpOpts.toPort = (int)GETV( opts, optName )->ToInteger(isolate->GetCurrentContext()).ToLocalChecked()->Value();
			}
			else
				udpOpts.toPort = 0;
			// ---- get toAddress
			if( opts->Has( context, optName = strings->addressString->Get( isolate ) ).ToChecked() ) {
				udpOpts.toAddress = StrDup( *String::Utf8Value( isolate,  GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ) );
			}
			else
				udpOpts.toAddress = NULL;
			// ---- get broadcast
			if( opts->Has( context, optName = strings->broadcastString->Get( isolate ) ).ToChecked() ) {
				udpOpts.broadcast = GETV( opts, optName )->ToBoolean( isolate )->Value();
			}
			// ---- get message callback
			if( opts->Has( context, optName = strings->messageString->Get( isolate ) ).ToChecked() ) {
				udpOpts.messageCallback.Reset( isolate, Local<Function>::Cast( GETV( opts, optName ) ) );
			}
			// ---- get read strings setting
			if( opts->Has( context, optName = strings->readStringsString->Get( isolate ) ).ToChecked() ) {
				udpOpts.readStrings = GETV( opts, optName )->ToBoolean( isolate )->Value();
			}
			// ---- get reuse address
			if( opts->Has( context, optName = strings->reuseAddrString->Get( isolate ) ).ToChecked() ) {
				udpOpts.reuseAddr = GETV( opts, optName )->ToBoolean( isolate )->Value();
			}
			else udpOpts.reuseAddr = false;
			// ---- get reuse port
			if( opts->Has( context, optName = strings->reusePortString->Get( isolate ) ).ToChecked() ) {
				udpOpts.reusePort = GETV( opts, optName )->ToBoolean( isolate )->Value();
			}
			else udpOpts.reusePort = false;
			argBase++;
		}

		if( args.Length() >= argBase && args[argBase]->IsFunction() ) {
			Local<Function> arg0 = Local<Function>::Cast( args[argBase] );
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

		class constructorSet *c = getConstructors(isolate);
		Local<Function> cons = Local<Function>::New( isolate, c->udpConstructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete[] argv;
	}
}

void udpObject::setBroadcast( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( !args.Length() ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Missing boolean in call to setBroadcast." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	udpObject *obj = ObjectWrap::Unwrap<udpObject>( args.This() );
	UDPEnableBroadcast( obj->pc, args[0]->ToBoolean( isolate )->Value() );
}

void udpObject::on( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	udpObject *obj = ObjectWrap::Unwrap<udpObject>( args.Holder() );
	if( args.Length() == 2 ) {
		Isolate* isolate = args.GetIsolate();
		String::Utf8Value event( isolate,  args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		Local<Function> cb = Local<Function>::Cast( args[1] );
		if( StrCmp( *event, "error" ) == 0 ) {
			// not sure how to get this... so many errors so few callbacks
		}
		else if( StrCmp( *event, "message" ) == 0 ) {
			if( cb->IsFunction() )
				obj->messageCallback.Reset( isolate, cb );
		}
		else if( StrCmp( *event, "close" ) == 0 ) {
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
	if( !obj->pc ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Socket is not open." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	SOCKADDR *dest = NULL;
	if( args.Length() > 1 ) {
		class constructorSet *c = getConstructors( isolate );
		Local<FunctionTemplate> tpl = c->addrTpl.Get( isolate );
		Local<Object> argObj = args[1]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
		if( !argObj.IsEmpty() && tpl->HasInstance( argObj ) ) {
			addrObject *obj = ObjectWrap::Unwrap<addrObject>( args[1]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() );
			if( obj )
				dest = obj->addr;
		}
		else {
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Address argument is not a sack.core.Network.Address" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
			return;
		}
	}
	if( args[0]->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
		SendUDPEx( obj->pc, ab->GetContents().Data(), ab->ByteLength(), dest );
	}
	else if( args[0]->IsString() ) {
		String::Utf8Value buf( isolate,  args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		SendUDPEx( obj->pc, *buf, buf.length(), dest );
	}
	else {
		lprintf( "Unhandled message format" );
	}
}


addrObject::addrObject( char *address, int port ) {
	this->key.addr = address;
	this->key.port = port;
	addr = CreateSockAddress( address, port );
}

addrObject::addrObject(  ) {

}

addrObject::~addrObject() {
	ReleaseAddress( addr );
}

addrObject *addrObject::internalNew( Isolate *isolate, SOCKADDR *sa ) {
		class constructorSet *c = getConstructors(isolate);
	Local<Function> cons = Local<Function>::New( isolate, c->addrConstructor );
	Local<Value> args[1];
	MaybeLocal<Object> __addr = cons->NewInstance( isolate->GetCurrentContext(), 0, args );
	Local<Object> _addr = __addr.ToLocalChecked();
	addrObject *addr = ObjectWrap::Unwrap<addrObject>(_addr);
	uint16_t port;
	GetAddressParts( sa, NULL, &port );
	addr->key.port = port;
	addr->key.addr = (char*)GetAddrName( sa );
	addr->addr = DuplicateAddress( sa );
	SET_READONLY( _addr, "family", String::NewFromUtf8( isolate, sa->sa_family == AF_INET ? "IPv4" :sa->sa_family == AF_INET6 ? "IPv6" : "unknown", v8::NewStringType::kNormal ).ToLocalChecked() );
	SET_READONLY( _addr, "address", String::NewFromUtf8( isolate, addr->key.addr, v8::NewStringType::kNormal ).ToLocalChecked() );
	SET_READONLY( _addr, "IP", String::NewFromUtf8( isolate, GetAddrString( addr->addr ), v8::NewStringType::kNormal ).ToLocalChecked() );
	SET_READONLY( _addr, "port", Number::New( isolate, port ) );
	return addr;
}

addrObject *addrObject::internalNew( Isolate *isolate, Local<Object> *_this ) {
	class constructorSet *c = getConstructors(isolate);
	Local<Function> cons = Local<Function>::New( isolate, c->addrConstructor );
	Local<Value> args[1];
	MaybeLocal<Object> _addr = cons->NewInstance( isolate->GetCurrentContext(), 0, args );
	_this[0] = _addr.ToLocalChecked();
	return ObjectWrap::Unwrap<addrObject>( _addr.ToLocalChecked() );
}

void addrObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		int argBase = 0;
		char *address = NULL;
		int port = 0;
		if( !args.Length() )
		{
			Local<Object> _this = args.This();
			addrObject* obj = new addrObject();
			obj->_this.Reset( isolate, _this );
			obj->Wrap( _this );
			args.GetReturnValue().Set( _this );
			return;
		}
		address = StrDup( *String::Utf8Value( isolate,  args[argBase]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ) );
		argBase++;

		if( (args.Length() >= argBase) && args[argBase]->IsNumber() ) {
			port = args[argBase]->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		}

		Local<Object> _this = args.This();
		addrObject* obj = new addrObject( address, port );
		AddBinaryNode( l.addresses, obj, (uintptr_t)&obj->key );
		AddBinaryNode( l.addressesBySA, obj, (uintptr_t)obj->addr );
		struct optionStrings *strings = getStrings( isolate );
		uint16_t realPort;
		GetAddressParts( obj->addr, NULL, &realPort );
		if( obj->addr ) {
			SET_READONLY( _this, "family", String::NewFromUtf8( isolate, obj->addr->sa_family == AF_INET ? "IPv4" : obj->addr->sa_family == AF_INET6 ? "IPv6" : "unknown", v8::NewStringType::kNormal ).ToLocalChecked() );
		}
		else
			SET_READONLY( _this, "family", Undefined( isolate ) );
		SET_READONLY( _this, "address", String::NewFromUtf8( isolate, address, v8::NewStringType::kNormal ).ToLocalChecked() );
		if( obj->addr )
			SET_READONLY( _this, "IP", String::NewFromUtf8( isolate, GetAddrString( obj->addr ), v8::NewStringType::kNormal ).ToLocalChecked() );
		else
			SET_READONLY( _this, "IP", Undefined(isolate) );
		SET_READONLY( _this, "port", Number::New( isolate, realPort ) );

		obj->_this.Reset( isolate, _this );
		obj->Wrap( _this );

		args.GetReturnValue().Set( _this );
	}
	else {
		if( argc == 0 ) {
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Must specify address string to create an address." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
			return;
		}

		int argBase = 0;
		char *address = NULL;
		int port = 0;
		if( !args.Length() )
		{
			Local<Object> _this = args.This();
			addrObject* obj = new addrObject();
			obj->_this.Reset( isolate, _this );
			obj->Wrap( _this );
			return;
		}
		address = StrDup( *String::Utf8Value( isolate,  args[argBase]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ) );
		argBase++;

		if( (args.Length() >= argBase) && args[argBase]->IsNumber() ) {
			port = args[argBase]->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		}
		addrObject *addr = ::getAddress( isolate, address, port );

		args.GetReturnValue().Set( addr->_this.Get( isolate ) );
	}
}
