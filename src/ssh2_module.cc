
#include "global.h"


#include "websocket_module.h"





struct optionStrings {
	Isolate* isolate;
	//Eternal<String> *String;
	DEF_STRING( address );
	DEF_STRING( user );
	DEF_STRING( password );
	DEF_STRING( privKey );
	DEF_STRING( pubKey );
	DEF_STRING( trace );
	DEF_STRING( data );
	DEF_STRING( windowSize );
	DEF_STRING( packetSize );
	DEF_STRING( type );
	DEF_STRING( message );
	DEF_STRING( close );
	DEF_STRING( connect );
	DEF_STRING( skipLogin );
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
		MK_STRING( type );
		MK_STRING( windowSize );
		MK_STRING( packetSize );
		MK_STRING( message );
		MK_STRING( password );
		MK_STRING( privKey );
		MK_STRING( pubKey );
		MK_STRING( trace );
		MK_STRING( data );
		MK_STRING( close );
		MK_STRING( skipLogin );
	}
	return check;
}



static void asyncmsg( uv_async_t* handle ) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope( isolate );
	SSH2_Object* ssh = (SSH2_Object*)handle->data;
	//SSH2_Object* channel = (SSH2_Object*)handle->data; 
	SSH2_Event* event;
	while( event = (SSH2_Event*)DequeLink( &ssh->eventQueue ) ) {
		switch( event->code ) {
			default:
				lprintf( "Unhandled event: %d", event->code );
				break;	
			case SSH2_EVENT_CLOSE:
				uv_close( (uv_handle_t*)&ssh->async, NULL ); // have to hold onto the handle until it's freed.
				break;
			case SSH2_EVENT_HANDSHAKE:
				{
					Local<Promise::Resolver> pr;
					Local<Promise::Resolver> pr_login;
					if( !ssh->connectPromise.IsEmpty() ) {
						pr = ssh->connectPromise.Get( isolate );
						ssh->connectPromise.Reset();
						ssh->activePromise = NULL;
					} 
					if( !ssh->loginPromise.IsEmpty() ) {
						pr_login = ssh->loginPromise.Get( isolate );
						ssh->activePromise = &ssh->loginPromise;
						//ssh->loginPromise.Reset();
					}
					//lprintf( "handshake event %p %p", pr, pr_login );

					if( event->success ) {
						if( pr_login.IsEmpty() ) {
							Local<ArrayBuffer> ab;
#if ( NODE_MAJOR_VERSION >= 14 )
							std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( (POINTER)ssh->fingerprintData, 20, releaseBufferBackingStore, NULL );
							ab = ArrayBuffer::New( isolate, bs );
#else
#error "Need to implement ArrayBuffer creation for node 14 and earlier (node 14 is End of life, consider upgrading)"
#endif
							pr->Resolve( isolate->GetCurrentContext(), ab );
						} else 
							pr->Resolve( isolate->GetCurrentContext(), pr_login );
					} else {
						pr->Reject( isolate->GetCurrentContext(), Undefined( isolate ) );
					}
				}
				break;
			case SSH2_EVENT_ERROR:
				if( ssh->activePromise ) {
					Local<Object> error = Object::New( isolate );
					error->Set( isolate->GetCurrentContext()
								, String::NewFromUtf8Literal( isolate, "message" )
						, String::NewFromUtf8( isolate, (const char*)event->data, NewStringType::kNormal, (int)event->length ).ToLocalChecked() );
					error->Set( isolate->GetCurrentContext()
						, String::NewFromUtf8Literal( isolate, "error" ), Number::New( isolate, event->iData ) );
					if( ssh->activePromise == &ssh->connectPromise ) 
						error->Set( isolate->GetCurrentContext()
							, String::NewFromUtf8Literal( isolate, "options" ), ssh->connectOptions.Get( isolate ) );
					if( ssh->activePromise ) {
						Maybe<bool> rs = ssh->activePromise[0].Get( isolate )->Reject( isolate->GetCurrentContext(), error );
						if( rs.IsNothing() ) {
						}
						ssh->activePromise[0].Reset();
						ssh->activePromise = NULL;
					}
					//ssh->activeP
				} else {
					lprintf( "Error Event: %d %s", event->iData, event->data );
				}
				if( !event->waiter ) ReleaseEx( event->data DBG_SRC );
				/*
				{
					Local<Value> argv[1];
					argv[0] = String::NewFromUtf8( isolate, (const char*)event->data, NewStringType::kNormal, (int)event->length ).ToLocalChecked();
					Local<Function> cb = Local<Function>::New( isolate, ssh->errorCallback );
					cb->Call( isolate->GetCurrentContext(), ssh->jsObject.Get( isolate ), 1, argv );
					event->done = true;
					WakeThread( event->waiter );
				}
				*/
				break;
			case SSH2_EVENT_CHANNEL_ERROR:
				{
					SSH2_Channel* channel = (SSH2_Channel*)event->data2;
					if( !IsQueueEmpty( &channel->activePromises ) ) {
						Local<Object> error = Object::New( isolate );
						error->Set( isolate->GetCurrentContext()
							, String::NewFromUtf8Literal( isolate, "message" )
							, String::NewFromUtf8( isolate, (const char*)event->data, NewStringType::kNormal, (int)event->length ).ToLocalChecked() );
						error->Set( isolate->GetCurrentContext()
							, String::NewFromUtf8Literal( isolate, "error" ), Number::New( isolate, event->iData ) );
						Persistent<Promise::Resolver>* promise = ( Persistent<Promise::Resolver>* )DequeLink( &channel->activePromises );
						//if( promise == &channel->connectPromise )
						//	error->Set( isolate->GetCurrentContext()
						//		, String::NewFromUtf8Literal( isolate, "options" ), ssh->connectOptions.Get( isolate ) );

						Maybe<bool> rs = promise[0].Get(isolate)->Reject(isolate->GetCurrentContext(), error);
						if( rs.IsNothing() ) {
						}
						//ssh->activePromise[0].Reset();
						//ssh->activePromise = NULL;
						//ssh->activeP
					} else {
						lprintf( "Channel Error Event: %d %s", event->iData, event->data );
					}
					// event->data is the message which is a dynamic string
					if( !event->waiter ) ReleaseEx( event->data DBG_SRC );
				}
				break;
			case SSH2_EVENT_LISTEN_ERROR:
				{
					SSH2_RemoteListen* listener = (SSH2_RemoteListen*)event->data2;
					//if( !IsQueueEmpty( &listener->activePromises ) ) 
					{
						lprintf( "Unhandled listener error event: %d %s", event->iData, event->data );
						Local<Object> error = Object::New( isolate );
						error->Set( isolate->GetCurrentContext()
							, String::NewFromUtf8Literal( isolate, "message" )
							, String::NewFromUtf8( isolate, (const char*)event->data, NewStringType::kNormal, (int)event->length ).ToLocalChecked() );
						error->Set( isolate->GetCurrentContext()
							, String::NewFromUtf8Literal( isolate, "error" ), Number::New( isolate, event->iData ) );
						//Persistent<Promise::Resolver>* promise = ( Persistent<Promise::Resolver>* )DequeLink( &listener->activePromises );
						//if( promise == &channel->connectPromise )
						//	error->Set( isolate->GetCurrentContext()
						//		, String::NewFromUtf8Literal( isolate, "options" ), ssh->connectOptions.Get( isolate ) );

						//Maybe<bool> rs = promise[0].Get(isolate)->Reject(isolate->GetCurrentContext(), error);
						//if( rs.IsNothing() ) {
						//}
						//ssh->activePromise[0].Reset();
						//ssh->activePromise = NULL;
						//ssh->activeP
					//} else {
						lprintf( "Channel Error Event: %d %s", event->iData, event->data );
					}
					// event->data is the message which is a dynamic string
					if( !event->waiter ) ReleaseEx( event->data DBG_SRC );
				}
				break;
			case SSH2_EVENT_AUTHDONE:
				{
					Local<Promise::Resolver> pr_connect;
					Local<Promise::Resolver> pr;
					if( !ssh->connectPromise.IsEmpty() ) {
						pr_connect = ssh->connectPromise.Get( isolate );
						ssh->connectPromise.Reset();
					} else if( !ssh->loginPromise.IsEmpty() ) {
						pr = ssh->loginPromise.Get( isolate );
						ssh->loginPromise.Reset();
						ssh->activePromise = NULL;
					}
					ssh->connectOptions.Reset();
					if( event->success ) {
						if( pr_connect.IsEmpty() ) {
							Local<ArrayBuffer> ab;
#if ( NODE_MAJOR_VERSION >= 14 )
							std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( (POINTER)ssh->fingerprintData, 20, releaseBufferBackingStore, NULL );
							ab = ArrayBuffer::New( isolate, bs );
#else
#error "Need to implement ArrayBuffer creation for node 14 and earlier (node 14 is End of life, consider upgrading)"
#endif
							pr->Resolve( isolate->GetCurrentContext(), ab );
						} else
							pr->Resolve( isolate->GetCurrentContext(), Undefined( isolate ) );
					} else
						pr->Reject( isolate->GetCurrentContext(), Undefined( isolate ) );
					ssh->connectOptions.Reset();
					ssh->loginPromise.Reset();
				}
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
						ch->ssh2 = ssh;
						ch->channel = (ssh_channel*)event->data;
						event->data = ch;
						sack_ssh_set_channel_error( ch->channel, SSH2_Channel::Error );

						ssh->channelPromise.Get( isolate )->Resolve( isolate->GetCurrentContext(), obj );
					} else {
						event->data = NULL;
						ssh->channelPromise.Get( isolate )->Reject( isolate->GetCurrentContext(), Undefined( isolate ) );
					}
				}
				break;
			case SSH2_EVENT_REVERSE_CONNECT:
				{
					constructorSet* c = getConstructors( isolate );
					Local<Value>* argv = new Local<Value>[0];
					Local<Function> cons = Local<Function>::New( isolate, c->SSH_Channel_constructor );
					MaybeLocal<Object> mo = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL );
					Local<Object> obj = mo.ToLocalChecked();
					SSH2_Channel* ch = SSH2_Channel::Unwrap<SSH2_Channel>( obj );
					ch->ssh2 = ssh;
					ch->channel = (ssh_channel*)event->data;

					ch->wsPipe = WebSocketPipeConnect( (struct html5_web_socket*)event->data2, (uintptr_t)ch );

					event->data = ch;
				}
				break;
			case SSH2_EVENT_REVERSE_CHANNEL:
				{
					if( event->data ) {
						constructorSet* c = getConstructors( isolate );
						Local<Value>* argv = new Local<Value>[0];
						Local<Function> cons = Local<Function>::New( isolate, c->SSH_RemoteListen_constructor );
						MaybeLocal<Object> mo = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL );
						Local<Object> obj = mo.ToLocalChecked();
						SSH2_RemoteListen* ch = SSH2_RemoteListen::Unwrap<SSH2_RemoteListen>(obj);
						ch->ssh2 = ssh;
						ch->listener = (ssh_listener*)event->data;
						event->data = ch;
						//sack_ssh_set_listener_error( ch->listener, SSH2_RemoteListen::Error );

						ssh->connectPromise.Get( isolate )->Resolve( isolate->GetCurrentContext(), obj );
					} else {
						event->data = NULL;
						ssh->connectPromise.Get( isolate )->Reject( isolate->GetCurrentContext(), Undefined( isolate ) );
					}
				}
				break;
			case SSH2_EVENT_DATA:
				{
					Local<Value> argv[2];
					SSH2_Channel*channel = (SSH2_Channel*)event->data2;
					if( channel->binary ) {
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
				}
				break;
			case SSH2_EVENT_SETENV:
			case SSH2_EVENT_PTY:
			case SSH2_EVENT_SHELL:
			case SSH2_EVENT_EXEC:
				{
					SSH2_Channel* channel = (SSH2_Channel*)event->data;
					Persistent<Promise::Resolver>* promise = ( Persistent<Promise::Resolver>* )DequeLink( &channel->activePromises );
					Local<Promise::Resolver> pr = promise[0].Get( isolate );
					if( pr->GetPromise()->State() == Promise::PromiseState::kPending ) {
						if( event->success ) {
							pr->Resolve( isolate->GetCurrentContext(), True( isolate ) );
						} else {
							pr->Reject( isolate->GetCurrentContext(), Undefined( isolate ) );
						}
					} // otherwise it would have already been fulfilled by error();
					promise[0].Reset();
				}
				break;
		}
		if( event->waiter ) {
			event->done = true;
			// waiters need to drop the event - or it might not get 'done'
			WakeThread( event->waiter );
		} else
			dropEvent( event );
	}

	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}


}

SSH2_Channel::SSH2_Channel() {
	this->activePromises = CreateLinkQueue( );
}
SSH2_Channel::~SSH2_Channel() {
	//sack_ssh_channel_free( channel );
}

ATEXIT(asdf   ) {
	DebugBreak();
}

void SSH2_Channel::DataCallback( uintptr_t psv, int stream, const uint8_t* data, size_t length ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	makeEvent( event );
	event->code = SSH2_EVENT_DATA;
	event->iData = stream;
	event->data = (void*)data;
	event->length = length;
	event->data2 = (void*)channel;
	event->done = 0;
	event->waiter = MakeThread();
	EnqueLink( &channel->ssh2->eventQueue, event );
	uv_async_send( &channel->ssh2->async );
	while( !event->done ) {
		WakeableSleep( 1000 );
	}
	dropEvent( event );
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
	sack_ssh_set_session_error( this->session, SSH2_Object::Error );
	//sack_ssh_set_listen( this->session, listenerSetup );


}
SSH2_Object::~SSH2_Object() {

}

void SSH2_Object::Error( uintptr_t psv, int errcode, const char* string, int errlen ) {
	SSH2_Object*ssh = (SSH2_Object*)psv;
	INDEX idx;
	PTHREAD test;
	PTHREAD self = MakeThread();
	LIST_FORALL( global.rootThreads, idx, PTHREAD, test ) if( test == self ) break;
	if( test ) string = (const char*)StrDup( string );
	makeEvent( event );
	event->code = SSH2_EVENT_ERROR;
	event->iData = errcode;
	event->data = (void*)string;
	event->length = errlen;
	event->done = 0;
	event->waiter = test?NULL:self;
	EnqueLink( &ssh->eventQueue, event );
	uv_async_send( &ssh->async );
	if( event->waiter ) {
		while( !event->done ) {
			WakeableSleep( 1000 );
		}
		dropEvent( event );
	}
}

void SSH2_Channel::Error( uintptr_t psv, int errcode, const char* string, int errlen ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	INDEX idx;
	PTHREAD test;
	PTHREAD self = MakeThread();
	LIST_FORALL( global.rootThreads, idx, PTHREAD, test ) if( test == self ) break;
	if( test ) string = (const char*)StrDup( string );
	makeEvent( event );
	event->code = SSH2_EVENT_CHANNEL_ERROR;
	event->iData = errcode;
	event->data = (void*)string;
	event->data2 = channel;
	event->length = errlen;
	event->done = 0;
	event->waiter = test ? NULL : self;
	EnqueLink( &channel->ssh2->eventQueue, event );
	uv_async_send( &channel->ssh2->async );
	if( event->waiter ) {
		while( !event->done ) {
			WakeableSleep( 1000 );
		}
		dropEvent( event );
	}
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
	
	Local<Context> context = isolate->GetCurrentContext();
	struct optionStrings *strings = getStrings( isolate );
	Local<Object> opts = args.Length() > 0?args[0]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() : Object::New( isolate );
	Local<String> optName;

	GET_STRING( type );
	GET_STRING( message );
	GET_NUMBER( windowSize );
	if( !windowSize ) windowSize = 0x4000;
	GET_NUMBER( packetSize );
	if( !packetSize ) packetSize = 0x1000;

	sack_ssh_channel_open_v2( ssh->session
		, type?*type[0]:"session", type ? type->length() : 7
		, windowSize, packetSize
		, message ? *message[0] : NULL, message ? message->length() : 0
		, SSH2_Object::channelOpen );


	Local<Promise::Resolver> pr = Promise::Resolver::New( isolate->GetCurrentContext() ).ToLocalChecked();
	ssh->channelPromise.Reset( isolate, pr );
	args.GetReturnValue().Set( pr->GetPromise() );

}

uintptr_t SSH2_Object::channelOpen( uintptr_t psv, struct ssh_channel* channel ) {
	SSH2_Object*ssh = (SSH2_Object*)psv;

	makeEvent( event );
	//sack_ssh_set_channel_data( channel, SSH2_Channel::DataCallback );
	event->code = SSH2_EVENT_CHANNEL;
	event->data = (void*)channel;
	event->waiter = MakeThread();
	event->done = 0;
	EnqueLink( &ssh->eventQueue, event );
	uv_async_send( &ssh->async );
	while( !event->done ) {
		WakeableSleep( 1000 );
	}
	uintptr_t result = (uintptr_t)event->data;
	dropEvent( event );
	return (uintptr_t)result;
}


void SSH2_Object::Init( Isolate *isolate, Local<Object> exports ){
	class constructorSet* c = getConstructors( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> sshTemplate;
	Local<FunctionTemplate> channelTemplate;
	Local<FunctionTemplate> remoteListenTemplate;

	AddLink( &global.rootThreads, MakeThread() );
	ssl_InitLibrary();

	sshTemplate = FunctionTemplate::New( isolate, SSH2_Object::New );
	sshTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.SSH2_Object" ) );
	sshTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap


	// Prototype
	NODE_SET_PROTOTYPE_METHOD( sshTemplate, "connect", SSH2_Object::Connect );
	NODE_SET_PROTOTYPE_METHOD( sshTemplate, "login", SSH2_Object::Login );
	NODE_SET_PROTOTYPE_METHOD( sshTemplate, "Channel", SSH2_Object::OpenChannel );
	// forward is a connection from local to remote
	NODE_SET_PROTOTYPE_METHOD( sshTemplate, "forward", SSH2_Object::Forward );
	// as confusing as this is , reverse is 'forward' from libssh2, forward is 'direct' which is also forward
	// reverse is a connection from remote to local

	NODE_SET_PROTOTYPE_METHOD( sshTemplate, "reverse", SSH2_Object::Reverse ); 
	sshTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "fingerprint" )
		, FunctionTemplate::New( isolate, SSH2_Object::fingerprint )
		, Local<FunctionTemplate>() );

	Local<Function> sshConstructor = sshTemplate->GetFunction( context ).ToLocalChecked();
	c->SSH_Object_constructor.Reset( isolate, sshConstructor );

	channelTemplate = FunctionTemplate::New( isolate, SSH2_Channel::New );
	channelTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.SSH2_Channel" ) );
	channelTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "read", SSH2_Channel::Read );
	channelTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "binary" )
		, FunctionTemplate::New( isolate, SSH2_Channel::getBinaryRead )
		, FunctionTemplate::New( isolate, SSH2_Channel::setBinaryRead )
	);
	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "send", SSH2_Channel::Send );
	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "close", SSH2_Channel::Close );
	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "pty", SSH2_Channel::OpenPTY );
	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "setenv", SSH2_Channel::SetEnv );
	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "shell", SSH2_Channel::Shell );
	NODE_SET_PROTOTYPE_METHOD( channelTemplate, "exec", SSH2_Channel::Exec );

	c->SSH_Channel_constructor.Reset( isolate, channelTemplate->GetFunction( context ).ToLocalChecked() );

	remoteListenTemplate = FunctionTemplate::New( isolate, SSH2_RemoteListen::New );
	remoteListenTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.SSH2_RemoteListener" ) );
	remoteListenTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap


	// Prototype
	NODE_SET_PROTOTYPE_METHOD( remoteListenTemplate, "close", SSH2_RemoteListen::Close );
	//NODE_SET_PROTOTYPE_METHOD( remoteListenTemplate, "onaccept", SSH2_RemoteListen::SetOnAccept );

	c->SSH_RemoteListen_constructor.Reset( isolate, remoteListenTemplate->GetFunction( context ).ToLocalChecked() );

	//Local<Object> i = Object::New( isolate );
	SET( exports, "SSH", sshConstructor );

}


void SSH2_Channel::PtyCallback( uintptr_t psv, LOGICAL success ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	makeEvent( event );
	event->code = SSH2_EVENT_PTY;
	event->data = channel;
	event->success = success;
	EnqueLink( &channel->ssh2->eventQueue, event );

	uv_async_send( &channel->ssh2->async );\
}

void SSH2_Channel::OpenPTY( const v8::FunctionCallbackInfo<Value>& args ) {
	SSH2_Channel* channel = Unwrap<SSH2_Channel>( args.This() );
	Isolate* isolate = args.GetIsolate();
	String::Utf8Value* term;
	if( args.Length() > 0 ) {
		term = new String::Utf8Value( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	} else term = NULL;
	Local<Promise::Resolver> pr = Promise::Resolver::New( isolate->GetCurrentContext() ).ToLocalChecked();
	channel->ptyPromise.Reset( isolate, pr );
	EnqueLink( &channel->activePromises, &channel->ptyPromise );
	sack_ssh_channel_request_pty( channel->channel, (term?*term[0] : "vanilla"), SSH2_Channel::PtyCallback );
	args.GetReturnValue().Set( pr->GetPromise() );

}


void SSH2_Channel::getBinaryRead( const v8::FunctionCallbackInfo<Value>& args ) {
	SSH2_Channel* channel = Unwrap<SSH2_Channel>( args.This() );
	args.GetReturnValue().Set( Boolean::New( args.GetIsolate(), channel->binary ) );
}

void SSH2_Channel::setBinaryRead( const v8::FunctionCallbackInfo<Value>& args ) {
	SSH2_Channel* channel = Unwrap<SSH2_Channel>( args.This() );
	channel->binary = args[0]->ToBoolean( args.GetIsolate() )->Value();	
}

void SSH2_Channel::ShellCallback( uintptr_t psv, LOGICAL success ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	makeEvent( event );
	event->code = SSH2_EVENT_SHELL;
	event->data = channel;
	event->success = success;
	EnqueLink( &channel->ssh2->eventQueue, event );
	uv_async_send( &channel->ssh2->async );
}

void SSH2_Channel::Shell( const v8::FunctionCallbackInfo<Value>& args ) {
	SSH2_Channel* channel = Unwrap<SSH2_Channel>( args.This() );
	Isolate* isolate = args.GetIsolate();
	Local<Promise::Resolver> pr = Promise::Resolver::New( isolate->GetCurrentContext() ).ToLocalChecked();
	channel->shellPromise.Reset( isolate, pr );
	EnqueLink( &channel->activePromises, &channel->shellPromise );
	//sack_ssh_set_shell_open( channel->channel, SSH2_Channel::ShellCallback );
	sack_ssh_channel_shell( channel->channel, SSH2_Channel::ShellCallback );
	args.GetReturnValue().Set( pr->GetPromise() );
}

void SSH2_Channel::ExecCallback( uintptr_t psv, LOGICAL success ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	makeEvent( event );
	event->code = SSH2_EVENT_EXEC;
	event->data = channel;
	event->success = success;
	EnqueLink( &channel->ssh2->eventQueue, event );
	uv_async_send( &channel->ssh2->async );
}


void SSH2_Channel::Exec( const v8::FunctionCallbackInfo<Value>& args ) {
	SSH2_Channel* channel = Unwrap<SSH2_Channel>( args.This() );
	Isolate* isolate = args.GetIsolate();
	String::Utf8Value* shell;
	if( args.Length() > 0 ) {
		shell = new String::Utf8Value( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	} else shell = NULL;
	Local<Promise::Resolver> pr = Promise::Resolver::New( isolate->GetCurrentContext() ).ToLocalChecked();
	channel->execPromise.Reset( isolate, pr );
	EnqueLink( &channel->activePromises, &channel->execPromise );
	sack_ssh_channel_exec( channel->channel, shell?**shell:"/bin/sh", SSH2_Channel::ExecCallback );

	args.GetReturnValue().Set( pr->GetPromise() );
}

void SSH2_Channel::SetEnvCallback( uintptr_t psv, LOGICAL success ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	makeEvent( event );
	event->code = SSH2_EVENT_SETENV;
	event->data = channel;
	event->success = success;
	EnqueLink( &channel->ssh2->eventQueue, event );
	uv_async_send( &channel->ssh2->async );
}

void SSH2_Channel::SetEnv( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8Literal( args.GetIsolate(), "setenv requires key and value parameters." ) ) );

	}
	SSH2_Channel* channel = Unwrap<SSH2_Channel>( args.This() );
	Local<Promise::Resolver> pr = Promise::Resolver::New( isolate->GetCurrentContext() ).ToLocalChecked();
	String::Utf8Value key( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	String::Utf8Value value( USE_ISOLATE( isolate ) args[1]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	channel->setenvPromise.Reset( isolate, pr );
	EnqueLink( &channel->activePromises, &channel->setenvPromise );
	sack_ssh_channel_setenv( channel->channel, *key, *value, SSH2_Channel::SetEnvCallback );

	args.GetReturnValue().Set( pr->GetPromise() );

}

void SSH2_Channel::Send( const v8::FunctionCallbackInfo<Value>& args ) {
	SSH2_Channel* channel = Unwrap<SSH2_Channel>( args.This() );
	if( args.Length() < 1 ) {
		args.GetIsolate()->ThrowException( Exception::TypeError(
			String::NewFromUtf8Literal( args.GetIsolate(), "Send requires data to send with optional stream" ) ) );
		return;
	}
	int n = 0;
	int stream = 0;
	if( args[0]->IsNumber() ) {
		n++;
		stream = args[0]->Int32Value( args.GetIsolate()->GetCurrentContext() ).FromMaybe( 0 );
	}

	if( args[n]->IsString() ) {
		String::Utf8Value data( args.GetIsolate(), args[n]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
		sack_ssh_channel_write( channel->channel, stream, (const uint8_t*)*data, data.length() );
	} else if( args[n]->IsArrayBuffer() ) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[n] );
		std::shared_ptr<BackingStore> content = ab->GetBackingStore();
		sack_ssh_channel_write( channel->channel, stream, (const uint8_t*)content->Data(), ab->ByteLength() );
	} else {
		args.GetIsolate()->ThrowException( Exception::TypeError(
			String::NewFromUtf8Literal( args.GetIsolate(), "Send requires data to send with optional stream" ) ) );
		return;
	}
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
	NetworkStart();
	if( args.IsConstructCall() ) {
		//lprintf( "SSH2_Object::New" );
		SSH2_Object* obj;
		obj = new SSH2_Object( isolate );
		obj->Wrap( args.This() );
		obj->jsObject.Reset( isolate, args.This() );

		//args.GetReturnValue().Set( args.This() );
	} else {
		//lprintf( "SSH2_Object::New(called)" );
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

void SSH2_Object::fingerprint( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SSH2_Object* ssh = Unwrap<SSH2_Object>( args.This() );
	Local<ArrayBuffer> ab;
#if ( NODE_MAJOR_VERSION >= 14 )
	std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( (POINTER)ssh->fingerprintData, 20, releaseBufferBackingStore, NULL );
	ab = ArrayBuffer::New( isolate, bs );
#else
	ab =
		ArrayBuffer::New( isolate,
			(void*)eventMessage->buf,
			eventMessage->buflen );

	PARRAY_BUFFER_HOLDER holder = GetHolder();
	holder->o.Reset( isolate, ab );
	holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
	holder->buffer = eventMessage->buf;
#endif
	args.GetReturnValue().Set( ab );
}

void SSH2_Object::handshook( uintptr_t psv, const uint8_t* string ) {
	SSH2_Object*ssh= (SSH2_Object*)psv;
	//lprintf( "Handshake:%s", string );
	//LogBinary( string, strlen( string ) );
	MemCpy( ssh->fingerprintData, string, 20 );
	//lprintf( "Saving fingerprint" );
	
	makeEvent( event );
	event->code = SSH2_EVENT_HANDSHAKE;
	event->success = TRUE;
	EnqueLink( &ssh->eventQueue, event );
	uv_async_send( &ssh->async );
	
}

static uintptr_t ForwardCallback( uintptr_t session_psv, LOGICAL success ) {
	// this is received after the local connection is setup to forward connections
	return 0;
}

void SSH2_Object::Forward( const v8::FunctionCallbackInfo<Value>& args ) {
	SSH2_Object* ssh = Unwrap<SSH2_Object>( args.This() );
	int localport = 0;
	int remoteport = 0;
	Isolate* isolate = args.GetIsolate();
	
	String::Utf8Value localHost( args.GetIsolate(), args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
	if( args.Length() > 1 )
		localport = args[1]->Int32Value( args.GetIsolate()->GetCurrentContext() ).FromMaybe( 0 );
	String::Utf8Value remoteHost( args.GetIsolate(), args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
	if( args.Length() > 1 )
		remoteport = args[1]->Int32Value( args.GetIsolate()->GetCurrentContext() ).FromMaybe( 0 );

	lprintf( " Forward is not implemented yet" );
	//sack_ssh_direc
	PCLIENT pcListener = sack_ssh_forward_connect( ssh->session, *localHost, localport, *remoteHost, remoteport, ForwardCallback );

}

static void ReverseChannelData( uintptr_t psv, int stream, const uint8_t* data, size_t length ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	lprintf( "ReverseChannelData" );
	LogBinary( data, length );
	WebSocketWrite( (struct html5_web_socket*)channel->remoteListen->wss->wsPipe, data, length );
}

static int ReverseChannelSend( uintptr_t psv, CPOINTER data, size_t length ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	lprintf( "ReverseChannelSend" );
	LogBinary( data, length );
	sack_ssh_channel_write( channel->channel, 0, (const uint8_t*)data, length );
	return length;
}

static void ReverseChannelClose( uintptr_t psv ) {
	SSH2_Channel* channel = (SSH2_Channel*)psv;
	lprintf( "ReverseChannelClose" );
	sack_ssh_channel_close( channel->channel );
}

static uintptr_t ReverseConnectCallback( uintptr_t psv, struct ssh_channel* channel ) {
	struct SSH2_RemoteListen* listener = (struct SSH2_RemoteListen*)psv;
	sack_ssh_set_channel_data( channel, ReverseChannelData );
	makeEvent( event );
	event->code = SSH2_EVENT_REVERSE_CONNECT;
	event->data = (void*)channel;
	event->data2 = listener->wss->wsPipe;
	event->waiter = MakeThread();
	event->done = 0;
	EnqueLink( &listener->ssh2->eventQueue, event );
	uv_async_send( &listener->ssh2->async );
	while( !event->done ) {
		WakeableSleep( 1000 );
	}
	SSH2_Channel* newChannel = (SSH2_Channel*)event->data;
	newChannel->remoteListen = listener;
	WebSocketPipeSetSend( listener->wss->wsPipe, ReverseChannelSend, (uintptr_t)newChannel );
	WebSocketPipeSetClose( listener->wss->wsPipe, ReverseChannelClose, (uintptr_t)newChannel );
	uintptr_t result = (uintptr_t)event->data;
	dropEvent( event );
	return result;
}


static uintptr_t ReverseCallback( uintptr_t psv, struct ssh_listener* listener, int boundPort ) {
	SSH2_Object* ssh = (SSH2_Object*)psv;
	makeEvent( event );
	sack_ssh_set_forward_listen_accept( listener, ReverseConnectCallback );
	event->code = SSH2_EVENT_REVERSE_CHANNEL;
	event->data = (void*)listener;
	event->waiter = MakeThread();
	event->done = 0;
	EnqueLink( &ssh->eventQueue, event );
	uv_async_send( &ssh->async );
	while( !event->done ) {
		WakeableSleep( 1000 );
	}
	uintptr_t result = (uintptr_t)event->data;
	dropEvent( event );
	return (uintptr_t)result;

}

void SSH2_Object::Reverse( const v8::FunctionCallbackInfo<Value>&args ) {
	SSH2_Object* ssh = Unwrap<SSH2_Object>( args.This() );
	int port = 0;
	Isolate* isolate = args.GetIsolate();
	String::Utf8Value remoteHost( args.GetIsolate(), args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
	if( args.Length() > 1 )
		port = args[1]->Int32Value( args.GetIsolate()->GetCurrentContext() ).FromMaybe( 0 );

	sack_ssh_forward_listen( ssh->session, *remoteHost, port, ReverseCallback );

	Local<Promise::Resolver> pr = Promise::Resolver::New( isolate->GetCurrentContext() ).ToLocalChecked();
	ssh->connectPromise.Reset( isolate, pr );
	ssh->activePromise = &ssh->connectPromise;
	args.GetReturnValue().Set( pr->GetPromise() );
}

static Local<Promise::Resolver> doLogin( SSH2_Object *ssh, Isolate* isolate, Local<Context> context, Local<Object> opts ) {
	size_t privkeylen = 0;
	uint8_t* privKeyBuf;
	size_t pubkeylen = 0;
	uint8_t* pubKeyBuf;
	Local<String> optName;
	struct optionStrings* strings = getStrings( isolate );

	GET_STRING( user );
	GET_STRING( password );
	GET_STRING( pubKey );
	GET_STRING( privKey );
	GET_ARRAY_BUFFER( pubKey );
	GET_ARRAY_BUFFER( privKey );
	GET_TYPED_ARRAY( pubKey );
	GET_TYPED_ARRAY( privKey );

	if( privKey ) {
		privKeyBuf = (uint8_t*)*privKey[0];
		privkeylen = privKey[0].length();
	} else if( !privKey_ab.IsEmpty() ) {
		privKeyBuf = (uint8_t*)privKey_ab->GetBackingStore()->Data();
		privkeylen = privKey_ab->ByteLength();
	} else if( !privKey_ta.IsEmpty() ) {
		Local<ArrayBuffer> ab = privKey_ta->Buffer();
		privKeyBuf = (uint8_t*)ab->GetBackingStore()->Data();
		privkeylen = ab->ByteLength();
	} else
		privKeyBuf = NULL;
	if( pubKey ) {
		pubKeyBuf = (uint8_t*)*pubKey[0];
		pubkeylen = pubKey[0].length();
	} else if( !pubKey_ab.IsEmpty() ) {
		pubKeyBuf = (uint8_t*)pubKey_ab->GetBackingStore()->Data();
		pubkeylen = pubKey_ab->ByteLength();
	} else if( !pubKey_ta.IsEmpty() ) {
		Local<ArrayBuffer> ab = pubKey_ta->Buffer();
		pubKeyBuf = (uint8_t*)ab->GetBackingStore()->Data();
		pubkeylen = ab->ByteLength();
	} else pubKeyBuf = NULL;

	if( privKeyBuf )
		sack_ssh_auth_user_cert( ssh->session, user ? *user[0] : NULL, pubKeyBuf, pubkeylen, privKeyBuf, privkeylen, password ? *password[0] : NULL, SSH2_Object::authDone );
	else
		sack_ssh_auth_user_password( ssh->session, user ? *user[0] : NULL, password ? *password[0] : NULL, SSH2_Object::authDone );

	Local<Promise::Resolver> pr = Promise::Resolver::New( isolate->GetCurrentContext() ).ToLocalChecked();
	ssh->loginPromise.Reset( isolate, pr );
	//ssh->activePromise = &ssh->connectPromise;
	//args.GetReturnValue().Set( pr->GetPromise() );
	return pr;
}

void SSH2_Object::Login( const v8::FunctionCallbackInfo<Value>& args ) {
	SSH2_Object* ssh = Unwrap<SSH2_Object>( args.This() );
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	doLogin( ssh, isolate, context, ssh->connectOptions.Get( isolate ) );
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

	Local<Promise::Resolver> pr = Promise::Resolver::New( isolate->GetCurrentContext() ).ToLocalChecked();
	ssh->connectPromise.Reset( isolate, pr );
	ssh->activePromise = &ssh->connectPromise;
	Local<String> optName;
	Local<Function> connect;
	// requires opts
	GET_STRING( address );
	GET_STRING( user );
	GET_STRING( password );
	GET_STRING( pubKey );
	GET_STRING( privKey );
	GET_ARRAY_BUFFER( pubKey );
	GET_ARRAY_BUFFER( privKey );
	GET_TYPED_ARRAY( pubKey );
	GET_TYPED_ARRAY( privKey );
	GET_BOOL( skipLogin );
	GET_BOOL( trace );

	if( trace )
		sack_ssh_trace( ssh->session, ~0 );

	ssh->connectOptions.Reset( isolate, opts );

	sack_ssh_session_connect( ssh->session, *address[0], 22, SSH2_Object::handshook );

	if( !skipLogin ) {
		// can queue this immediately after starting a connect...
		
		doLogin( ssh, isolate, context, opts );
	}
	args.GetReturnValue().Set( pr->GetPromise() );

}

SSH2_RemoteListen::SSH2_RemoteListen() {

}

SSH2_RemoteListen::~SSH2_RemoteListen() {

}

void SSH2_RemoteListen::New( const v8::FunctionCallbackInfo<Value>& args ) {
	// this constructor is only called from internal code
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( args.IsConstructCall() ) {
		SSH2_RemoteListen* obj;
		obj = new SSH2_RemoteListen();
		obj->Wrap( args.This() );
		obj->jsObject.Reset( isolate, args.This() );
		args.GetReturnValue().Set( args.This() );
	} else {
		class constructorSet* c = getConstructors( isolate );
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Value>* argv = new Local<Value>[0];
		Local<Function> cons = Local<Function>::New( isolate, c->SSH_RemoteListen_constructor );
		MaybeLocal<Object> mo = cons->NewInstance( isolate->GetCurrentContext(), 0, argv );
		if( !mo.IsEmpty() )
			args.GetReturnValue().Set( mo.ToLocalChecked() );
		delete[] argv;
	}
}

void SSH2_RemoteListen::Close( const v8::FunctionCallbackInfo<Value>& args ) {
}

void SSH2_RemoteListen::Error( uintptr_t psv, int errcode, const char* string, int errlen ) {
	SSH2_RemoteListen* listener = (SSH2_RemoteListen*)psv;
	INDEX idx;
	PTHREAD test;
	PTHREAD self = MakeThread();
	LIST_FORALL( global.rootThreads, idx, PTHREAD, test ) if( test == self ) break;
	if( test ) string = (const char*)StrDup( string );
	makeEvent( event );
	event->code = SSH2_EVENT_LISTEN_ERROR;
	event->iData = errcode;
	event->data = (void*)string;
	event->data2 = listener;
	event->length = errlen;
	event->done = 0;
	event->waiter = test ? NULL : self;
	EnqueLink( &listener->ssh2->eventQueue, event );
	uv_async_send( &listener->ssh2->async );
	if( event->waiter ) {
		while( !event->done ) {
			WakeableSleep( 1000 );
		}
		dropEvent( event );
	}
}


// - - - - - - Port Forward Test - - - - - - - - - - -

#if 0
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
	const char* username = "";
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

#endif
