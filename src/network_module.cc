
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
	Eternal<String> *connectionString;

	Eternal<String>* certString;
	Eternal<String>* caString;
	Eternal<String>* keyString;
	Eternal<String>* sslString;
	Eternal<String>* pemString;
	Eternal<String>* passString;
	Eternal<String>* hostString;
	Eternal<String>* hostsString;
	Eternal<String>* allowSSLfallbackString;

	// object field for connect callback in options
	Eternal<String> *connectString;
	Eternal<String>* readyString;
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
	PERSISTENT_FUNCTION messageCallback;
};

struct tcpHostOption {
	char* host;
	int hostlen;
	char* cert_chain;
	int cert_chain_len;
	char* key;
	int key_len;
	char* pass;
	int pass_len;
};

struct tcpOptions {
	Isolate *isolate;

	// server side options (client side bind option)
	int port;
	char *address;
	bool reuseAddr;
	bool reusePort;
	bool delayConnect;
	bool allowSSLfallback;
	// client side options (else is server)
	int toPort;
	char *toAddress;

	bool addressDefault;
	bool v6;
	bool readStrings;

	bool ssl;
	char* cert_chain;
	int cert_chain_len;
	char* key;
	int key_len;
	char* pass;
	int pass_len;
	char* host;
	PLIST hostList;


	PERSISTENT_FUNCTION connectCallback;
	PERSISTENT_FUNCTION readyCallback;
	PERSISTENT_FUNCTION messageCallback;
	PERSISTENT_FUNCTION closeCallback;
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

	// utility functions?  This is probably a verbose object that all of these are available
	// in the resulting JS object without getting parts.
	//static void getAddress( const v8::FunctionCallbackInfo<Value>& args );
	//static void getAddress(	Local<String> property, const PropertyCallbackInfo<Value>& info );
	//static void getPort( Local<String> property, const PropertyCallbackInfo<Value>& info );
	//static void getFamily( Local<String> property, const PropertyCallbackInfo<Value>& info );
	//static void toString( const v8::FunctionCallbackInfo<Value>& args );
	static addrObject *internalNew( Isolate *isolate, SOCKADDR *sa );
	static addrObject *internalNew( Isolate *isolate, Local<Object> *_this );

	~addrObject();
};

// UDP socket Object
class udpObject : public node::ObjectWrap {
public:
	LOGICAL closed;
	PCLIENT pc;
	POINTER buffer;
	Persistent<Object> _this;
	bool ivm_hosted = false;
	class constructorSet *c;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE eventQueue;
	bool readStrings;  // return a string instead of a buffer
//	static Persistent<Function> constructor;
	PERSISTENT_FUNCTION messageCallback;
	PERSISTENT_FUNCTION closeCallback;
	struct networkEvent *eventMessage;

public:

	udpObject( struct udpOptions *opts );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void send( const v8::FunctionCallbackInfo<Value>& args );
	static void setBroadcast( const FunctionCallbackInfo<Value>& args );
	static void string_get( const FunctionCallbackInfo<Value>& args );
	static void string_set( const FunctionCallbackInfo<Value>& args );

	~udpObject();
};

// TCP socket Object
class tcpObject : public node::ObjectWrap {
	static Isolate *isolate; // temporary initializing new socket...
public:
	LOGICAL closed = false;
	PCLIENT pc = NULL;
	POINTER buffer = NULL;  // pending read buffer?
	bool ssl = false;
	bool allowSSLfallback = true;
	Persistent<Object> _this;
	bool ivm_hosted = false;
	class constructorSet *c;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE eventQueue;
	bool readStrings = false;  // return a string instead of a buffer
	class tcpObject *server = NULL; // if this is an accepted client, this is the server object
	//	static Persistent<Function> constructor;	
	PERSISTENT_FUNCTION messageCallback;
   // if this is a client, this is triggered when the socket completes or fails connection
   // if this is a server, this is a newly accepted connection
	//    messageCallback and closeCallback are automatically copied
   //    a server listen socket(normally) can't close (should check for disconnecting network)
   //    a server will also never get messages.
   //    But the newly accepted socket can also just have their callbacks set.
	PERSISTENT_FUNCTION connectCallback;
	PERSISTENT_FUNCTION readyCallback;
	PERSISTENT_FUNCTION closeCallback;
	struct networkEvent *eventMessage; // probably use the same queue?

public:

	tcpObject( struct tcpOptions *opts );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void send( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );

	static void ssl_get( const v8::FunctionCallbackInfo<Value>& args );
	static void ssl_set( const v8::FunctionCallbackInfo<Value>& args );
	static void string_get( const FunctionCallbackInfo<Value>& args );
	static void string_set( const FunctionCallbackInfo<Value>& args );
	static void ssl_fallback_get( const v8::FunctionCallbackInfo<Value>& args );
	static void ssl_fallback_set( const v8::FunctionCallbackInfo<Value>& args );
	/**
	 * add additional certificates to a server, switched on hostname
	 *  (host,cert,key,keypadd)
	*/
	static void addHost( const FunctionCallbackInfo<Value>& args );

	static class tcpObject* getSelf( Local<Object> _this );
	~tcpObject();
};

Isolate *tcpObject::isolate = NULL;

enum networkEvents {
	NET_EVENT_READ,
	NET_EVENT_CLOSE,
	NET_EVENT_CONNECT,  // server accepcted connection - connect callback
	NET_EVENT_CONNECTED,  // client connected - connect callback
	NET_EVENT_FIRST_READ,  // This socket is now able to write and read
	NET_EVENT_CONNECT_ERROR, // client failed connect - connect callback
};

struct networkEvent {
	enum networkEvents eventType;
	union {
		class udpObject* udp;
		class tcpObject* tcp;
	} _this;
	int error;
	CPOINTER buf;
	size_t buflen;
	SOCKADDR *from;
	int done;
	PTHREAD waiter;
};

typedef struct networkEvent NET_EVENT;
#define MAXNET_EVENTSPERSET 128
DeclareSet( NET_EVENT );

static void TCP_Close( uintptr_t psv );
static void TCP_Write( uintptr_t psv, CPOINTER buffer, size_t length );
static void TCP_Close( uintptr_t psv );
static void TCP_ReadComplete( uintptr_t psv, POINTER buffer, size_t length );


static void getName( const v8::FunctionCallbackInfo<Value>& args );
static void ping( const v8::FunctionCallbackInfo<Value>& args );

static struct local {
	int data;
	PLIST strings;
	PNET_EVENTSET networkEvents;
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
		check->portString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "port" ) );
		check->addressString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "address" ) );
		check->broadcastString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "broadcast" ) );
		check->messageString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "message" ) );
		check->closeString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "close" ) );

		check->familyString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "family" ) );
		check->v4String = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "IPv4" ) );
		check->v6String = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "IPv6" ) );

		check->toPortString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "toPort" ) );
		check->toAddressString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "toAddress" ) );
		check->readStringsString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "readStrings" ) );
		check->reusePortString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "reusePort" ) );
		check->reuseAddrString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "reuseAddress" ) );
		check->connectionString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "connection" ) );

		check->certString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "cert" ) );
		check->sslString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "ssl" ) );
		check->caString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "ca" ) );
		check->keyString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "key" ) );
		check->pemString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "pem" ) );
		check->passString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "passphrase" ) );
		check->hostString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "host" ) );
		check->hostsString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "hosts" ) );
		check->allowSSLfallbackString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "allowSSLfallback" ) );
		check->connectString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "connect" ) );
		check->readyString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "ready" ) );
	}
	return check;
}

void getOpenPorts(  Local<Name> property, const PropertyCallbackInfo<Value>& args ) {
	Isolate *isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Array> result = Array::New( isolate );
	PDATALIST list;
	struct listener_pid_info *info;
	INDEX idx;
	SackNetstat_GetListeners( &list );
	DATA_FORALL( list, idx, struct listener_pid_info*, info ){
		Local<Object> o = Object::New( isolate );
		SET_READONLY( o, "port", Number::New( isolate, info->port ) );
		SET_READONLY( o, "pid", Number::New( isolate, info->pid ) );
		SETN( result, idx, o );
	}
	DeleteDataList( &list );
	args.GetReturnValue().Set( result );
}

void InitUDPSocket( Isolate *isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> oNet = Object::New( isolate );
	SET_READONLY( exports, "Network", oNet );
	class constructorSet *c = getConstructors(isolate); 
	{
		Local<FunctionTemplate> udpTemplate;
		udpTemplate = FunctionTemplate::New( isolate, udpObject::New );
		udpTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.core.dgram.socket" ) );
		udpTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "close", udpObject::close );
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "on", udpObject::on );
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "send", udpObject::send );
		NODE_SET_PROTOTYPE_METHOD( udpTemplate, "setBroadcast", udpObject::setBroadcast );
		udpTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "readStrings" )
			, FunctionTemplate::New( isolate, udpObject::string_get )
			, FunctionTemplate::New( isolate, udpObject::string_set )
		);
		udpTemplate->ReadOnlyPrototype();

		c->udpConstructor.Reset( isolate, udpTemplate->GetFunction(context).ToLocalChecked() );

		SET_READONLY( oNet, "UDP", udpTemplate->GetFunction(context).ToLocalChecked() );
	}
	{
		Local<FunctionTemplate> tcpTemplate;
		tcpTemplate = FunctionTemplate::New( isolate, tcpObject::New );
		tcpTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.core.stream.socket" ) );
		tcpTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( tcpTemplate, "close", tcpObject::close );
		NODE_SET_PROTOTYPE_METHOD( tcpTemplate, "on", tcpObject::on );
		NODE_SET_PROTOTYPE_METHOD( tcpTemplate, "addHost", tcpObject::addHost );
		NODE_SET_PROTOTYPE_METHOD( tcpTemplate, "send", tcpObject::send );
		NODE_SET_PROTOTYPE_METHOD( tcpTemplate, "ssl", tcpObject::on );
		tcpTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "ssl" )
				, FunctionTemplate::New( isolate, tcpObject::ssl_get )
				, FunctionTemplate::New( isolate, tcpObject::ssl_set )
			);
		tcpTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "allowSSLfallback" )
				, FunctionTemplate::New( isolate, tcpObject::ssl_fallback_get )
				, FunctionTemplate::New( isolate, tcpObject::ssl_fallback_set )
			);
		tcpTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "readStrings" )
			, FunctionTemplate::New( isolate, tcpObject::string_get )
			, FunctionTemplate::New( isolate, tcpObject::string_set )
		);
		tcpTemplate->ReadOnlyPrototype();
		Local<Function> tcpObject = tcpTemplate->GetFunction(context).ToLocalChecked();
		c->tcpConstructor.Reset( isolate, tcpObject );

		SET_READONLY( oNet, "TCP", tcpTemplate->GetFunction(context).ToLocalChecked() );
		tcpObject->SetNativeDataProperty( context, String::NewFromUtf8Literal( isolate, "ports" )
			, getOpenPorts
			, nullptr //Local<Function>()
			, Local<Value>()
			, PropertyAttribute::ReadOnly
			, SideEffectType::kHasNoSideEffect
			, SideEffectType::kHasSideEffect
		);
	}

	{
		Local<Object> icmpObject = Object::New( isolate );
		NODE_SET_METHOD( icmpObject, "ping", ping );
		NODE_SET_METHOD( icmpObject, "nameOf", getName );
		
		SET_READONLY( oNet, "ICMP", icmpObject );
	}
	{
		Local<FunctionTemplate> addrTemplate;
		addrTemplate = FunctionTemplate::New( isolate, addrObject::New );
		c->addrTpl.Reset( isolate, addrTemplate );
		addrTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.core.network.address" ) );
		addrTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		//NODE_SET_PROTOTYPE_METHOD( addrTemplate, "toString", addrObject::toString );
		addrTemplate->ReadOnlyPrototype();

		c->addrConstructor.Reset( isolate, addrTemplate->GetFunction(context).ToLocalChecked() );

		SET_READONLY( oNet, "Address", addrTemplate->GetFunction(context).ToLocalChecked() );
	}
}

void FreeCallback( char* data, void* hint ) {
	Deallocate( char*, data );
}

static void udpAsyncMsg_( Isolate *isolate, Local<Context> context, udpObject*obj ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	networkEvent *eventMessage;

	{
		Local<Value> argv[2];
		while( eventMessage = (struct networkEvent*)DequeLink( &obj->eventQueue ) ) {
			Local<Function> cb;
			Local<Object> ab;
			switch( eventMessage->eventType ) {
			case NET_EVENT_READ:
				argv[1] = ::getAddressBySA( isolate, eventMessage->from );
				if( !obj->readStrings ) {
#if ( NODE_MAJOR_VERSION >= 14 )
					std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( (POINTER)eventMessage->buf, eventMessage->buflen, releaseBufferBackingStore, NULL );
					ab = ArrayBuffer::New( isolate, bs );
#else
					ab = ArrayBuffer::New( isolate, (POINTER)eventMessage->buf, eventMessage->buflen );
					PARRAY_BUFFER_HOLDER holder = GetHolder();
					holder->o.Reset( isolate, ab );
					holder->o.SetWeak< ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
					holder->buffer = (void*)eventMessage->buf;
#endif
					argv[0] = ab;
					obj->messageCallback.Get( isolate )->Call( context, eventMessage->_this.udp->_this.Get( isolate ), 2, argv );
				}
				else {
					MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
					//lprintf( "built string from %p", eventMessage->buf );
					argv[0] = buf.ToLocalChecked();
					obj->messageCallback.Get( isolate )->Call( context, eventMessage->_this.udp->_this.Get( isolate ), 2, argv );
					Deallocate( CPOINTER, eventMessage->buf );
				}
				ReleaseAddress( eventMessage->from );
				break;
			case NET_EVENT_CLOSE:
				cb = Local<Function>::New( isolate, obj->closeCallback );
				if( !cb.IsEmpty() )
					cb->Call( context, eventMessage->_this.udp->_this.Get( isolate ), 0, argv );
				//lprintf( "Close async handle: %p",(uv_handle_t*)&obj->async );
				if( !obj->ivm_hosted )
					uv_close( (uv_handle_t *)&obj->async, NULL );
				DeleteLinkQueue( &obj->eventQueue );
				break;
			default:
				lprintf( "Unknown event type passed to udpAsyncMsg: %d", eventMessage->eventType );
				break;
			}
			DeleteFromSet( NET_EVENT, l.networkEvents, eventMessage );
		}
	}
}

static void udpAsyncMsg( uv_async_t *handle ) {
	v8::Isolate *isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	udpObject *obj         = (udpObject *)handle->data;
	udpAsyncMsg_( isolate, context, obj );
	if( !obj->c->ThreadObject_idleProc.IsEmpty() ) {
		Local<Function> cb      = Local<Function>::New( isolate, obj->c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}

struct udpAsyncTask : SackTask {
	udpObject *obj;
	udpAsyncTask( udpObject *obj )
	    : obj( obj ) {}
	void Run2( Isolate *isolate, Local<Context> context ) { udpAsyncMsg_( isolate, context, obj ); }
};

static void CPROC ReadComplete( uintptr_t psv, CPOINTER buffer, size_t buflen, SOCKADDR *from ) {
	udpObject *_this = (udpObject*)psv;
	if( !buffer ) {
		// skip init read; buffer is allocated later and then this callback is triggered
	}
	else {
		struct networkEvent *pevt = GetFromSet( NET_EVENT, &l.networkEvents );
		(*pevt).eventType = NET_EVENT_READ;
		(*pevt).buf = NewArray( uint8_t*, buflen );
		//lprintf( "Send buffer %p", (*pevt).buf );
		memcpy( (POINTER)(*pevt).buf, buffer, buflen );
		(*pevt).buflen = buflen;
		(*pevt)._this.udp = _this;
		(*pevt).from = DuplicateAddress( from );
		(*pevt).waiter = NULL;

		EnqueLink( &_this->eventQueue, pevt );
		if( _this->ivm_hosted )
			_this->c->ivm_post( _this->c->ivm_holder, std::make_unique<udpAsyncTask>( _this ) );
		else
			uv_async_send( &_this->async );
		doUDPRead( _this->pc, (POINTER)buffer, 4096 );
	}
}

static void CPROC Closed( uintptr_t psv ) {
	udpObject *_this = (udpObject*)psv;
	_this->closed = true;

	struct networkEvent *pevt = GetFromSet( NET_EVENT, &l.networkEvents );
	(*pevt).eventType = NET_EVENT_CLOSE;
	(*pevt)._this.udp = _this;
	(*pevt).waiter = NULL;
	EnqueLink( &_this->eventQueue, pevt );
	if( _this->ivm_hosted )
		_this->c->ivm_post( _this->c->ivm_holder, std::make_unique<udpAsyncTask>( _this ) );
	else
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
		this->c                 = c;
		if( c->ivm_holder )
			this->ivm_hosted = true;
		else
			uv_async_init( c->loop, &async, udpAsyncMsg );

		doUDPRead( pc, (POINTER)buffer, 4096 );
		if( !opts->messageCallback.IsEmpty() )
			this->messageCallback.Reset( opts->isolate, opts->messageCallback );
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
		// if it was uninitialized it would fail to reset
		//udpOpts.messageCallback.Reset();

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
	//Isolate* isolate = args.GetIsolate();
	udpObject *obj = ObjectWrap::Unwrap<udpObject>( getHolder(args) );
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
	//Isolate* isolate = args.GetIsolate();
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
#if ( NODE_MAJOR_VERSION >= 14 )
		SendUDPEx( obj->pc, ab->GetBackingStore()->Data(), ab->ByteLength(), dest );
#else
		SendUDPEx( obj->pc, ab->GetContents().Data(), ab->ByteLength(), dest );
#endif
	}
	else if( args[0]->IsUint8Array() ) {
		Local<Uint8Array> body = args[0].As<Uint8Array>();
		Local<ArrayBuffer> ab = body->Buffer();
#if ( NODE_MAJOR_VERSION >= 14 )
		SendUDPEx( obj->pc, ab->GetBackingStore()->Data(), ab->ByteLength(), dest );
#else
		SendUDPEx( obj->pc, ab->GetContents().Data(), ab->ByteLength(), dest );
#endif
	} else if( args[0]->IsString() ) {
		String::Utf8Value buf( isolate,  args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		SendUDPEx( obj->pc, *buf, buf.length(), dest );
	}
	else {
		lprintf( "Unhandled message format" );
	}
}


//---------------------------- TCP Sockets ---------------------------

static void tcpAsyncMsg_( Isolate *isolate, Local<Context> context, tcpObject * obj ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	networkEvent* eventMessage;

	{
		Local<Value> argv[2];
		while( eventMessage = (struct networkEvent*)DequeLink( &obj->eventQueue ) ) {
			Local<Function> cb;
			Local<Object> ab;
			//lprintf( "Handle posted message?%d %p", eventMessage->eventType, &obj->async );
			switch( eventMessage->eventType ) {
			case NET_EVENT_CONNECT:
			{
				cb = Local<Function>::New( isolate, obj->connectCallback );

				class constructorSet* c = getConstructors( isolate );
				Local<Function> cons = Local<Function>::New( isolate, c->tcpConstructor );
				Local<Object> tcpNew = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
				tcpObject* tcpObj = tcpObject::getSelf( tcpNew );
				tcpObj->server = obj;
				tcpObj->ssl = obj->ssl;
				tcpObj->allowSSLfallback = obj->allowSSLfallback;
				tcpObj->readStrings = obj->readStrings;
				tcpObj->pc = (PCLIENT)( eventMessage->buf );
				tcpObj->messageCallback.Reset( isolate, obj->messageCallback );
				tcpObj->connectCallback.Reset( isolate, obj->connectCallback );
				tcpObj->closeCallback.Reset( isolate, obj->closeCallback );

				SetCPPNetworkCloseCallback( tcpObj->pc, TCP_Close, (uintptr_t)tcpObj );
				SetCPPNetworkReadComplete( tcpObj->pc, TCP_ReadComplete, (uintptr_t)tcpObj );
				SetCPPNetworkWriteComplete( tcpObj->pc, TCP_Write, (uintptr_t)tcpObj );
				{
					struct optionStrings* strings = getStrings( isolate );
					SETV( tcpNew, strings->connectionString->Get( isolate ), makeSocket( isolate, tcpObj->pc, NULL, NULL, NULL, NULL ) );
				}

				argv[0] = tcpNew;
				if( !cb.IsEmpty() )
					cb->Call( context, eventMessage->_this.tcp->_this.Get( isolate ), 1, argv );
			}
				break;
			case NET_EVENT_CONNECTED:
				cb = Local<Function>::New( isolate, obj->connectCallback );
				{
					struct optionStrings* strings = getStrings( isolate );
					//lprintf( "Re-make connection status object.." );
					SETV( eventMessage->_this.tcp->_this.Get( isolate ), strings->connectionString->Get( isolate ), makeSocket( isolate, obj->pc, NULL, NULL, NULL, NULL ) );
				}
				if( !cb.IsEmpty() )
					cb->Call( context, eventMessage->_this.tcp->_this.Get( isolate ), 0, NULL );
				break;
			case NET_EVENT_FIRST_READ:
				cb = Local<Function>::New( isolate, obj->readyCallback );
				if( !cb.IsEmpty() )
					cb->Call( context, eventMessage->_this.tcp->_this.Get( isolate ), 0, NULL );
				break;
			case NET_EVENT_CONNECT_ERROR:
				cb = Local<Function>::New( isolate, obj->connectCallback );
				argv[0] = Integer::New( isolate, eventMessage->error );
				if( !cb.IsEmpty() )
					cb->Call( context, eventMessage->_this.tcp->_this.Get( isolate ), 1, argv );
				break;
			case NET_EVENT_READ:
				//argv[1] = ::getAddressBySA( isolate, eventMessage->from );
				if( !obj->readStrings ) {
#if ( NODE_MAJOR_VERSION >= 14 )
					std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( (POINTER)eventMessage->buf, eventMessage->buflen, releaseBufferBackingStore, NULL );
					ab = ArrayBuffer::New( isolate, bs );
#else
					ab = ArrayBuffer::New( isolate, (POINTER)eventMessage->buf, eventMessage->buflen );
					PARRAY_BUFFER_HOLDER holder = GetHolder();
					holder->o.Reset( isolate, ab );
					holder->o.SetWeak< ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
					holder->buffer = (void*)eventMessage->buf;
#endif
					argv[0] = ab;
					if( !obj->messageCallback.IsEmpty() )
						obj->messageCallback.Get( isolate )->Call( context, eventMessage->_this.tcp->_this.Get( isolate ), 1, argv );
				} else {
					MaybeLocal<String> buf = String::NewFromUtf8( isolate, (const char*)eventMessage->buf, NewStringType::kNormal, (int)eventMessage->buflen );
					//lprintf( "built string from %p", eventMessage->buf );
					argv[0] = buf.ToLocalChecked();
					if( !obj->messageCallback.IsEmpty() )
						obj->messageCallback.Get( isolate )->Call( context, eventMessage->_this.tcp->_this.Get( isolate ), 1, argv );
					Deallocate( CPOINTER, eventMessage->buf );
				}
				ReleaseAddress( eventMessage->from );
				break;
			case NET_EVENT_CLOSE:
				cb = Local<Function>::New( isolate, obj->closeCallback );
				if( !cb.IsEmpty() )
					cb->Call( context, eventMessage->_this.tcp->_this.Get( isolate ), 0, argv );
				//lprintf( "Close async handle: %p",(uv_handle_t*)&obj->async );
				if( !obj->ivm_hosted )
					uv_close( (uv_handle_t*)&obj->async, NULL );
				DeleteLinkQueue( &obj->eventQueue );
				break;
			}
			//lprintf( "Done with event %d, wake thread %p", eventMessage->eventType, eventMessage->waiter );
			eventMessage->done = TRUE;
			if( eventMessage->waiter )
				WakeThread( eventMessage->waiter );
			else
				DeleteFromSet( NET_EVENT, l.networkEvents, eventMessage );
		}
	}
}

static void tcpAsyncMsg( uv_async_t *handle ) {
	v8::Isolate *isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	tcpObject *obj         = (tcpObject *)handle->data;
	tcpAsyncMsg_( isolate, context, obj );
	if( !obj->c->ThreadObject_idleProc.IsEmpty() ) {
		Local<Function> cb      = Local<Function>::New( isolate, obj->c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}

struct tcpAsyncTask : SackTask {
	tcpObject *obj;
	tcpAsyncTask( tcpObject *obj )
	    : obj( obj ) {}
	void Run2( Isolate *isolate, Local<Context> context ) { tcpAsyncMsg_( isolate, context, obj ); }
};

void TCP_ReadComplete( uintptr_t psv, POINTER buffer, size_t length ) {
	tcpObject *obj = (tcpObject*)psv;
	if( buffer ) {
		struct networkEvent *pevt = GetFromSet( NET_EVENT, &l.networkEvents );
		(*pevt).eventType = NET_EVENT_READ;
		(*pevt).buf = NewArray( uint8_t*, length );
		memcpy( (POINTER)(*pevt).buf, buffer, length );
		(*pevt).buflen = length;
		(*pevt)._this.tcp = obj;
		(*pevt).waiter = NULL;
		EnqueLink( &obj->eventQueue, pevt );
		if( ( (tcpObject *)psv )->ivm_hosted )
			( (tcpObject *)psv )->c->ivm_post( ( (tcpObject *)psv )->c->ivm_holder
			                                 , std::make_unique<tcpAsyncTask>( (tcpObject *)psv ) );
		else
			uv_async_send( &obj->async );
	}
	else {
		obj->buffer = buffer = NewArray( uint8_t, 4096 );
		if( !obj->server ) {
			struct networkEvent *pevt = GetFromSet( NET_EVENT, &l.networkEvents );
			(*pevt).eventType = NET_EVENT_FIRST_READ;
			(*pevt)._this.tcp = obj;
			EnqueLink( &obj->eventQueue, pevt );
			if( ( (tcpObject *)psv )->ivm_hosted )
				( (tcpObject *)psv )->c->ivm_post( ( (tcpObject *)psv )->c->ivm_holder
				                                 , std::make_unique<tcpAsyncTask>( (tcpObject *)psv ) );
			else
				uv_async_send( &obj->async );
		}
	}
	ReadTCP( obj->pc, (POINTER)buffer, 4096 );
}

void TCP_Connect( uintptr_t psv, int error ) {
	tcpObject *obj = (tcpObject*)psv;
	struct networkEvent *pevt = GetFromSet( NET_EVENT, &l.networkEvents );
	//lprintf( "Got connect?  If we don't... should we get a close? %p", psv );
	if( !error ) {
		(*pevt).eventType = NET_EVENT_CONNECTED;
	}
	else {
		(*pevt).eventType = NET_EVENT_CONNECT_ERROR;
		(*pevt).error = error;
	}
	(*pevt)._this.tcp = obj;
	(*pevt).waiter = NULL;
	//lprintf( "Enque connect: %p", &obj->async );
	EnqueLink( &obj->eventQueue, pevt );
	if( ( (tcpObject *)psv )->ivm_hosted )
		( (tcpObject *)psv )->c->ivm_post( ( (tcpObject *)psv )->c->ivm_holder
		                                 , std::make_unique<tcpAsyncTask>( (tcpObject *)psv ) );
	else
		uv_async_send( &obj->async );
}

void TCP_Write( uintptr_t psv, CPOINTER buffer, size_t length ) {
	//tcpObject *obj = (tcpObject*)psv;
	// buffer is completed, so we can release it.
}

void TCP_Close( uintptr_t psv ) {
	tcpObject *obj = (tcpObject*)psv;
	if( obj->buffer ) {
		Release( obj->buffer );
	}
	struct networkEvent *pevt = GetFromSet( NET_EVENT, &l.networkEvents );
	(*pevt).eventType = NET_EVENT_CLOSE;
	(*pevt)._this.tcp = obj;
	(*pevt).waiter = NULL;
	//lprintf( "!!! Enque close: %p %p", (void*)psv, &obj->async );
	EnqueLink( &obj->eventQueue, pevt );
	if( ( (tcpObject *)psv )->ivm_hosted )
		( (tcpObject *)psv )->c->ivm_post( ( (tcpObject *)psv )->c->ivm_holder
		                                 , std::make_unique<tcpAsyncTask>( (tcpObject *)psv ) );
	else
		uv_async_send( &obj->async );
	//lprintf( "Close Happened to socket %p %p", psv, &obj->async );
	
	obj->pc = NULL;
}

void TCP_Notify( uintptr_t psv, PCLIENT pcNew ) {
//	tcpObject* tcpNew = new tcpObject( NULL );
//	tcpNew->pc = pcNew;
//	tcpNew->server = (tcpObject*)psv;
	struct networkEvent* pevt = GetFromSet( NET_EVENT, &l.networkEvents );
	( *pevt ).eventType = NET_EVENT_CONNECT;
	( *pevt )._this.tcp = (tcpObject*)psv;
	( *pevt ).buf = (CPOINTER)pcNew;
	( *pevt ).done = 0;
	( *pevt ).waiter = MakeThread();
	//lprintf( "Server connect event: %p %p %p", (void*)psv, pcNew, & (*pevt )._this.tcp->async );
	EnqueLink( &( (tcpObject*)psv )->eventQueue, pevt );
	if( ( (tcpObject *)psv )->ivm_hosted )
		( (tcpObject *)psv )->c->ivm_post( ( (tcpObject *)psv )->c->ivm_holder
		                                 , std::make_unique<tcpAsyncTask>( (tcpObject *)psv ) );
	else
		uv_async_send( &( (tcpObject*)psv )->async );
	while( !(*pevt).done ) {		
		WakeableSleep( 100 );
	}
	DeleteFromSet( NET_EVENT, l.networkEvents, pevt );
	//lprintf( "Notified of connect; waited for callback...");
}

static void sockLowError( uintptr_t psv, PCLIENT pc, enum SackNetworkErrorIdentifier error, ... ) {
	class tcpObject *obj = (class tcpObject*)psv;
	va_list args;
	va_start( args, error );
	//lprintf( "Low Error: %p %p %p %d", pc, obj->pc, obj, error );
	// auto handling callback...
	switch( error ) {
	default:
		lprintf( "Low error on %p %d", pc, error );
		break;
	case SACK_NETWORK_ERROR_HOST_NOT_FOUND:
		{
			// address that's not found...
			//const char* buf = va_arg( args, const char* );
			//size_t buflen = va_arg( args, size_t );
			//SOCKADDR *sa1 = (SOCKADDR*)GetNetworkLong( pc, GNL_REMOTE_ADDRESS );
			//SOCKADDR *sa2 = (SOCKADDR*)GetNetworkLong( pc, GNL_LOCAL_ADDRESS );
			//lprintf( "Request for host (not configured for this host): %p %p %p", pc, sa1, sa2 );
		}
		break;
	case SACK_NETWORK_ERROR_SSL_HANDSHAKE:
		if( obj->allowSSLfallback ) {
			//lprintf( "Fallback SSL" );
			const char* buf = va_arg( args, const char* );
			size_t buflen = va_arg( args, size_t );
			ssl_EndSecure( pc, (POINTER)buf, buflen );
		} else {
			//lprintf( "Close Socket... (won't get event, connect never actually issued)");
			// this gets blocked by being 'InUse', so should wait until we return to do this close again...
			RemoveClient( pc );
		}

		break;
	}
}


tcpObject::tcpObject( struct tcpOptions *opts ) {
	if( !opts ) {
		// this is an accepting socket; no options....
		eventQueue = CreateLinkQueue();
		async.data = this;

		class constructorSet* c = getConstructors( tcpObject::isolate );
		this->c                 = c;
		if( c->ivm_holder ) {
			this->ivm_hosted = true;
		} else 
			uv_async_init( c->loop, &async, tcpAsyncMsg );
		// pc will be set later...
		return;
	}
	NetworkWait( NULL, 256, 2 );
	SOCKADDR *addr = opts->address?CreateSockAddress( opts->address, opts->port ):NULL;
	SOCKADDR *toAddr = opts->toAddress?CreateSockAddress( opts->toAddress, opts->port ):NULL;

	this->readStrings = opts->readStrings;
	this->allowSSLfallback = opts->allowSSLfallback;
	eventQueue = CreateLinkQueue();
	//lprintf( "Init async handle. (wss)" );
	async.data = this;

	class constructorSet* c = getConstructors( opts->isolate );
	//lprintf( "Init async handle: %p",(uv_handle_t*)&async );
	if( c->ivm_holder ) {
		this->ivm_hosted = true;
	} else
		uv_async_init( c->loop, &async, tcpAsyncMsg );
	if( !opts->messageCallback.IsEmpty() )
		this->messageCallback.Reset( isolate, opts->messageCallback );
	if( !opts->connectCallback.IsEmpty() )
		this->connectCallback.Reset(isolate, opts->connectCallback );
	if( !opts->readyCallback.IsEmpty() )
		this->readyCallback.Reset(isolate, opts->readyCallback );
	if( !opts->closeCallback.IsEmpty() )
		this->closeCallback.Reset(isolate, opts->closeCallback );

	this->ssl = opts->ssl;

	if( toAddr )
		pc = CPPOpenTCPClientAddrExxx( toAddr, TCP_ReadComplete, (uintptr_t)this
		                             , TCP_Close, (uintptr_t)this
		                             , TCP_Write, (uintptr_t)this
		                             , TCP_Connect, (uintptr_t)this
		                             , ((opts->delayConnect || opts->ssl)? OPEN_TCP_FLAG_DELAY_CONNECT:0)
		                               | (( opts->ssl ) ? OPEN_TCP_FLAG_SSL_CLIENT : 0 )
		                               DBG_SRC );
	if( pc ) {
		if( opts->ssl ) {
			ssl_BeginClientSession( pc
				, opts->key, opts->key_len
				, opts->pass, opts->pass_len
				, opts->cert_chain, opts->cert_chain_len );
			NetworkConnectTCP( pc );
		}
	} else {
		pc = CPPOpenTCPListenerAddr_v2d( addr, TCP_Notify, (uintptr_t)this, TRUE DBG_SRC );

		// gets events about ssl failures

		if( pc && opts->ssl ) {
			if( opts->cert_chain ) {
				ssl_BeginServer_v2( pc
					, opts->cert_chain, opts->cert_chain_len
					, opts->key, opts->key_len
					, opts->pass, opts->pass_len, opts->host );
				{
					struct tcpHostOption* host;
					INDEX idx;
					LIST_FORALL( opts->hostList, idx, struct tcpHostOption*, host ) {
						ssl_setupHostCert( pc, host->host
						             , host->cert_chain, host->cert_chain_len
						             , host->key, host->key_len
						             , host->pass, host->pass_len );
					}
				}
			}
		}
		if( pc ) {
			SetCPPNetworkCloseCallback( pc, TCP_Close, (uintptr_t)this );
			SetNetworkErrorCallback( pc, sockLowError, (uintptr_t)this );
			SetNetworkListenerReady( pc );
		} else lprintf( "Failed to listen at:%s", opts->address );

	}

	if( pc ) {
		if( opts->reuseAddr )
			SetSocketReuseAddress( pc, TRUE );
		if( opts->reusePort )
			SetSocketReusePort( pc, TRUE );
	}

}

tcpObject::~tcpObject() {
	if( !closed )
		RemoveClient( pc );
}


static void ParseTcpHostOption( struct optionStrings* strings
	, struct tcpOptions* tcpOpts
	, Isolate* isolate, Local<Object> hostOpt ) {

	Local<Context> context = isolate->GetCurrentContext();
	Local<String> optName;
	struct tcpHostOption* newOpt = NewArray( struct tcpHostOption, 1 );
	MemSet( newOpt, 0, sizeof( *newOpt ) );
	if( hostOpt->Has( context, optName = strings->hostString->Get( isolate ) ).ToChecked() ) {
		Local<Value> opt = GETV( hostOpt, optName );
		if( opt->IsString() ) {
			String::Utf8Value address( USE_ISOLATE( isolate ) opt->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
			newOpt->host = StrDup( *address );
			newOpt->hostlen = address.length();
		}
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
		} else {
			newOpt->cert_chain = StrDup( *ca );
			newOpt->cert_chain_len = ca.length();
		}
	}

	if( !hostOpt->Has( context, optName = strings->keyString->Get( isolate ) ).ToChecked() ) {
		newOpt->key = NULL;
		newOpt->key_len = 0;
	} else {
		String::Utf8Value cert( USE_ISOLATE( isolate ) GETV( hostOpt, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		newOpt->key = StrDup( *cert );
		newOpt->key_len = cert.length();
	}

	if( !hostOpt->Has( context, optName = strings->passString->Get( isolate ) ).ToChecked() ) {
		newOpt->pass = NULL;
		newOpt->pass_len = 0;
	} else {
		String::Utf8Value cert( USE_ISOLATE( isolate ) GETV( hostOpt, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		newOpt->pass = StrDup( *cert );
		newOpt->pass_len = cert.length();
	}

	AddLink( &tcpOpts->hostList, newOpt );

}

void tcpObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int argc = args.Length();
	if(0)
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Must specify either type or options for server." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		struct tcpOptions tcpOpts = {};
		int argBase = 0;
		tcpOpts.isolate= isolate;
		struct optionStrings* strings = getStrings( isolate );

		
		// if it was uninitialized it would fail to reset
		//tcpOpts.messageCallback.Reset();
		if( argc > 0 ) {
			if( args[argBase]->IsString() ) {
				tcpOpts.address = StrDup( *String::Utf8Value( isolate, args[argBase]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ) );
				argBase++;
			}
			if( ( args.Length() >= argBase ) && args[argBase]->IsObject() ) {
				Local<Object> opts = args[argBase]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();

				Local<String> optName;
				// ---- get port
				if( !opts->Has( context, optName = strings->portString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.port = 0;
				} else {
					tcpOpts.port = (int)GETV( opts, optName )->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
				}
				// ---- get family
				if( opts->Has( context, optName = strings->familyString->Get( isolate ) ).ToChecked() ) {
					String::Utf8Value family( isolate, GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
					tcpOpts.v6 = ( StrCmp( *family, "IPv6" ) == 0 );
					if( tcpOpts.addressDefault ) {
						Deallocate( char*, tcpOpts.address );
						if( tcpOpts.v6 )
							tcpOpts.address = StrDup( "[::]" );
						else
							tcpOpts.address = StrDup( "0.0.0.0" );
					}
				}
				// ---- get address
				if( !opts->Has( context, optName = strings->addressString->Get( isolate ) ).ToChecked() ) {
					//tcpOpts.addressDefault = true;
				} else {
					tcpOpts.address = StrDup( *String::Utf8Value( isolate, GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ) );
				}
				// ---- get to port
				if( opts->Has( context, optName = strings->toPortString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.toPort = (int)GETV( opts, optName )->ToInteger( isolate->GetCurrentContext() ).ToLocalChecked()->Value();
				} else
					tcpOpts.toPort = 0;
				// ---- get toAddress
				if( opts->Has( context, optName = strings->toAddressString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.toAddress = StrDup( *String::Utf8Value( isolate, GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ) );
				} else
					tcpOpts.toAddress = NULL;

				if( !tcpOpts.toAddress && !tcpOpts.address ) {
					tcpOpts.addressDefault = true;
					if( tcpOpts.v6 )
						tcpOpts.address = StrDup( "[::0]" );
					else
						tcpOpts.address = StrDup( "0.0.0.0" );
				}

				// ---- get message callback
				if( opts->Has( context, optName = strings->messageString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.messageCallback.Reset( isolate, Local<Function>::Cast( GETV( opts, optName ) ) );
				}
				// ---- get connect callback
				if( opts->Has( context, optName = strings->connectString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.connectCallback.Reset( isolate, Local<Function>::Cast( GETV( opts, optName ) ) );
				}
				// ---- get ready callback
				if( opts->Has( context, optName = strings->readyString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.readyCallback.Reset( isolate, Local<Function>::Cast( GETV( opts, optName ) ) );
				}
				// ---- get close callback
				if( opts->Has( context, optName = strings->closeString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.closeCallback.Reset( isolate, Local<Function>::Cast( GETV( opts, optName ) ) );
				}
				// ---- get read strings setting
				if( opts->Has( context, optName = strings->readStringsString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.readStrings = GETV( opts, optName )->ToBoolean( isolate )->Value();
				}
				// ---- get reuse address
				if( opts->Has( context, optName = strings->reuseAddrString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.reuseAddr = GETV( opts, optName )->ToBoolean( isolate )->Value();
				} else tcpOpts.reuseAddr = false;
				// ---- get reuse port
				if( opts->Has( context, optName = strings->reusePortString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.reusePort = GETV( opts, optName )->ToBoolean( isolate )->Value();
				} else tcpOpts.reusePort = false;

				if( opts->Has( context, optName = strings->sslString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.ssl = GETV( opts, optName )->ToBoolean( isolate )->Value();
				} else tcpOpts.ssl = false;

				if( opts->Has( context, optName = strings->allowSSLfallbackString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.allowSSLfallback = GETV( opts, optName )->ToBoolean( isolate )->Value();
				} else tcpOpts.allowSSLfallback = TRUE;

				if( opts->Has( context, optName = strings->hostString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.host = StrDup( *String::Utf8Value( isolate, GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ) );
				}
				if( opts->Has( context, optName = strings->hostsString->Get( isolate ) ).ToChecked() ) {
					Local<Value> val = GETV( opts, optName );
					if( val->IsArray() ) {
						Local<Array> hosts = GETV( opts, optName ).As<Array>();
						uint32_t o;
						for( o = 0; o < hosts->Length(); o++ ) {
							Local<Object> host = GETV( hosts, o ).As<Object>();
							ParseTcpHostOption( strings, &tcpOpts, isolate, host );
						}
					}
				}

				if( opts->Has( context, optName = strings->certString->Get( isolate ) ).ToChecked() ) {
					String::Utf8Value cert( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
					tcpOpts.cert_chain = StrDup( *cert );
					tcpOpts.cert_chain_len = cert.length();
					tcpOpts.ssl = true;
				}

				if( opts->Has( context, optName = strings->caString->Get( isolate ) ).ToChecked() ) {
					String::Utf8Value ca( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
					if( tcpOpts.cert_chain ) {
						tcpOpts.cert_chain = (char*)Reallocate( tcpOpts.cert_chain, tcpOpts.cert_chain_len + ca.length() + 1 );
						strcpy( tcpOpts.cert_chain + tcpOpts.cert_chain_len, *ca );
						tcpOpts.cert_chain_len += ca.length();
					} else {
						tcpOpts.cert_chain = StrDup( *ca );
						tcpOpts.cert_chain_len = ca.length();
					}
					tcpOpts.ssl = true;
				}

				if( !opts->Has( context, optName = strings->keyString->Get( isolate ) ).ToChecked() ) {
					tcpOpts.key = NULL;
					tcpOpts.key_len = 0;
				} else {
					String::Utf8Value cert( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
					tcpOpts.key = StrDup( *cert );
					tcpOpts.key_len = cert.length();
				}

				argBase++;

			}
		}

		if( args.Length() >= argBase && args[argBase]->IsFunction() ) {
			Local<Function> arg0 = Local<Function>::Cast( args[argBase] );
			tcpOpts.messageCallback.Reset( isolate, arg0 );
		}

		Local<Object> _this = args.This();

		tcpObject::isolate = isolate;
		tcpObject* obj = new tcpObject( argc?&tcpOpts:NULL );
		if( argc > 0 ) {
			Deallocate( char*, tcpOpts.address );
			Deallocate( char*, tcpOpts.toAddress );
		}
		Deallocate( char*, tcpOpts.host );
		{
			struct tcpHostOption* opt;
			INDEX idx;

			LIST_FORALL( tcpOpts.hostList, idx, struct tcpHostOption*, opt ) {
				Deallocate( char*, opt->host );
				Deallocate( char*, opt->cert_chain );
				Deallocate( char*, opt->key );
				Deallocate( char*, opt->pass );
				Deallocate( struct tcpHostOption*, opt );
			}
			DeleteList( &tcpOpts.hostList );
		}
		obj->_this.Reset( isolate, _this );
		obj->Wrap( _this );
		//lprintf( "Makes a connection socket here with just tcp object?");
		SETV( _this, strings->connectionString->Get( isolate ), makeSocket( isolate, obj->pc, NULL, NULL, NULL, NULL ) );

		args.GetReturnValue().Set( _this );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		class constructorSet *c = getConstructors(isolate);
		Local<Function> cons = Local<Function>::New( isolate, c->tcpConstructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete[] argv;
	}
}

class tcpObject* tcpObject::getSelf( Local<Object> _this ) {
	return ObjectWrap::Unwrap<tcpObject>( _this );
}

void tcpObject::on( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	tcpObject *obj = ObjectWrap::Unwrap<tcpObject>( getHolder(args) );
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
		else if( StrCmp( *event, "connect" ) == 0 ) {
			if( cb->IsFunction() )
				obj->connectCallback.Reset( isolate, cb );
		}
		else if( StrCmp( *event, "ready" ) == 0 ) {
			if( cb->IsFunction() )
				obj->readyCallback.Reset( isolate, cb );
		} else if( StrCmp( *event, "close" ) == 0 ) {
			if( cb->IsFunction() )
				obj->closeCallback.Reset( isolate, cb );
		}
	}

}

void tcpObject::close( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	tcpObject *obj = ObjectWrap::Unwrap<tcpObject>( args.This() );
	RemoveClient( obj->pc );
}


void tcpObject::addHost( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	tcpObject *obj = ObjectWrap::Unwrap<tcpObject>( args.This() );
	if( args.Length() == 4 ) {
		String::Utf8Value hosts( isolate,  args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		String::Utf8Value cert( isolate,  args[1]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		String::Utf8Value key( isolate,  args[3]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		String::Utf8Value keypass( isolate,  args[3]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );

		ssl_setupHostCert( obj->pc, *hosts, *cert, cert.length(), *key, key.length(), *keypass, keypass.length() );
	}
}

void tcpObject::send( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	tcpObject *obj = ObjectWrap::Unwrap<tcpObject>( args.This() );
	if( !obj->pc ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Socket is not open." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	if( args[0]->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
		if( obj->ssl ) {
#if ( NODE_MAJOR_VERSION >= 14 )
			ssl_Send( obj->pc, ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
			ssl_Send( obj->pc, ab->GetContents().Data(), ab->ByteLength(), dest );
#endif
		} else {
#if ( NODE_MAJOR_VERSION >= 14 )
			SendTCP( obj->pc, ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
			SendTCP( obj->pc, ab->GetContents().Data(), ab->ByteLength(), dest );
#endif
		}
	}
	else if( args[0]->IsUint8Array() ) {
		Local<Uint8Array> body = args[0].As<Uint8Array>();
		Local<ArrayBuffer> ab = body->Buffer();
		if( obj->ssl ) {
#if ( NODE_MAJOR_VERSION >= 14 )
			ssl_Send( obj->pc, ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
			ssl_Send( obj->pc, ab->GetContents().Data(), ab->ByteLength(), dest );
#endif
		} else {
#if ( NODE_MAJOR_VERSION >= 14 )
			SendTCP( obj->pc, ab->GetBackingStore()->Data(), ab->ByteLength() );
#else
			SendTCP( obj->pc, ab->GetContents().Data(), ab->ByteLength() );
#endif
		}
	} else if( args[0]->IsString() ) {
		String::Utf8Value buf( isolate,  args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		if( obj->ssl ) {
			ssl_Send( obj->pc, *buf, buf.length() );
		} else {
			SendTCP( obj->pc, *buf, buf.length() );
		}
	}
	else {
		lprintf( "Unhandled message format" );
	}
}

void tcpObject::string_get( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	tcpObject* obj = ObjectWrap::Unwrap<tcpObject>( args.This() );
	if( !obj->pc ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Socket is not open." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	args.GetReturnValue().Set( Boolean::New( isolate, obj->readStrings ) );
}
void tcpObject::string_set( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	tcpObject* obj = ObjectWrap::Unwrap<tcpObject>( args.This() );
	if( args.Length() && args[0]->IsBoolean() ) {
		obj->readStrings = args[0]->BooleanValue( isolate );
	}
}

void udpObject::string_get( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	tcpObject* obj = ObjectWrap::Unwrap<tcpObject>( args.This() );
	args.GetReturnValue().Set( Boolean::New( isolate, obj->readStrings ) );
}
void udpObject::string_set( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	tcpObject* obj = ObjectWrap::Unwrap<tcpObject>( args.This() );
	if( args.Length() && args[0]->IsBoolean() ) {
		obj->readStrings = args[0]->BooleanValue( isolate );
	}
}

void tcpObject::ssl_fallback_get( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	tcpObject *obj = ObjectWrap::Unwrap<tcpObject>( args.This() );
	args.GetReturnValue().Set( Boolean::New( isolate, obj->allowSSLfallback ) );
}

void tcpObject::ssl_fallback_set( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() && args[0]->IsBoolean() ) {
		tcpObject *obj = ObjectWrap::Unwrap<tcpObject>( args.This() );
		obj->allowSSLfallback = args[0]->BooleanValue( isolate );
	}
}

void tcpObject::ssl_get( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	tcpObject *obj = ObjectWrap::Unwrap<tcpObject>( args.This() );
	args.GetReturnValue().Set( Boolean::New( isolate, obj->readStrings ) );
}

void tcpObject::ssl_set( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	tcpObject *obj = ObjectWrap::Unwrap<tcpObject>( args.This() );
	if( !obj->pc ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Socket is not open." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	if( args.Length() && args[0]->IsBoolean() ) {
		if( args[0]->BooleanValue( isolate ) ) {
			if( obj->server ) {
			} else {
				if( obj->pc ) {
					// keypair, keypass, rootcert
					ssl_BeginClientSession( obj->pc, NULL, 0, NULL, 0, NULL, 0 );
				} else {
					obj->ssl = TRUE;
				}
			}
		}
		else {
			// passes the buffer/length here to the read callback
			//  This is for the server to fall back to HTTP from HTTPS.
			ssl_EndSecure( obj->pc, NULL, 0 );
		}
	}
}

//---------------------------- Socket Address ---------------------------

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
		//struct optionStrings *strings = getStrings( isolate );
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

//-------------- ICMP ----------------------------

static void getName( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "A string to decode as an IP to lookup." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	String::Utf8Value addr( USE_ISOLATE( isolate ) args[0] );
	SOCKADDR* sockaddr = CreateSockAddress( *addr, 0 );
	char domain_name[256];
	//char service_name[32];
	int rc =  getnameinfo( (const SOCKADDR*)sockaddr,SOCKADDR_LENGTH(sockaddr), domain_name, 256, NULL, 0, 0 );
	
	if( !rc )
	{
		args.GetReturnValue().Set( String::NewFromUtf8( isolate, domain_name ).ToLocalChecked() );
	} else {
		args.GetReturnValue().Set( Null(isolate) );
	}
}

struct pingState {
	bool ivm_hosted;
	class constructorSet *c;
	uv_async_t async;
	Persistent<Function> cb;
	Isolate* isolate;
	volatile int done;
	volatile int handled;
	struct {
		SOCKADDR* addr;
		CTEXTSTR name;
		int min;
		int max;
		int avg;
		int drop;
		int hops;
	} result;
};

struct pingParams {
	struct pingState* state;
	TEXTSTR addr;
	int ttl;
	int count;
	int time;
	volatile int received;
};

static void asyncClosed( uv_handle_t* async ) {
	struct pingState* state = (struct pingState*)async->data;
	ReleaseEx( state DBG_SRC );
}

static void pingAsync_( Isolate*isolate, Local<Context>context, struct pingState*state ) {
	Local<Object> data = Object::New( state->isolate );
	if( !state->done ) {
		if( state->result.name )
			data->Set( context, String::NewFromUtf8Literal( state->isolate, "IP" ), String::NewFromUtf8( state->isolate, state->result.name ).ToLocalChecked() );
		else
			data->Set( context, String::NewFromUtf8Literal( state->isolate, "IP" ), Null( state->isolate ) );
		data->Set( context, String::NewFromUtf8Literal( state->isolate, "name" ), String::NewFromUtf8( state->isolate, GetAddrName( state->result.addr ) ).ToLocalChecked() );
		data->Set( context, String::NewFromUtf8Literal( state->isolate, "ip" ), String::NewFromUtf8( state->isolate, GetAddrString( state->result.addr ) ).ToLocalChecked() );
		data->Set( context, String::NewFromUtf8Literal( state->isolate, "min" ), Number::New( state->isolate, state->result.min ) );
		data->Set( context, String::NewFromUtf8Literal( state->isolate, "max" ), Number::New( state->isolate, state->result.max ) );
		data->Set( context, String::NewFromUtf8Literal( state->isolate, "avg" ), Number::New( state->isolate, state->result.avg ) );
		data->Set( context, String::NewFromUtf8Literal( state->isolate, "drop" ), Number::New( state->isolate, state->result.drop ) );
		data->Set( context, String::NewFromUtf8Literal( state->isolate, "hops" ), Number::New( state->isolate, state->result.hops ) );
		Local<Value> args[1] = { data };
		state->cb.Get( state->isolate )->Call( context, Null( state->isolate ), 1, args );
		state->handled = TRUE;
	} else {
		state->cb.Reset();
		uv_close( (uv_handle_t*)&state->async, asyncClosed );
	}
}

static void pingAsync( uv_async_t *async ) {
	struct pingState *state = (struct pingState *)async->data;
	HandleScope scope( state->isolate );
	Local<Context> context = state->isolate->GetCurrentContext();
	pingAsync_( state->isolate, context, state );
}

struct pingAsyncTask : SackTask {
	struct pingState *state;
	pingAsyncTask( struct pingState *state ) { this->state = state; }
	void Run2( Isolate *isolate, Local<Context> context ) { pingAsync_( isolate, context, state ); }
};

static void pingResult( uintptr_t psv, SOCKADDR* dwIP, CTEXTSTR name, int min, int max, int avg, int drop, int hops ) {
	struct pingState* state = (struct pingState*)psv;
	state->result.addr = dwIP;
	state->result.name = StrDup( name );
	state->result.min = min;
	state->result.max = max;
	state->result.avg = avg;
	state->result.drop = drop;
	state->result.hops = hops;
	state->handled = FALSE;
	if( state->ivm_hosted )
		state->c->ivm_post( state->c->ivm_holder, std::make_unique<pingAsyncTask>( state ) );
	uv_async_send( &state->async );
	while( !state->handled ) Relinquish();
}

static uintptr_t pingThread( PTHREAD thread ) {
	struct pingParams* params = (struct pingParams*)GetThreadParam( thread );
	struct pingState* state = params->state;
	params->received = 1;
	DoPingEx( params->addr, params->ttl, params->time, params->count, NULL, FALSE, pingResult, (uintptr_t)params->state );
	state->done = TRUE;
	uv_async_send( &state->async );
	return 0;
}


static void ping( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 2 ) {
		return;
	}
	struct pingParams params;
	struct pingState* state = NewArray( struct pingState, 1 );
	NetworkStart();
	params.received = 0;
	params.state = state;
	MemSet( state, 0, sizeof( *state ) );
	state->isolate = isolate;
	state->async.data = (void*)state;
	if( args.Length() > 1 ) {
		state->cb.Reset( isolate, args[0].As<Function>() );
		{
			params.ttl = 0;
			params.time = 1000;
			params.count = 1;
			String::Utf8Value addr( USE_ISOLATE( isolate ) args[1] );
			if( args.Length() > 2 ) {
				Local<Integer> i = args[2].As<Integer>();
				params.ttl = (int)i->Value();
			}
			if( args.Length() > 3 ) {
				Local<Integer> i = args[3].As<Integer>();
				params.count = (int)i->Value();
			}
			if( args.Length() > 4 ) {
				Local<Integer> i = args[4].As<Integer>();
				params.time = (int)i->Value();
			}
			class constructorSet* c = getConstructors( isolate );
			state->c = c;
			if( c->ivm_holder ) {
				state->ivm_hosted = true;
			} else
				uv_async_init( c->loop, &state->async, pingAsync );
			params.addr = *addr;
			ThreadTo( pingThread, (uintptr_t)&params );
			while( !params.received ) Relinquish();
		}
	}
	else ReleaseEx( state DBG_SRC );
}
