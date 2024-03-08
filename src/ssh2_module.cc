
#include "global.h"
//#include <sack.h>
//#include <openssl/bn.h>
#define LIBSSH2_OPENSSL
//#include <../src/libssh2_priv.h>
//#include <libssh2.h>



#define DEF_STRING(name) Eternal<String> *name##String
#define MK_STRING(name)  check->name##String = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, #name ) );
#define GET_STRING(name)  if( opts->Has( context, optName = strings->name##String->Get( isolate ) ).ToChecked() ) { \
				if( GETV( opts, optName )->IsString() ) { \
					name = new String::Utf8Value( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ); \
				} \
			}
#define GET_BOOL(name)  if( opts->Has( context, optName = strings->name##String->Get( isolate ) ).ToChecked() ) { \
				if( GETV( opts, optName )->IsBoolean() ) { \
					name = GETV( opts, optName )->TOBOOL( isolate ); \
				} \
			}

struct optionStrings {
	Isolate* isolate;
	//Eternal<String> *String;
	DEF_STRING( address );
	DEF_STRING( user );
	DEF_STRING( pass );
	DEF_STRING( privKey );
	DEF_STRING( pubKey );
	DEF_STRING( trace );
	DEF_STRING( data );
	DEF_STRING( close );
	DEF_STRING( connect );
};

static struct optionStrings *getStrings( Isolate *isolate ) {
	static PLIST strings;
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
		MK_STRING( address );
		MK_STRING( user );
		MK_STRING( pass );
		MK_STRING( privKey );
		MK_STRING( pubKey );
		MK_STRING( trace );
		MK_STRING( data );
		MK_STRING( close );
	}
	return check;
}



static void asyncmsg( uv_async_t* handle ) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope( isolate );
	SSH2_Object* ssh = (SSH2_Object*)handle->data;
	SSH2_Event* event;
	while( event = (SSH2_Event*)DequeLink( &ssh->eventQueue ) ) {
		switch( event->code ) {
			case SSH2_EVENT_CLOSE:
				uv_close( (uv_handle_t*)&ssh->async, NULL ); // have to hold onto the handle until it's freed.
				break;
			case SSH2_EVENT_AUTHDONE:
				if( event->success )
					ssh->connectPromise.Get(isolate)->Resolve( isolate->GetCurrentContext(), Undefined(isolate) );
				else
					ssh->connectPromise.Get( isolate )->Reject( isolate->GetCurrentContext(), Undefined( isolate ) );

				break;
			case SSH2_EVENT_CHANNEL:
				{
				if( event->data ) {
					constructorSet* c = getConstructors( isolate );
					Local<Value>* argv = new Local<Value>[0];
					Local<Function> cons = Local<Function>::New( isolate, c->SSH_Channel_constructor );
					MaybeLocal<Object> mo = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL );
					Local<Object> obj = mo.ToLocalChecked();
					SSH2_Channel* ch = SSH2_Channel::Unwrap<SSH2_Channel>(obj);

					ssh->channelPromise.Get( isolate )->Resolve( isolate->GetCurrentContext(), obj );
				} else {

					ssh->channelPromise.Get( isolate )->Reject( isolate->GetCurrentContext(), Undefined( isolate ) );

				}
				}
				break;
			case SSH2_EVENT_DATA:
				{
					Local<Value> argv[2];
					SSH2_Channel*channel = (SSH2_Channel*)event->data2;
					if( ssh->binary ) {
#if ( NODE_MAJOR_VERSION >= 14 )
						std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( event->data, event->length, releaseBufferBackingStore, NULL );
						Local<Object> arrayBuffer = ArrayBuffer::New( isolate, bs );
#else
						Local<Object> arrayBuffer = ArrayBuffer::New( isolate, event->data, event->length );
						PARRAY_BUFFER_HOLDER holder = GetHolder();
						holder->o.Reset( isolate, arrayBuffer );
						holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
						holder->buffer = buf;
#endif
						argv[0] = arrayBuffer;
					} else {
						argv[0] = String::NewFromUtf8( isolate, (const char*)event->data, NewStringType::kNormal, (int)event->length ).ToLocalChecked();
					}
					argv[1] = Number::New( isolate, event->iData );
					Local<Function> cb = Local<Function>::New( isolate, channel->dataCallback );
					cb->Call( isolate->GetCurrentContext(), channel->jsObject.Get( isolate ), 2, argv );
					event->done = true;
					WakeThread( event->waiter );
				}
				break;
		}
		dropEvent( event );
	}

}

SSH2_Channel::SSH2_Channel() {

}
SSH2_Channel::~SSH2_Channel() {
	//sack_ssh_channel_free( channel );
}

void SSH2_Channel::DataCallback( uintptr_t psv, int stream, const uint8_t* data, size_t length ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	makeEvent( event );
	event->code = SSH2_EVENT_DATA;
	event->iData = stream;
	event->data = (void*)data;
	event->length = length;
	event->data2 = (void*)channel;
	uv_async_send( &channel->ssh2->async );
	event->waiter = MakeThread();
	while( !event->done ) {
		WakeableSleep( 1000 );
	}
}

void SSH2_Channel::CloseCallback( uintptr_t psv ) {
	struct ssh_channel* channel = (struct ssh_channel*)psv;
	lprintf( "Channel %p closed", channel );
}

SSH2_Object::SSH2_Object( Isolate *isolate ) {
	constructorSet* c = getConstructors( isolate );

	uv_async_init( c->loop, &this->async, asyncmsg );
	this->async.data = this;


	this->session = sack_ssh_session_init( (uintptr_t)this );

	sack_ssh_set_handshake_complete( this->session, SSH2_Object::handshook );
	sack_ssh_set_auth_complete( this->session, SSH2_Object::authDone );
	sack_ssh_set_channel_open( this->session, SSH2_Object::channelOpen );
	//sack_ssh_set_listen( this->session, listenerSetup );


}
SSH2_Object::~SSH2_Object() {

}

void SSH2_Object::authDone( uintptr_t psv, LOGICAL success ) {
	SSH2_Object*ssh = (SSH2_Object*)psv;
	makeEvent( event );
	event->code = SSH2_EVENT_AUTHDONE;
	event->success = success;
	EnqueLink( &ssh->eventQueue, event );
	uv_async_send( &ssh->async );
}

void SSH2_Object::OpenChannel( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SSH2_Object* ssh = Unwrap<SSH2_Object>( args.This() );
	if( args.Length() < 1 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8Literal( isolate, "OpenChannel requires option object" ) ) );
		return;
	}
	Local<Context> context = isolate->GetCurrentContext();
	struct optionStrings *strings = getStrings( isolate );
	Local<Object> opts = args[0]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();


	Local<Promise::Resolver> pr = Promise::Resolver::New( isolate->GetCurrentContext() ).ToLocalChecked();
	ssh->channelPromise.Reset( isolate, pr );
	args.GetReturnValue().Set( pr->GetPromise() );

}

uintptr_t SSH2_Object::channelOpen( uintptr_t psv, struct ssh_channel* channel ) {
	SSH2_Object*ssh = (SSH2_Object*)psv;

	SSH2_Channel* ch = new SSH2_Channel();
	ch->channel = channel;
	ch->ssh2 = ssh;
	//ch->Wrap( args.This() );
	//ch->jsObject.Reset( isolate, args.This() );

	makeEvent( event );
	event->code = SSH2_EVENT_CHANNEL;
	event->data = (void*)ch;
	event->data2 = (void*)ssh;
	EnqueLink( &ssh->eventQueue, event );
	uv_async_send( &ssh->async );
	return (uintptr_t)ch;
	/*
	sack_ssh_set_pty_open( channel, channelPty );
	sack_ssh_set_channel_data( channel, DataCallback );
	sack_ssh_set_channel_close( channel, CloseCallback );
	sack_ssh_set_shell_open( channel, shellOpen );
	sack_ssh_channel_setenv( channel, "FOO", "bar" );
	sack_ssh_channel_request_pty( channel, "vanilla" );
	sack_ssh_channel_shell( channel );

	*/
}


void SSH2_Object::Init( Isolate *isolate, Local<Object> exports ){
	class constructorSet* c = getConstructors( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> sshTemplate;
	Local<FunctionTemplate> channelTemplate;

	ssl_InitLibrary();

	sshTemplate = FunctionTemplate::New( isolate, New );
	sshTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.SSH2_Object" ) );
	sshTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	Local<Function> sshConstructor = sshTemplate->GetFunction( context ).ToLocalChecked();
	c->SSH_Object_constructor.Reset( isolate, sshConstructor );

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( sshTemplate, "connect", SSH2_Object::Connect );

	channelTemplate = FunctionTemplate::New( isolate, New );
	channelTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.SSH2_Channel" ) );
	channelTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "read", SSH2_Channel::Read );
	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "close", SSH2_Channel::Close );
	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "pty", SSH2_Channel::OpenPTY );
	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "setenv", SSH2_Channel::SetEnv );
	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "shell", SSH2_Channel::Shell );
	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "exec", SSH2_Channel::Exec );

	c->SSH_Channel_constructor.Reset( isolate, channelTemplate->GetFunction( context ).ToLocalChecked() );

	Local<Object> i = Object::New( isolate );
	SET( exports, "SSH", sshConstructor );

}

void SSH2_Channel::OpenPTY( const v8::FunctionCallbackInfo<Value>& args ) {
}

void SSH2_Channel::Shell( const v8::FunctionCallbackInfo<Value>& args ) {
}

void SSH2_Channel::Exec( const v8::FunctionCallbackInfo<Value>& args ) {
}

void SSH2_Channel::SetEnv( const v8::FunctionCallbackInfo<Value>& args ) {
}

void SSH2_Channel::Read( const v8::FunctionCallbackInfo<Value>& args ) {
	SSH2_Channel* channel = Unwrap<SSH2_Channel>( args.This() );
	if( args.Length() < 1 ) {
		args.GetIsolate()->ThrowException( Exception::TypeError(
			String::NewFromUtf8Literal( args.GetIsolate(), "Read requires callback" ) ) );
		return;
	}
	sack_ssh_set_channel_data( channel->channel, SSH2_Channel::DataCallback );
	channel->dataCallback.Reset( args.GetIsolate(), Local<Function>::Cast( args[0] ) );
}

void SSH2_Channel::Close( const v8::FunctionCallbackInfo<Value>& args ) {
	SSH2_Channel* channel = Unwrap<SSH2_Channel>( args.This() );
	sack_ssh_set_channel_close( channel->channel, SSH2_Channel::CloseCallback );
	channel->closeCallback.Reset( args.GetIsolate(), Local<Function>::Cast( args[0] ) );
}

void SSH2_Channel::New( const v8::FunctionCallbackInfo<Value>& args ) {
	// this constructor is only called from internal code
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( args.IsConstructCall() ) {
		SSH2_Channel* obj;
		obj = new SSH2_Channel();
		obj->Wrap( args.This() );
		obj->jsObject.Reset( isolate, args.This() );
		args.GetReturnValue().Set( args.This() );
	} else {
		class constructorSet* c = getConstructors( isolate );
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Value>* argv = new Local<Value>[0];
		Local<Function> cons = Local<Function>::New( isolate, c->SSH_Channel_constructor );
		MaybeLocal<Object> mo = cons->NewInstance( isolate->GetCurrentContext(), 0, argv );
		if( !mo.IsEmpty() )
			args.GetReturnValue().Set( mo.ToLocalChecked() );
		delete[] argv;
	}
}

void SSH2_Object::New( const v8::FunctionCallbackInfo<Value>& args  ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( args.IsConstructCall() ) {
		SSH2_Object* obj;
		obj = new SSH2_Object( isolate );
		obj->Wrap( args.This() );
		obj->jsObject.Reset( isolate, args.This() );

		args.GetReturnValue().Set( args.This() );
	} else {
		class constructorSet* c = getConstructors( isolate );
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Value> *argv = new Local<Value>[0];
		Local<Function> cons = Local<Function>::New( isolate, c->SSH_Object_constructor );
		MaybeLocal<Object> mo = cons->NewInstance( isolate->GetCurrentContext(), 0, argv );
		if( !mo.IsEmpty() )
			args.GetReturnValue().Set( mo.ToLocalChecked() );
		delete[] argv;
	}
}

void SSH2_Object::handshook( uintptr_t psv, const uint8_t* string ) {
	SSH2_Object*ssh= (SSH2_Object*)psv;
	//lprintf( "Handshake:%s", string );
	//LogBinary( string, strlen( string ) );

	for( int i = 0; i < 20; i++ ) {
		fprintf( stderr, "%02X ", (unsigned char)string[i] );
	}
	makeEvent( event );
	event->code = SSH2_EVENT_HANDSHAKE;
	event->data = (void*)string;
	EnqueLink( &ssh->eventQueue, event );
	uv_async_send( &ssh->async );
}

void SSH2_Object::Connect( const v8::FunctionCallbackInfo<Value>& args  ) {

	SSH2_Object* ssh = Unwrap<SSH2_Object>( args.This() );
	if( !ssh->connectPromise.IsEmpty() ) {
		args.GetIsolate()->ThrowException( Exception::TypeError(
			String::NewFromUtf8Literal( args.GetIsolate(), "Connect already in progress, or already completed" ) ) );
		return;
	}

	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct optionStrings *strings = getStrings( isolate );


	if( args.Length() < 1 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8Literal( isolate, "Connect requires option object" ) ) );
		return;
	}
	Local<Object> opts = args[0]->ToObject( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked();
	String::Utf8Value *address = NULL;
	String::Utf8Value *user = NULL;
	String::Utf8Value *pass = NULL;
	String::Utf8Value *pubKey = NULL;
	String::Utf8Value *privKey = NULL;

	Local<Promise::Resolver> pr = Promise::Resolver::New( isolate->GetCurrentContext() ).ToLocalChecked();
	ssh->connectPromise.Reset( isolate, pr );

	LOGICAL trace = FALSE;
	Local<String> optName;
	Local<Function> connect;
	// requires opts
	GET_STRING( address );
	GET_STRING( user );
	GET_STRING( pass );
	GET_STRING( pubKey );
	GET_STRING( privKey );
	GET_BOOL( trace );

	if( opts->Has( context, optName = strings->connectString->Get( isolate ) ).ToChecked() ) {
		Local<Value> val;
		if( GETV( opts, optName )->IsFunction() ) {
			connect = Local<Function>::Cast( GETV( opts, optName ) );
		}
	}

	sack_ssh_session_connect( ssh->session, *address[0], 22);
	if( privKey ) {
		//sack_ssh_userauth_publickey_fromfile( ssh->session, *user, *pubKey, *privKey, *pass );
		sack_ssh_auth_user_cert( ssh->session, *user[0], *pubKey[0], *privKey[0], *pass[0]);
	}else
		sack_ssh_auth_user_password( ssh->session, *user[0], *pass[0]);

	args.GetReturnValue().Set( pr->GetPromise() );

}

// - - - - - - Port Forward Test - - - - - - - - - - -

static void relayReadCallback( PCLIENT pc, POINTER buffer, size_t length ) {
	if( !buffer ) {
		buffer = AllocateEx( 4096 DBG_SRC );
		SetNetworkLong( pc, 2, (intptr_t)buffer );
	} else {
		struct client_ssh* cs = (struct client_ssh*)GetNetworkLong( pc, 0 );
		sack_ssh_channel_write( (struct ssh_channel*)GetNetworkLong( pc, 1 ), 0, (const uint8_t*)buffer, length );
	}
	ReadTCP( pc, buffer, 4096 );
}

static void relayDataCallback( uintptr_t psv, int stream, const uint8_t* buffer, size_t length ) {
	PCLIENT pc = (PCLIENT)psv;
	SendTCP( pc, (POINTER)buffer, length );
}

static void relayCloseCallback( PCLIENT pc ) {
	struct client_ssh* cs = (struct client_ssh*)GetNetworkLong( pc, 0 );
	sack_ssh_channel_close( (struct ssh_channel*)GetNetworkLong( pc, 1 ) );
	ReleaseEx( (POINTER)GetNetworkLong( pc, 2 ) DBG_SRC );
}

static uintptr_t openRelaySocket( uintptr_t psv, struct ssh_channel* channel ) {
	struct client_ssh* cs = (struct client_ssh*)psv;
	PCLIENT pc = OpenTCPClientExxx( "localhost", 8080, relayReadCallback, relayCloseCallback, NULL, NULL, OPEN_TCP_FLAG_DELAY_CONNECT DBG_SRC );
	sack_ssh_set_channel_data( channel, relayDataCallback );
	SetNetworkLong( pc, 0, (intptr_t)cs );
	SetNetworkLong( pc, 1, (intptr_t)channel );
	NetworkConnectTCP( pc );
	return (uintptr_t)pc;
}

struct client_ssh {
	struct ssh_session* session;
};


static uintptr_t listenerSetup( uintptr_t psv, struct ssh_listener* listener, int bound_port ) {
	struct client_ssh* cs = (struct client_ssh*)psv;
	//sack_ssh_set_listen_connect( listener, openRelaySocket );
	if( !listener ) {
		CTEXTSTR error;
		int rc;
		sack_ssh_get_error( cs->session, &rc, &error );
		lprintf( "Listener error: %d %s", rc, error );
	}
	lprintf( "Listener setup:%p %d", listener, bound_port );
	return (uintptr_t)listener;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - -

static void handshook( uintptr_t psv, const uint8_t *string ) {
	struct client_ssh*cs = (struct client_ssh*)psv;
	//lprintf( "Handshake:%s", string );
	//LogBinary( string, strlen( string ) );
	const char* username = "d3x0r";
	const char* fn1 = "fn1";
	const char* fn2 = "fn2";
	const char* password = "";
	sack_ssh_auth_user_password( cs->session, username, password );

}

static void authDone( uintptr_t psv, LOGICAL success ) {
	struct client_ssh*cs = (struct client_ssh*)psv;
	//lprintf( "Auth Success:%d", success );
	if( success ) {
		sack_ssh_channel_forward_listen( cs->session, "127.0.0.1", 2223, openRelaySocket );
		/* Request a session channel on which to run a shell */
		sack_ssh_channel_open( cs->session );
	}
}


static void DataCallback( uintptr_t psv, int stream, const uint8_t* buffer, size_t length ) {
	struct ssh_channel* channel = (struct ssh_channel*)psv;
	struct client_ssh*cs = (struct client_ssh*)psv;
	lprintf( "Got data from channel %p %d %p %zd", channel, stream, buffer, length );
	LogBinary( (const uint8_t*)buffer, length );
}

static void CloseCallback( uintptr_t psv ) {
	struct ssh_channel* channel = (struct ssh_channel*)psv;
	lprintf( "Channel %p closed", channel );
}

static void channelPty( uintptr_t psv, LOGICAL success ) {
}

static void shellOpen( uintptr_t psv, LOGICAL success ) {
	struct ssh_channel* channel = (struct ssh_channel*)psv;
	lprintf( "SSH Opened:%d", success );
	//struct ssh_channel* channel = sack_ssh_channel_open( cs->session );
	//)
	sack_ssh_channel_write( channel, 0, (const uint8_t*)"ls\n", 3 );
}



static uintptr_t channelOpen( uintptr_t psv, struct ssh_channel* channel ) {
	struct client_ssh*cs = (struct client_ssh*)psv;
	sack_ssh_set_pty_open( channel, channelPty );
	sack_ssh_set_channel_data( channel, DataCallback );
	sack_ssh_set_channel_close( channel, CloseCallback );
	sack_ssh_set_shell_open( channel, shellOpen );
	/* Some environment variables may be set,
	 * It's up to the server which ones it'll allow though
	 */
	sack_ssh_channel_setenv( channel, "FOO", "bar" );
	/* Request a terminal with 'vanilla' terminal emulation
	 * See /etc/termcap for more options. This is useful when opening
	 * an interactive shell.
	 */
	sack_ssh_channel_request_pty( channel, "vanilla" );
   /* Open a SHELL on that pty */
	sack_ssh_channel_shell( channel );
	//if(0)
	//if( sack_ssh_channel_exec( channel, shell ) ) {
	//	fprintf( stderr, "Unable to request command on channel\n" );
//	}

	return (uintptr_t)channel;
}


void test( void ) {
	const char* shell = "/bin/bash";
	struct client_ssh* cs = NewArray( struct client_ssh, 1 );
	NetworkStart();

	struct ssh_session* session = cs->session = sack_ssh_session_init( (uintptr_t)cs );
	if( session == NULL ) {
		lprintf( "Failed to initialize an ssh session" );
	}
	sack_ssh_trace(cs->session, ~0);
	
	sack_ssh_set_handshake_complete( cs->session, handshook );
	sack_ssh_set_auth_complete( cs->session, authDone );
	sack_ssh_set_channel_open( cs->session, channelOpen );
	sack_ssh_session_connect( cs->session, "sp.d3x0r.org", 0 );
	sack_ssh_set_listen( cs->session, listenerSetup );

	
	/*
if(0)
	if( sack_ssh_userauth_publickey_fromfile( session, username,
		fn1, fn2,
		password ) ) {
		lprintf( "Authentication by public key failed." );
		//free( fn2 );
		//free( fn1 );
		return;// goto shutdown;
	}
	else {
		lprintf( "Authentication by public key succeeded." );
	}
	*/
}


static uintptr_t startTest( PTHREAD thread ) {
	test();
	return 0;
}
/*
PRELOAD( testinit ) {
	ssl_InitLibrary();
	ThreadTo( startTest, 0 );
}
*/

