
#include "global.h"


struct msgbuf {
	int closeEvent;
	size_t buflen;
	uint8_t buf[1];
};

class ComObject : public node::ObjectWrap {
public:
	int handle;
	char* name;
	//static Persistent<Function> constructor;
	bool rts = 1;
	Persistent<Function, CopyablePersistentTraits<Function>>* readCallback; //
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE readQueue;

public:

	static void Init( Local<Object> exports );
	ComObject( char* name );
	Persistent<Object> jsObject;

private:
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void getPorts( Local<Name> property, const PropertyCallbackInfo<Value>& info );
	static void getRTS2( Local<Name> property, const PropertyCallbackInfo<Value>& args );
	//static void getRTS( const v8::FunctionCallbackInfo<Value>& args );
	static void setRTS2( Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void>& args );
	//static void setRTS( const v8::FunctionCallbackInfo<Value>& args );
	static void onRead( const v8::FunctionCallbackInfo<Value>& args );
	static void writeCom( const v8::FunctionCallbackInfo<Value>& args );
	static void closeCom( const v8::FunctionCallbackInfo<Value>& args );
	~ComObject();
};



ComObject::ComObject( char *name ) : jsObject() {
	this->readQueue = CreateLinkQueue();
	this->name = name;
	handle = SackOpenComm( name, 0, 0 );
}

ComObject::~ComObject() {
	if( handle >=0 )
		SackCloseComm( handle );
  	Deallocate( char*, name );
}

void ComObjectInit( Local<Object> exports ) {
	ComObject::Init( exports );
}

void test( Local<Name> property,
	const PropertyCallbackInfo<Value>& info ) {
}


void ComObject::Init( Local<Object> exports ) {
		Isolate* isolate = Isolate::GetCurrent();
		Local<Context> context = isolate->GetCurrentContext();
		Local<FunctionTemplate> comTemplate;

		comTemplate = FunctionTemplate::New( isolate, New );
		comTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.ComPort" ) );
		comTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

		// Prototype
		NODE_SET_PROTOTYPE_METHOD( comTemplate, "onRead", onRead );
		NODE_SET_PROTOTYPE_METHOD( comTemplate, "write", writeCom );
		NODE_SET_PROTOTYPE_METHOD( comTemplate, "close", closeCom );

		comTemplate->PrototypeTemplate()->SetNativeDataProperty( String::NewFromUtf8Literal( isolate, "rts" )
			, ComObject::getRTS2
			, ComObject::setRTS2
			, Local<Value>()
			, PropertyAttribute::ReadOnly
			, SideEffectType::kHasNoSideEffect
			, SideEffectType::kHasSideEffect
		);

		/*
		comTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "rts" )
			, FunctionTemplate::New( isolate, ComObject::getRTS )
			, FunctionTemplate::New( isolate, ComObject::setRTS )
		);
		*/
		Local<Function> ComFunc = comTemplate->GetFunction( isolate->GetCurrentContext() ).ToLocalChecked();
		/*
		ComFunc->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "ports" )
			, Function::New( context, ComObject::getPorts ).FromMaybe( Local<Function>() )
			, Local<Function>()
			, PropertyAttribute::ReadOnly );
			*/
		// PropertyCallbackInfo
		//Object::DefineAccessor
		/*
		Handle<i::AccessorInfo> info =
			MakeAccessorInfo( i_isolate, name, getter, setter, data, settings,
				is_special_data_property, replace_on_access );
		*/
		//info->set_getter_side_effect_type( getter_side_effect_type );
		//info->set_setter_side_effect_type( setter_side_effect_type );

		
		ComFunc->SetNativeDataProperty( context, String::NewFromUtf8Literal( isolate, "ports" )
			, ComObject::getPorts
			, nullptr //Local<Function>()
			, Local<Value>()
			, PropertyAttribute::ReadOnly
			, SideEffectType::kHasSideEffect
			, SideEffectType::kHasSideEffect
		);
		

		class constructorSet *c = getConstructors( isolate );
		c->comConstructor.Reset( isolate, ComFunc );
		SET( exports, "ComPort", ComFunc );
}


void ComObject::getRTS2( Local<Name> property, const PropertyCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	ComObject* obj = ObjectWrap::Unwrap<ComObject>( args.This() );
	if( obj )
		args.GetReturnValue().Set( Boolean::New( args.GetIsolate(), (int)obj->rts ) );

}

/*
void ComObject::getRTS( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	ComObject* obj = ObjectWrap::Unwrap<ComObject>( args.This() );
	if( obj )
		args.GetReturnValue().Set( Boolean::New( args.GetIsolate(), (int)obj->rts ) );

}
*/
void ComObject::setRTS2( Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void>& args ) {
	Isolate* isolate = args.GetIsolate();
	ComObject* obj = ObjectWrap::Unwrap<ComObject>( args.This() );
	if( obj )
		SetCommRTS( obj->handle, obj->rts = value.As<Boolean>()->BooleanValue( isolate ) );
}

/*
void ComObject::setRTS( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() > 0 ) {
		ComObject* obj = ObjectWrap::Unwrap<ComObject>( args.This() );
		if( obj )
			SetCommRTS( obj->handle, obj->rts = args[0].As<Boolean>()->BooleanValue( isolate ) );
	}
}
*/

void dont_releaseBufferBackingStore(void* data, size_t length, void* deleter_data) {
	(void)length;
	(void)deleter_data;;
}

static void asyncmsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	ComObject* myself = (ComObject*)handle->data;

	HandleScope scope(isolate);
	{
		struct msgbuf *msg;
		while( msg = (struct msgbuf *)DequeLink( &myself->readQueue ) ) {
			size_t length;
			if( msg->closeEvent ) {
				myself->jsObject.Reset();
				Release( msg );
				uv_close( (uv_handle_t*)&myself->async, NULL ); // have to hold onto the handle until it's freed.
				return;
			}
#if ( NODE_MAJOR_VERSION >= 14 )
			std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( msg->buf, length=msg->buflen, dont_releaseBufferBackingStore, NULL );
			Local<ArrayBuffer> ab = ArrayBuffer::New( isolate, bs );
#else
			Local<ArrayBuffer> ab =
				ArrayBuffer::New( isolate,
											  msg->buf,
											  length = msg->buflen );

#endif

			Local<Uint8Array> ui = Uint8Array::New( ab, 0, length );

			Local<Value> argv[] = { ui };
			Local<Function> cb = Local<Function>::New( isolate, myself->readCallback[0] );
			//lprintf( "callback ... %p", myself );
			// using obj->jsThis  fails. here...
			{
				MaybeLocal<Value> result = cb->Call( isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, argv );
				if( result.IsEmpty() ) {
					Deallocate( struct msgbuf *, msg );
					return;
				}
			}
			//lprintf( "called ..." );	
			Deallocate( struct msgbuf *, msg );
		}
	}
	//lprintf( "done calling message notice." );
	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}

void ComObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		class constructorSet *c = getConstructors( isolate );
		if( args.IsConstructCall() ) {
			char *portName;
			int argc = args.Length();
			if( argc > 0 ) {
				String::Utf8Value fName( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
				portName = StrDup( *fName );
			} else {
				isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Must specify port name to open." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
				return;
			}
			// Invoked as constructor: `new MyObject(...)`
			ComObject* obj = new ComObject( portName );
			if( obj->handle < 0 )
			{
				char msg[256];
				snprintf( msg, 256, "Failed to open %s", obj->name );
				isolate->ThrowException( Exception::Error( String::NewFromUtf8(isolate, msg, v8::NewStringType::kNormal ).ToLocalChecked() ) );
			}
			else {
				//lprintf( "empty async...." );
				//MemSet( &obj->async, 0, sizeof( obj->async ) );
				//Environment* env = Environment::GetCurrent(args);
				uv_async_init( c->loop, &obj->async, asyncmsg );
				obj->async.data = obj;
				obj->jsObject.Reset( isolate, args.This() );
				obj->Wrap( args.This() );
				args.GetReturnValue().Set( args.This() );
			}

		}
		else {
			// Invoked as plain function `MyObject(...)`, turn into construct call.
			int argc = args.Length();
			Local<Value> *argv = new Local<Value>[argc];
			for( int n = 0; n < argc; n++ )
				argv[n] = args[n];

			Local<Function> cons = Local<Function>::New( isolate, c->comConstructor );
			MaybeLocal<Object> newInst = cons->NewInstance( isolate->GetCurrentContext(), argc, argv );
			if( !newInst.IsEmpty() )
				args.GetReturnValue().Set( newInst.ToLocalChecked() );
			delete[] argv;
		}
}



static void CPROC dispatchRead( uintptr_t psv, int nCommId, POINTER buffer, int len ) {
	struct msgbuf *msgbuf = NewPlus( struct msgbuf, len );
	//lprintf( "got read: %p %d", buffer, len );
	msgbuf->closeEvent = 0;
	MemCpy( msgbuf->buf, buffer, len );
	msgbuf->buflen = len;
	ComObject *com = (ComObject*)psv;
	if( !com->readCallback->IsEmpty() ) {
		EnqueLink( &com->readQueue, msgbuf );
		uv_async_send( &com->async );
	}
}


void ComObject::onRead( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc < 1 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Must pass callback to onRead handler" ) ) );
		return;
	}

	ComObject *com = ObjectWrap::Unwrap<ComObject>( args.This() );

	if( com->handle >= 0 ) {
		SackSetReadCallback( com->handle, dispatchRead, (uintptr_t)com );
	}

	Local<Function> arg0 = Local<Function>::Cast( args[0] );
	com->readCallback = new Persistent<Function,CopyablePersistentTraits<Function>>(isolate,arg0);
}

void ComObject::writeCom( const v8::FunctionCallbackInfo<Value>& args ) {
	int argc = args.Length();
	if( argc < 1 ) {
		//isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "required parameter missing" ) ) );
		return;
	}
	Isolate* isolate = args.GetIsolate();
	ComObject *com = ObjectWrap::Unwrap<ComObject>( args.This() );

	//assert(args[i]->IsFloat32Array());
	if (args[0]->IsString()) {
		String::Utf8Value u8str( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked());
		SackWriteComm(com->handle, *u8str, u8str.length());
	}
	else if (args[0]->IsUint8Array()) {
		Local<Uint8Array> myarr = args[0].As<Uint8Array>();
#if ( NODE_MAJOR_VERSION >= 14 )
		std::shared_ptr<BackingStore> ab_c = myarr->Buffer()->GetBackingStore();
		char *buf = static_cast<char*>(ab_c->Data()) + myarr->ByteOffset();
#else
		ArrayBuffer::Contents ab_c = myarr->Buffer()->GetContents();
		char *buf = static_cast<char*>(ab_c.Data()) + myarr->ByteOffset();
#endif
		SackWriteComm( com->handle, buf, (int)myarr->Length() );
	}
	else if (args[0]->IsArrayBuffer()) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
#if ( NODE_MAJOR_VERSION >= 14 )
		std::shared_ptr<BackingStore> ab_c = ab->GetBackingStore();
		char *buf = static_cast<char*>(ab_c->Data()) ;
#else
		ArrayBuffer::Contents ab_c = ab->GetContents();
		char *buf = static_cast<char*>(ab_c.Data()) ;
#endif
		SackWriteComm( com->handle, buf, (int)ab->ByteLength() );
	}

}

void ComObject::closeCom( const v8::FunctionCallbackInfo<Value>& args ) {
	
	ComObject *com = ObjectWrap::Unwrap<ComObject>( args.This() );
	if( com->handle >= 0 )
		SackCloseComm( com->handle );
	com->handle = -1;

	{
		//lprintf( "Garbage collected" );
		struct msgbuf* msgbuf = NewPlus( struct msgbuf, 0 );
		msgbuf->closeEvent = 1;
		EnqueLink( &com->readQueue, msgbuf );
		uv_async_send( &com->async );
	}

}


struct port_info {
	size_t portLen;
	size_t nameLen;
	size_t techLen;
	TEXTCHAR buf[];
};

static LOGICAL portCallback( uintptr_t psv, LISTPORTS_PORTINFO* lpPortInfo ) {
	size_t portLen = StrLen( lpPortInfo->lpPortName );
	size_t nameLen = StrLen( lpPortInfo->lpFriendlyName );
	size_t techLen = StrLen( lpPortInfo->lpTechnology );

	struct port_info *pi = NewPlus( struct port_info, portLen+nameLen+techLen );
	pi->portLen = portLen;
	pi->nameLen = nameLen;
	pi->techLen = techLen;
	MemCpy( pi->buf, lpPortInfo->lpPortName, portLen );
	MemCpy( pi->buf+portLen, lpPortInfo->lpFriendlyName, nameLen );
	MemCpy( pi->buf+portLen+nameLen, lpPortInfo->lpTechnology, techLen );
	PLIST* ppList = (PLIST*)psv;
	AddLink( ppList, pi );
	return TRUE;
}

void ComObject::getPorts( Local<Name> property, const PropertyCallbackInfo<Value>& args ) {
	//( const v8::FunctionCallbackInfo<Value>& args ) {
	PLIST list = NULL;
	ListPorts( portCallback, (uintptr_t)&list );
	INDEX idx;
	struct port_info *pi;

	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Array> jsList = Array::New( isolate );

	LIST_FORALL( list, idx, struct port_info*, pi ) {
		Local<Object> info = Object::New( isolate );
		SET( info, "port", String::NewFromUtf8( isolate, pi->buf, v8::NewStringType::kNormal, pi->portLen ).ToLocalChecked() );
		SET( info, "name", String::NewFromUtf8( isolate, pi->buf+pi->portLen, v8::NewStringType::kNormal, pi->nameLen ).ToLocalChecked() );
		SET( info, "technology", String::NewFromUtf8( isolate, pi->buf+pi->portLen+pi->nameLen, v8::NewStringType::kNormal, pi->techLen ).ToLocalChecked() );
		SETN( jsList, idx, info );
		Deallocate( struct port_info*, pi );
	}
	//lprintf( "Returning %d ports", idx );
	args.GetReturnValue().Set( jsList );	
}
