
#include "global.h"
//#include <sack.h>
//#include <openssl/bn.h>
#define LIBSSH2_OPENSSL
//#include <../src/libssh2_priv.h>
#include <libssh2.h>


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
	Isolate *isolate;
	//Eternal<String> *String;
	DEF_STRING( address );
	DEF_STRING( user );
	DEF_STRING( pass );
	DEF_STRING( privKey );
	DEF_STRING( pubKey );
	DEF_STRING( trace );
	DEF_STRING( data );
	DEF_STRING( close );

}

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

static LIBSSH2_SEND_FUNC( SendCallback ) {
	// return ssize_t
	/* int socket \
		, const void* buffer
		, size_t length \
		, int flags
		, void** abstract)
	*/
	struct client_ssh* cs = (struct client_ssh*)abstract[0];
	SendTCP( cs->pc, buffer, length );
	return length;
}

static LIBSSH2_CHANNEL_EOF_FUNC( EofCallback ) {
	/* session, channel, void** abstract */
	struct client_ssh* cs = (struct client_ssh*)abstract[0];
	
}

static LIBSSH2_CHANNEL_DATA_FUNC( DataCallback ) {
	/* session, channel, stderr, POINTER buffer, size_t length, void** abstract */
	struct client_ssh* cs = (struct client_ssh*)abstract[0];
	lprintf( "Got data from channel %p %p %zd", channel, buffer, length );
	LogBinary( (const uint8_t*)buffer, length );
}


static void moveBuffers( PDATALIST pdl ) {
	struct data_buffer* buf;
	INDEX idx;
	struct data_buffer buf0;


	DATA_FORALL( pdl, idx, struct data_buffer*, buf ) {
		if( !idx ) buf0 = buf[0];
		else {
			buf[-1] = buf[0];
		}
	}
	if( buf )
	buf[idx] = buf0;

}

static LIBSSH2_RECV_FUNC( RecvCallback ) {
	// return ssize_t
	/* int socket \
		, const void* buffer
		, size_t length \
		, int flags
		, void** abstract)
	*/
	struct client_ssh* cs = (struct client_ssh*)abstract[0];
	while( !cs->buffers || !cs->buffers->Cnt ) {
		IdleFor( 500 );
	}
	struct data_buffer* buf;
	INDEX idx;
	cs->waiter = MakeThread();
	while( 1 )
	DATA_FORALL( cs->buffers, idx, struct data_buffer*, buf ) {
		if( buf->length ) {
			ssize_t filled;
			MemCpy( buffer, buf->buffer+buf->used, filled = ((( buf->length-buf->used)< length)?(buf->length-buf->used):length) );
			buf->used += filled;
			if( buf->used == buf->length ) {
				buf->length = 0;
				moveBuffers( cs->buffers );
			}
			return filled;
		}
	}
}

static void* alloc_callback( size_t size, void** abstract ) {
	return AllocateEx( size DBG_SRC );
}

static void free_callback( void *data, void** abstract ) {
	ReleaseEx( data DBG_SRC );
}
static void* realloc_callback( void* p, size_t size, void** abstract ) {
	return Reallocate( p, size DBG_SRC );
}

void readCallback( PCLIENT pc, POINTER buffer, size_t length ) {
	if( !buffer ) {
		struct client_ssh* cs = NewArray( struct client_ssh, 1 );

		SetNetworkLong( pc, 0, (intptr_t)cs );
		cs->buffers = CreateDataList( sizeof( struct data_buffer ) );
		cs->pc = pc;
		cs->session = NULL;
		cs->waiter = NULL;
		//buffer = AllocateEx( 4096 DBG_SRC );
	}
	else {
		// got data from socket, need to stuff it into SSH.
		//LIBSSH2_SESSION* session = (LIBSSH2_SESSION*)GetNetworkLong( pc, 0 );
		struct client_ssh* cs = (struct client_ssh*)GetNetworkLong( pc, 0 );
		INDEX idx;
		struct data_buffer* db;
		DATA_FORALL( cs->buffers, idx, struct data_buffer*, db ) {
			if( !db->length && db->buffer == buffer ) {
				db->length = length;
				WakeThread( cs->waiter );
				break;
			}
		}
	}
	struct client_ssh* cs = (struct client_ssh*)GetNetworkLong( pc, 0 );
	INDEX idx;
	struct data_buffer* db;
	DATA_FORALL( cs->buffers, idx, struct data_buffer*, db ) {
		if( !db->length )
			ReadTCP( pc, db->buffer, 4096 );
	}
	struct data_buffer new_db;
	new_db.buffer = (uint8_t*)AllocateEx( 4096 DBG_SRC );
	new_db.length = 0;
	new_db.used = 0;
	AddDataItem( &cs->buffers, &new_db );
	ReadTCP( pc, new_db.buffer, 4096 );
}




SSH2_Object::SSH2_Object() {
	this.client = NewArray( struct client_ssh, 1 );
	LIBSSH2_SESSION* session = this.client->session = libssh2_session_init_ex( alloc_callback, free_callback, realloc_callback, this );

	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_SEND, (libssh2_cb_generic*)SendCallback );
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_RECV, (libssh2_cb_generic*)RecvCallback );
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_CHANNEL_EOF, (libssh2_cb_generic*)EofCallback );
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_CHANNEL_DATA, (libssh2_cb_generic*)DataCallback );


}
SSH2_Object::~SSH2_Object() {

}

static void SSH2_Object::Init( Isolate *isolate, Local<Object> exports ){
	class constructorSet* c = getConstructors( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> sshTemplate;

	ssl_InitLibrary();

	sshTemplate = FunctionTemplate::New( isolate, New );
	sshTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.vfs.SSH2" ) );
	sshTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	c->SSH_Object_constructor.Reset( isolate, sshTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

	// Prototype
	//NODE_SET_PROTOTYPE_METHOD( sshTemplate, "genkey", genKey );
	Local<Object> i = Object::New( isolate );
	//Local<Function> i = sshTemplate->GetFunction(isolate->GetCurrentContext());
	/*
	SET( i, "seed", Function::New( context, seed ).ToLocalChecked() );
	SET( i, "genkey", Function::New( context, genKey ).ToLocalChecked() );
	SET( i, "gencert", Function::New( context, genCert ).ToLocalChecked() );
	SET( i, "genreq", Function::New( context, genReq ).ToLocalChecked() );
	SET( i, "pubkey", Function::New( context, pubKey ).ToLocalChecked() );
	SET( i, "signreq", Function::New( context, signReq ).ToLocalChecked() );
	SET( i, "validate", Function::New( context, validate ).ToLocalChecked() );
	SET( i, "expiration", Function::New( context, expiration ).ToLocalChecked() );
	SET( i, "certToString", Function::New( context, certToString ).ToLocalChecked() );
	*/
	//constructor.Reset( isolate, sshTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	SET( exports, "SSH2", i );

}

static void SSH2_Object::New( const v8::FunctionCallbackInfo<Value>& args  ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( args.IsConstructCall() ) {
		SSH_Object* obj;
		obj = new SSH_Object( );
		obj->Wrap( args.This() );

		args.GetReturnValue().Set( args.This() );
	} else {
		class constructorSet* c = getConstructors( isolate );
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Value> *argv = new Local<Value>[0];
		Local<Function> cons = Local<Function>::New( isolate, constructor );
		//MaybeLocal<Object> mo = cons->NewInstance( isolate->GetCurrentContext(), 0, argv );
		//if( !mo.IsEmpty() )
		//	args.GetReturnValue().Set( mo.ToLocalChecked() );
		//delete[] argv;
	}
}

static void SSH2_Object::Connect( const v8::FunctionCallbackInfo<Value>& args  ) {
	Isolate* isolate = args.GetIsolate();
	SSH2_Object* ssh = Unwrap<SSH2_Object>( args.This() );
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

	LOGICAL trace = FALSE;

	// requires opts
	GET_STRING( address );
	GET_STRING( user );
	GET_STRING( pass );
	GET_STRING( pubKey );
	GET_STRING( privKey );
	GET_BOOL( trace );

			if( opts->Has( context, optName = strings->dataString->Get( isolate ) ).ToChecked() ) {
				Local<Value> val;
				if( GETV( opts, optName )->IsFunction() ) {
					newTask->endCallback.Reset( isolate, Local<Function>::Cast( GETV( opts, optName ) ) );
					end = true;
				}
			}


	/* Create a session instance and start it up. This will trade welcome
	 * banners, exchange keys, and setup crypto, compression, and MAC layers
     */
	/* Enable all debugging when libssh2 was built with debugging enabled */
	if( trace )
		libssh2_trace( session, ~0 );


	PCLIENT pc = OpenTCPClientEx( address, 22, readCallback, closeCallback, NULL );
	cs = (struct client_ssh*)GetNetworkLong( pc, 0 );

	int rc = libssh2_session_handshake( session, 1/*sock*/ );
	if( rc ) {
		lprintf( "Failure establishing SSH session: %d", rc );
		return;// goto shutdown;
	}

	rc = 1;

	/* At this point we have not yet authenticated.  The first thing to do
	 * is check the hostkey's fingerprint against our known hosts Your app
	 * may have it hard coded, may go to a file, may present it to the
	 * user, that's your call
	 */
	CTEXTSTR fingerprint = libssh2_hostkey_hash( session, LIBSSH2_HOSTKEY_HASH_SHA1 );
	fprintf( stderr, "Fingerprint: " );
	for( int i = 0; i < 20; i++ ) {
		fprintf( stderr, "%02X ", (unsigned char)fingerprint[i] );
	}
	fprintf( stderr, "\n" );
	const char* username = "d3x0r";
	const char* fn1 = "fn1";
	const char* fn2 = "fn2";
	const char* password = "P3ap0dm4n";
	if( libssh2_userauth_password( session, username, password ) ) {
		lprintf( "Authentication by public key failed." );

	}
	else {
		lprintf( "Authentication by user pass succeeded." );
	}
if(0)
	if( libssh2_userauth_publickey_fromfile( session, username,
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



	/* Request a session channel on which to run a shell */
	LIBSSH2_CHANNEL* channel = libssh2_channel_open_session( session );
	if( !channel ) {
		fprintf( stderr, "Unable to open a session\n" );
		return;// goto shutdown;
	}

	/* Some environment variables may be set,
	 * It's up to the server which ones it'll allow though
	 */
	libssh2_channel_setenv( channel, "FOO", "bar" );


	/* Request a terminal with 'vanilla' terminal emulation
	 * See /etc/termcap for more options. This is useful when opening
	 * an interactive shell.
	 */
	if( libssh2_channel_request_pty( channel, "vanilla" ) ) {
		fprintf( stderr, "Failed requesting pty\n" );
		return;// goto skip_shell;
	}

	/* Open a SHELL on that pty */
	if( libssh2_channel_shell( channel ) ) {
		fprintf( stderr, "Unable to request shell on allocated pty\n" );
		return;// goto shutdown;
	}
	if(0)
	if( libssh2_channel_exec( channel, shell ) ) {
		fprintf( stderr, "Unable to request command on channel\n" );
		return;// goto shutdown;
	}

        /* At this point the shell can be interacted with using
         * libssh2_channel_read()
         * libssh2_channel_read_stderr()
         * libssh2_channel_write()
         * libssh2_channel_write_stderr()
		 *
		 * Blocking mode may be (en|dis)abled with:
		 *    libssh2_channel_set_blocking()
		 * If the server send EOF, libssh2_channel_eof() will return non-0
		 * To send EOF to the server use: libssh2_channel_send_eof()
		 * A channel can be closed with: libssh2_channel_close()
		 * A channel can be freed with: libssh2_channel_free()
		 */

		 /* Read and display all the data received on stdout (ignoring stderr)
		  * until the channel closes. This will eventually block if the command
		  * produces too much data on stderr; the loop must be rewritten to use
		  * non-blocking mode and include interspersed calls to
		  * libssh2_channel_read_stderr() to avoid this. See ssh2_echo.c for
		  * an idea of how such a loop might look.
		  */




}


void test( void ) {
	const char* shell = "/bin/bash";
	struct client_ssh* cs = NewArray( struct client_ssh, 1 );
	NetworkStart();
	PCLIENT pc = OpenTCPClient( "10.173.0.1", 22, readCallback );
	cs = (struct client_ssh*)GetNetworkLong( pc, 0 );
	LIBSSH2_SESSION* session = cs->session = libssh2_session_init_ex( alloc_callback, free_callback, realloc_callback, cs );
	if( session == NULL ) {
		lprintf( "Failed to initialize an ssh session" );
	}

	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_SEND, (libssh2_cb_generic*)SendCallback );
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_RECV, (libssh2_cb_generic*)RecvCallback );
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_CHANNEL_EOF, (libssh2_cb_generic*)EofCallback );
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_CHANNEL_DATA, (libssh2_cb_generic*)DataCallback );


	/* Create a session instance and start it up. This will trade welcome
	 * banners, exchange keys, and setup crypto, compression, and MAC layers
     */
	/* Enable all debugging when libssh2 was built with debugging enabled */
	libssh2_trace( session, ~0 );

	// assigns socket to session, but really we want to just use abstract
	int rc = libssh2_session_handshake( session, 1/*sock*/ );
	if( rc ) {
		lprintf( "Failure establishing SSH session: %d", rc );
		return;// goto shutdown;
	}

	rc = 1;

	/* At this point we have not yet authenticated.  The first thing to do
	 * is check the hostkey's fingerprint against our known hosts Your app
	 * may have it hard coded, may go to a file, may present it to the
	 * user, that's your call
	 */
	CTEXTSTR fingerprint = libssh2_hostkey_hash( session, LIBSSH2_HOSTKEY_HASH_SHA1 );
	fprintf( stderr, "Fingerprint: " );
	for( int i = 0; i < 20; i++ ) {
		fprintf( stderr, "%02X ", (unsigned char)fingerprint[i] );
	}
	fprintf( stderr, "\n" );
	const char* username = "d3x0r";
	const char* fn1 = "fn1";
	const char* fn2 = "fn2";
	const char* password = "";
	if( libssh2_userauth_password( session, username, password ) ) {
		lprintf( "Authentication by public key failed." );

	}
	else {
		lprintf( "Authentication by user pass succeeded." );
	}
if(0)
	if( libssh2_userauth_publickey_fromfile( session, username,
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



	/* Request a session channel on which to run a shell */
	LIBSSH2_CHANNEL* channel = libssh2_channel_open_session( session );
	if( !channel ) {
		fprintf( stderr, "Unable to open a session\n" );
		return;// goto shutdown;
	}

	/* Some environment variables may be set,
	 * It's up to the server which ones it'll allow though
	 */
	libssh2_channel_setenv( channel, "FOO", "bar" );


	/* Request a terminal with 'vanilla' terminal emulation
	 * See /etc/termcap for more options. This is useful when opening
	 * an interactive shell.
	 */
	if( libssh2_channel_request_pty( channel, "vanilla" ) ) {
		fprintf( stderr, "Failed requesting pty\n" );
		return;// goto skip_shell;
	}

	/* Open a SHELL on that pty */
	if( libssh2_channel_shell( channel ) ) {
		fprintf( stderr, "Unable to request shell on allocated pty\n" );
		return;// goto shutdown;
	}
	if(0)
	if( libssh2_channel_exec( channel, shell ) ) {
		fprintf( stderr, "Unable to request command on channel\n" );
		return;// goto shutdown;
	}

        /* At this point the shell can be interacted with using
         * libssh2_channel_read()
         * libssh2_channel_read_stderr()
         * libssh2_channel_write()
         * libssh2_channel_write_stderr()
		 *
		 * Blocking mode may be (en|dis)abled with:
		 *    libssh2_channel_set_blocking()
		 * If the server send EOF, libssh2_channel_eof() will return non-0
		 * To send EOF to the server use: libssh2_channel_send_eof()
		 * A channel can be closed with: libssh2_channel_close()
		 * A channel can be freed with: libssh2_channel_free()
		 */

		 /* Read and display all the data received on stdout (ignoring stderr)
		  * until the channel closes. This will eventually block if the command
		  * produces too much data on stderr; the loop must be rewritten to use
		  * non-blocking mode and include interspersed calls to
		  * libssh2_channel_read_stderr() to avoid this. See ssh2_echo.c for
		  * an idea of how such a loop might look.
		  */


	while( !libssh2_channel_eof( channel ) ) {
		char buf[1024];
		ssize_t err = libssh2_channel_read( channel, buf, sizeof( buf ) );
		if( err < 0 )
			fprintf( stderr, "Unable to read response: %ld\n", (long)err );
		else {
			fwrite( buf, 1, (size_t)err, stdout );
		}
	}

	libssh2_free( session, NULL );
}

static uintptr_t startTest( PTHREAD thread ) {
	test();
	return 0;
}

PRELOAD( testinit ) {
	//ssl_InitLibrary();
	//ThreadTo( startTest, 0 );
}

void connect( void ) {
	// PCLIENT = TCPClientLIstener();
	// 
}