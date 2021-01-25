#include "global.h"
using namespace sack::SACK_VFS::objStore;

//#define LOG_DISK_TIME
//#define DEBUG_LOG_PARSING
//#define DEBUG_LOG_OUTPUT

class ObjectStorageObject : public node::ObjectWrap {
public:
	uv_async_t async;
	PLINKQUEUE plqEvents;
	struct objStore::sack_vfs_os_volume *vol;
	bool volNative;
	char *mountName;
	char *fileName;
	struct file_system_interface *fsInt;
	struct file_system_mounted_interface* fsMount;
	LOGICAL thrown;

public:

	static void Init( Isolate *isolate, Local<Object> exports );
	ObjectStorageObject( const char *mount, const char *filename, uintptr_t version, const char *key, const char *key2, struct file_system_mounted_interface* useMount );

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	// get object pass object ID
	static void getObject( const v8::FunctionCallbackInfo<Value>& args );

	// get object and all recursive objects associated from here (for 1 level?)
	static void mapObject( const v8::FunctionCallbackInfo<Value>& args );

	// pass object, parser, result with object ID.
	static void putObject( const v8::FunctionCallbackInfo<Value>& args );

	// pass ID to remove from storage
	static void removeObject( const v8::FunctionCallbackInfo<Value>& args );

	// pass object ID, get back a ObjectStorageFileObject ( support seek/read/write? )
	static void openObject( const v8::FunctionCallbackInfo<Value>& args );

	// utility to remove the key so it can be diagnosed.
	static void volDecrypt( const v8::FunctionCallbackInfo<Value>& args );

	static void fileReadJSOX( const v8::FunctionCallbackInfo<Value>& args );
	static void fileRead( const v8::FunctionCallbackInfo<Value>& args );
	static void flush( const v8::FunctionCallbackInfo<Value>& args );

	static void fileWrite( const v8::FunctionCallbackInfo<Value>& args );
	static void fileStore( const v8::FunctionCallbackInfo<Value>& args );
	static void createIndex( const v8::FunctionCallbackInfo<Value>& args );
	static ObjectStorageObject* openInVFS( Isolate *isolate, const char *mount, const char *name, const char *key1, const char *key2, struct file_system_mounted_interface* useMount );

	Local<Object> WrapObjectStorage( Isolate* isolate ) {
		Local<Object> o = Object::New( isolate );
		this->Wrap( o );
		return o;
	}

	~ObjectStorageObject();
};


Local<Object> WrapObjectStorage( Isolate* isolate, class ObjectStorageObject* oso ) {
	return oso->WrapObjectStorage( isolate );
}


struct optionStrings {
	Isolate *isolate;

	Eternal<String> *objectHashString;
	Eternal<String> *sealantString;
	Eternal<String> *storedString;
	Eternal<String> *failedString;
	Eternal<String> *signedString;
	Eternal<String> *readKeyString;
	Eternal<String> *thenString;
	Eternal<String> *catchString;
	Eternal<String> *aString;
	Eternal<String> *dataString;
	Eternal<String> *nameString;
	Eternal<String> *optsString;
};


enum objectStorageEvents {
	OSEV_CLOSE,
	OSEV_STORED,
	OSEV_SUCCESS,
	OSEV_FAIL,
};
struct ObjectStorageEvent {
	enum objectStorageEvents op;
	struct objectStorageOptions *storageOpts;
};


struct objectStorageOptions {
	ObjectStorageObject *vol;
	int set;
	char *sealant;
	size_t sealantLen;
	char *data;
	size_t dataLen;
	char *objectHash;
	size_t objectHashLen;
	char *readKey;
	size_t readKeyLen;
	char result[64];//
	Persistent<Function> cbStored;
	Persistent<Function> cbFailed;
	objectStorageOptions() : cbStored( ), cbFailed( ) {

	}
};


typedef struct ObjectStorageEvent OBJECT_STORAGE_EVENT;
typedef struct ObjectStorageEvent *POBJECT_STORAGE_EVENT;
#define MAXOBJECT_STORAGE_EVENTSPERSET 128
DeclareSet( OBJECT_STORAGE_EVENT );

struct objectStorageTransport {
	class ObjectStorageObject *oso;
};

struct objectStorageUnloadStation {
	Persistent<Object> this_;
	String::Utf8Value* s;  // destination address
	uv_async_t poster;
	uv_loop_t  *targetThread;
	Persistent<Function> cb; // callback to invoke
	PLIST transport;
};


static struct objStoreLocal {
	PLIST open;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	POBJECT_STORAGE_EVENTSET osEvents;
	PLIST strings;
	PLIST transportDestinations;
} osl;


ATEXIT( closeVolumes ) {
	INDEX idx;
	struct objStore::sack_vfs_os_volume* vol;
	LIST_FORALL( osl.open, idx, struct objStore::sack_vfs_os_volume*, vol ) {
		sack_vfs_os_unload_volume( vol );
	}
}


static struct optionStrings *getStrings( Isolate *isolate ) {
	INDEX idx;
	struct optionStrings * check;
	LIST_FORALL( osl.strings, idx, struct optionStrings *, check ) {
		if( check->isolate == isolate )
			break;
	}
	if( !check ) {
		check = NewArray( struct optionStrings, 1 );
		AddLink( &osl.strings, check );
		check->isolate = isolate;
#define DEFSTRING(s) check->s##String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, #s, v8::NewStringType::kNormal ).ToLocalChecked() )
		DEFSTRING( objectHash );
		DEFSTRING( sealant );
		DEFSTRING( stored );
		DEFSTRING( failed );
		DEFSTRING( signed );
		DEFSTRING( readKey );
		DEFSTRING( data );
		DEFSTRING( name );
		DEFSTRING( opts );
	}
	return check;
}

static void objStoreEventHandler( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	ObjectStorageObject* obj = (ObjectStorageObject*)handle->data;
	//udpEvent *eventMessage;
	struct ObjectStorageEvent *event;
	HandleScope scope( isolate );

	while( event = (struct ObjectStorageEvent*)DequeLink( &obj->plqEvents ) ) {

		switch( event->op ) {
		case OSEV_STORED:
			// post that this object has beens tored
			// and give back the appropriate things.
			break;
		case OSEV_CLOSE:
			uv_close( (uv_handle_t*)&obj->async, NULL );
			break;
		}
		DeleteFromSet( OBJECT_STORAGE_EVENT, osl.osEvents, event );
	}
	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}

}

static void postEvent( ObjectStorageObject *_this, enum objectStorageEvents evt, ... ) {
	//= (udpObject*)psv;
	va_list args;
	va_start( args, evt );
	struct ObjectStorageEvent *pevt = GetFromSet( OBJECT_STORAGE_EVENT, &osl.osEvents );
	(*pevt).op = OSEV_CLOSE;
	switch( evt ) {
	case OSEV_SUCCESS:
		pevt->storageOpts = va_arg( args, struct objectStorageOptions * );
		break;
	case OSEV_FAIL:
		pevt->storageOpts = va_arg( args, struct objectStorageOptions * );
		break;

	default:
	case OSEV_CLOSE:
		/* no additional parameters */
		break;
	}
	EnqueLink( &_this->plqEvents, pevt );
	uv_async_send( &_this->async );
}



static LOGICAL PostObjectStorage( Isolate *isolate, String::Utf8Value *name, ObjectStorageObject* obj ) {
	struct objectStorageTransport* trans = new struct objectStorageTransport();
	trans->oso = obj;

	{
		struct objectStorageUnloadStation* station;
		INDEX idx;
		LIST_FORALL( osl.transportDestinations, idx, struct objectStorageUnloadStation*, station ) {
			if( memcmp( *station->s[0], *(name[0]), (name[0]).length() ) == 0 ) {
				AddLink( &station->transport, trans );
				//lprintf( "Send Post Request %p", station->poster );
				uv_async_send( &station->poster );
				break;
			}
		}
		if( !station ) {
			return FALSE;
			//isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Failed to find target accepting thread", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		}
	}
	return TRUE;
}

static void postObjectStorage( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Required parameter missing: (unique,socket)", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	ObjectStorageObject* obj = ObjectStorageObject::Unwrap<ObjectStorageObject>( args[1].As<Object>() );
	if( obj ){
		String::Utf8Value s( isolate, args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		if( PostObjectStorage( isolate, &s, obj ) )
			args.GetReturnValue().Set( True(isolate) );
		else
			args.GetReturnValue().Set( False(isolate) );

	}
	else {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Second paramter is not an accepted socket", v8::NewStringType::kNormal ).ToLocalChecked() ) );
	}
}



static void postObjectStorageObject( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 1 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Required parameter missing: (unique)", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}

	ObjectStorageObject* obj = ObjectStorageObject::Unwrap<ObjectStorageObject>( args.This() );
	if( obj ){
		String::Utf8Value s( isolate, args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		if( PostObjectStorage( isolate, &s, obj ) )
			args.GetReturnValue().Set( True(isolate) );
		else
			args.GetReturnValue().Set( False(isolate) );
	}
	else {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Object is not an accepted socket", v8::NewStringType::kNormal ).ToLocalChecked() ) );
	}
}


static void finishPostClose( uv_handle_t *async ) {
	struct objectStorageUnloadStation* unload = ( struct objectStorageUnloadStation* )async->data;
	delete unload->s;
	delete unload;
}

static void handlePostedObjectStorage( uv_async_t* async ) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	class constructorSet* c = getConstructors( isolate );
	struct objectStorageUnloadStation* unload = ( struct objectStorageUnloadStation* )async->data;
	Local<Function> f = unload->cb.Get( isolate );
	INDEX idx;
	struct objectStorageTransport* trans;
	LIST_FORALL( unload->transport, idx, struct objectStorageTransport*, trans ) {


		Local<Function> cons = Local<Function>::New( isolate, c->ObjectStorageObject_constructor );
		Local<Object> newThreadObject = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();

		Local<Value> args[] = { localStringExternal( isolate, **( unload->s ), unload->s->length() ), newThreadObject };
		ObjectStorageObject* obj = ObjectStorageObject::Unwrap<ObjectStorageObject>( newThreadObject );

		obj->vol = trans->oso->vol;
		obj->volNative = trans->oso->volNative;
		obj->mountName = trans->oso->mountName;
		obj->fileName = trans->oso->fileName;
		obj->fsInt = trans->oso->fsInt;
		obj->fsMount = trans->oso->fsMount;
		obj->thrown = trans->oso->thrown = TRUE;

	//lprintf( "having copied all the data to the new one, erase the old one. %p %p", trans->oso, obj );
	trans->oso->vol = NULL;
	trans->oso->volNative = NULL;
	trans->oso->mountName = NULL;
	trans->oso->fileName = NULL;
	trans->oso->fsInt = NULL;
	trans->oso->fsMount = NULL;



		MaybeLocal<Value> ml_result = f->Call( context, unload->this_.Get(isolate), 2, args );
		if( !ml_result.IsEmpty() ) {
			Local<Value> result = ml_result.ToLocalChecked();
			if( result->TOBOOL(isolate) ) {
			}
		}
		//lprintf( "Cleanup this event.." );
		unload->cb.Reset();
		unload->this_.Reset();
		DeleteLink( &osl.transportDestinations, unload );
		SetLink( &unload->transport, 0, NULL );
		uv_close( (uv_handle_t*)async, finishPostClose ); // have to hold onto the handle until it's freed.
		break;
	}
	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}

static void setClientObjectStorageHandler( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	class constructorSet* c = getConstructors( isolate );
	String::Utf8Value unique( isolate, args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );

	struct objectStorageUnloadStation* unloader = new struct objectStorageUnloadStation();
	unloader->this_.Reset( isolate, args.This() );
	unloader->s = new String::Utf8Value( isolate, args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	if( args[1]->IsFunction() )
		unloader->cb.Reset( isolate, args[1].As<Function>() );
	unloader->targetThread = c->loop;
	unloader->poster.data = unloader;
	unloader->transport = NULL;
	//lprintf( "New async event handler for this unloader%p", &unloader->clientSocketPoster );

	uv_async_init( unloader->targetThread, &unloader->poster, handlePostedObjectStorage );

	AddLink( &osl.transportDestinations, unloader );
}




void ObjectStorageObject::Init( Isolate *isolate, Local<Object> exports ) {

	//if( !l.loop )
	//	l.loop = uv_default_loop();
	Local<Context> context = isolate->GetCurrentContext();

	Local<FunctionTemplate> clsTemplate;
	clsTemplate = FunctionTemplate::New( isolate, New );
	clsTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.ObjectStorage", v8::NewStringType::kNormal ).ToLocalChecked() );
	clsTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "read", ObjectStorageObject::fileReadJSOX );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "readRaw", ObjectStorageObject::fileRead );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "writeRaw", ObjectStorageObject::fileWrite );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "createIndex", ObjectStorageObject::createIndex );
	//NODE_SET_PROTOTYPE_METHOD( clsTemplate, "store", ObjectStorageObject::fileStore );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "put", ObjectStorageObject::putObject );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "get", ObjectStorageObject::getObject );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "delete", ObjectStorageObject::removeObject );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "flush", ObjectStorageObject::flush );

	Local<Function> VolFunc = clsTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();

	class constructorSet* c = getConstructors( isolate );
	Local<Function> objectStoreFunc = clsTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
	c->ObjectStorageObject_constructor.Reset( isolate, objectStoreFunc );

	{
		Local<Object> threadObject = Object::New( isolate );
		SET_READONLY_METHOD( threadObject, "post", postObjectStorage );
		SET_READONLY_METHOD( threadObject, "accept", setClientObjectStorageHandler );
		SET_READONLY( objectStoreFunc, "Thread", threadObject );
	}

	SET( exports, "ObjectStorage"
		, clsTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

}


ObjectStorageObject::~ObjectStorageObject() {
	lprintf( "ObjectStorage object evaporated. %p", this );
	if( volNative ) {
		Deallocate( char*, mountName );
		Deallocate( char*, fileName );
		sack_unmount_filesystem( fsMount );
		objStore::sack_vfs_os_unload_volume( vol );
		DeleteLink( &osl.open, vol );
	}
}

ObjectStorageObject::ObjectStorageObject( const char *mount, const char *filename, uintptr_t version, const char *key
	, const char *key2, struct file_system_mounted_interface*useMount ) {
	mountName = (char *)mount;
	if( !mount && !filename ) {
		// no native storage.
	}
	else if( mount && !filename ) {
		volNative = false;
		fsMount = sack_get_mounted_filesystem( mount );
		fsInt = sack_get_mounted_filesystem_interface( fsMount );
		vol = (struct objStore::sack_vfs_os_volume*)sack_get_mounted_filesystem_instance( fsMount );

		//lprintf( "open native mount" );
	}
	else {
		//lprintf( "sack_vfs_os_volume: %s %p %p", filename, key, key2 );
		fileName = StrDup( filename );
		volNative = true;
		vol = objStore::sack_vfs_os_load_crypt_volume( filename, version, key, key2, useMount );
		AddLink( &osl.open, vol );
		//lprintf( "VOL: %p for %s %d %p %p", vol, filename, version, key, key2 );
		if( vol )
			fsMount = sack_mount_filesystem( mount, fsInt = sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME "-os" )
				, 2000, (uintptr_t)vol, TRUE );
		else
			fsMount = NULL;
	}
}


static uintptr_t CPROC DoPutObject( PTHREAD thread ) {
	struct objectStorageOptions osoOpts;
	struct objectStorageOptions **ppOptions = (struct objectStorageOptions **)GetThreadParam( thread );
	char *storeId;
	(*ppOptions) = &osoOpts;
	while( !osoOpts.set )
		Relinquish();


	// a specific name is passed...
	//    the name is hashed with sealant and that is the resulting ID.
	//    if no sealant,
	if( osoOpts.objectHash ) {
		if( osoOpts.sealant ) {
			struct random_context *ctx = SRG_CreateEntropy4( NULL, 0 );
			SRG_FeedEntropy( ctx, (uint8_t*)osoOpts.objectHash, osoOpts.objectHashLen );
			SRG_FeedEntropy( ctx, (uint8_t*)osoOpts.sealant, osoOpts.sealantLen );
			uint32_t buf[256 / 32];
			SRG_GetEntropyBuffer( ctx, buf, 256 );
			size_t outlen;
			storeId = EncodeBase64Ex( (uint8_t*)buf, (16 + 16), &outlen, (const char *)1 );
			SRG_DestroyEntropy( &ctx );
		}
		else {
			struct random_context *ctx = SRG_CreateEntropy4( NULL, 0 );
			SRG_FeedEntropy( ctx, (uint8_t*)osoOpts.objectHash, osoOpts.objectHashLen );
			uint32_t buf[256 / 32];
			SRG_GetEntropyBuffer( ctx, buf, 256 );
			size_t outlen;
			storeId = EncodeBase64Ex( (uint8_t*)buf, (16 + 16), &outlen, (const char *)1 );
			SRG_DestroyEntropy( &ctx );
		}
	}
	else {
		if( osoOpts.data ) {
			// no specfic has, this has to do a signing process...
			if( osoOpts.sealant ) {
				storeId = NULL;
				if( osoOpts.readKey )
					sack_vfs_os_ioctl_store_crypt_sealed_object( osoOpts.vol->fsMount
						, osoOpts.data, osoOpts.dataLen
						, osoOpts.sealant, osoOpts.sealantLen
						, osoOpts.readKey, osoOpts.readKeyLen
						, osoOpts.result, sizeof( osoOpts.result ) );
				else
					sack_vfs_os_ioctl_store_sealed_object( osoOpts.vol->fsMount
						, osoOpts.data, osoOpts.dataLen
						, osoOpts.sealant, osoOpts.sealantLen
						, osoOpts.result, sizeof( osoOpts.result ) );
			}
			else {
				sack_vfs_os_ioctl_store_rw_object( osoOpts.vol->fsMount
					, osoOpts.data, osoOpts.dataLen
					, osoOpts.result, sizeof( osoOpts.result ) );
			}
		}
	}
	return 0;
}

void ObjectStorageObject::flush( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	//Local<Function> cb;
	ObjectStorageObject* vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	// And unload?
	sack_vfs_os_flush_volume( vol->vol, FALSE );

}


void ObjectStorageObject::removeObject( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	//Local<Function> cb;
	ObjectStorageObject* vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	if( args[0]->IsString() ) {
		String::Utf8Value fName( isolate, args[0] );
		//lprintf( "OPEN FILE:%s", *fName );
		sack_vfs_os_unlink_file( vol->vol, *fName );// , fName.length() );
		//sack_vfs_os_flush_volume( vol->vol, FALSE );
		sack_vfs_os_polish_volume( vol->vol );
	}
	else if( args[0]->IsObject() ) {

	}

}

void ObjectStorageObject::putObject( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate *isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct optionStrings *strings = getStrings( isolate );
	String::Utf8Value data( USE_ISOLATE(isolate) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	Local<Object> opts = args[1]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
	Local<String> optName;

	struct objectStorageOptions *osoOpts = NULL;
	ThreadTo( DoPutObject, (uintptr_t)&osoOpts );
	while( !osoOpts )
		Relinquish();

	//memset( &osoOpts, 0, sizeof( osoOpts ) );
	osoOpts->vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );

	osoOpts->data = StrDup( *data );
	osoOpts->dataLen = data.length();

	if( opts->Has( context, optName = strings->objectHashString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value strval( isolate,  GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		osoOpts->objectHash = StrDup( *strval );
		osoOpts->objectHashLen = strval.length();
	}
	if( opts->Has( context, optName = strings->readKeyString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value strval( isolate, GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		osoOpts->readKey = StrDup( *strval );
		osoOpts->readKeyLen = strval.length();
	}

	if( opts->Has( context, optName = strings->sealantString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value strval( isolate,  GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		osoOpts->sealant = StrDup( *strval );
		osoOpts->sealantLen = strval.length();
	}

	if( opts->Has( context, optName = strings->storedString->Get( isolate ) ).ToChecked() ) {
		osoOpts->cbStored.Reset( isolate, GETV( opts, optName ).As<Function>() );
	}

	if( opts->Has( context, optName = strings->failedString->Get( isolate ) ).ToChecked() ) {
		osoOpts->cbFailed.Reset( isolate, GETV( opts, optName ).As<Function>() );
	}

	osoOpts->set = 1;

}


static uintptr_t CPROC DoGetObject( PTHREAD thread ) {
	struct objectStorageOptions osoOpts;
	struct objectStorageOptions **ppOptions = (struct objectStorageOptions **)GetThreadParam( thread );
	//char *storeId;
	(*ppOptions) = &osoOpts;
	while( !osoOpts.set )
		Relinquish();

	return 0;
}

void ObjectStorageObject::getObject( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate *isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct optionStrings *strings = getStrings( isolate );
	String::Utf8Value data( USE_ISOLATE(isolate) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	Local<Object> opts = args[1]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
	Local<String> optName;

	struct objectStorageOptions *osoOpts = NULL;
	ThreadTo( DoGetObject, (uintptr_t)osoOpts );
	while( !osoOpts )
		Relinquish();

	//memset( &osoOpts, 0, sizeof( osoOpts ) );
	osoOpts->vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );

	osoOpts->data = StrDup( *data );
	osoOpts->dataLen = data.length();

	if( opts->Has( context, optName = strings->objectHashString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value strval( isolate,  GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		osoOpts->objectHash = StrDup( *strval );
		osoOpts->objectHashLen = strval.length();
	}
	if( opts->Has( context, optName = strings->readKeyString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value strval( isolate,  GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		osoOpts->readKey = StrDup( *strval );
		osoOpts->readKeyLen = strval.length();
	}

	if( opts->Has( context, optName = strings->sealantString->Get( isolate ) ).ToChecked() ) {
		String::Utf8Value strval( isolate,  GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		osoOpts->sealant = StrDup( *strval );
		osoOpts->sealantLen = strval.length();
	}

	if( opts->Has( context, optName = strings->thenString->Get( isolate ) ).ToChecked() ) {
		osoOpts->cbStored.Reset( isolate, GETV( opts, optName ).As<Function>() );
	}

	if( opts->Has( context, optName = strings->catchString->Get( isolate ) ).ToChecked() ) {
		osoOpts->cbFailed.Reset( isolate, GETV( opts, optName ).As<Function>() );
	}

	osoOpts->set = 1;

}




void ObjectStorageObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		char *mount_name;
		char *filename = (char*)"default.os";
		LOGICAL defaultFilename = TRUE;
		char *key = NULL;
		char *key2 = NULL;
		uintptr_t version = 0;
		VolumeObject* vol = NULL;
		int arg = 0;
		int argc = args.Length();
		if( argc > 0 && args[0]->IsObject() ) {
			vol = ObjectWrap::Unwrap<VolumeObject>( args[0].As<Object>() );
			arg++;
		}
		if( argc == arg ) {
			ObjectStorageObject* obj = new ObjectStorageObject( NULL, NULL, 0, NULL, NULL, NULL );
			obj->Wrap( args.This() );
			args.GetReturnValue().Set( args.This() );
		}
		else {
			//int arg = 0;
			if( args[arg]->IsString() ) {

				//TooObject( isolate.getCurrentContext().FromMaybe( Local<Object>() )
				String::Utf8Value fName( isolate,  args[arg++]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
				mount_name = StrDup( *fName );
			}
			else {
				if( argc > arg )
					arg++; // assume null mount name
				mount_name = SRG_ID_Generator();
			}
			if( argc > (arg) ) {
				if( args[arg]->IsString() ) {
					String::Utf8Value fName( isolate,  args[arg++]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
					defaultFilename = FALSE;
					filename = StrDup( *fName );
				}
				else
					filename = NULL;
			}
			else {
				defaultFilename = FALSE;
				{
					char* sep;
					if( sep = strchr( mount_name, '@' ) ) {
						lprintf( "find mount name to get volume..." );
					}
				}
				filename = mount_name;
				mount_name = SRG_ID_Generator();
			}
			//if( args[argc
			if( args[arg]->IsNumber() ) {
				version = (uintptr_t)args[arg++]->ToNumber( isolate->GetCurrentContext() ).ToLocalChecked()->Value();
			}
			if( argc > arg ) {
				if( !args[arg]->IsNull() && !args[arg]->IsUndefined() ) {
					String::Utf8Value k( isolate,  args[arg] );
					key = StrDup( *k );
				}
				arg++;
			}
			if( argc > arg ) {
				if( !args[arg]->IsNull() && !args[arg]->IsUndefined() ) {
					String::Utf8Value k( isolate,  args[arg] );
					key2 = StrDup( *k );
				}
				arg++;
			}
			// Invoked as constructor: `new MyObject(...)`
			ObjectStorageObject* obj = new ObjectStorageObject( mount_name, filename, version, key, key2, vol?vol->fsMount:NULL );
			if( !obj->vol ) {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "ObjectStorage failed to open." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
			}
			else {
				obj->Wrap( args.This() );
				args.GetReturnValue().Set( args.This() );
			}
			//Deallocate( char*, mount_name );
			if( !defaultFilename )
				Deallocate( char*, filename );
			Deallocate( char*, key );
			Deallocate( char*, key2 );
		}
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->ObjectStorageObject_constructor );
		MaybeLocal<Object> mo = cons->NewInstance( isolate->GetCurrentContext(), argc, argv );
		if( !mo.IsEmpty() )
			args.GetReturnValue().Set( mo.ToLocalChecked() );
		delete[] argv;
	}
}

void ObjectStorageObject::createIndex( const v8::FunctionCallbackInfo<Value>& args ) {

	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, TranslateText( "Requires filename to open and key to define" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}

	ObjectStorageObject *vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	lprintf( "And here we get to actually " );


	String::Utf8Value fName( isolate,  args[0] );
	struct objStore::sack_vfs_os_file *file = objStore::sack_vfs_os_openfile( vol->vol, (*fName) );
	if( file ) {
		Local<Object> indexDef = args[1]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
		struct optionStrings *strings = getStrings( isolate );
		Local<String> optName;

		Local<Object> theArray = GETV( indexDef, optName = strings->dataString->Get(isolate) ).As<Object>();
		Local<String> name = GETV( indexDef, optName = strings->nameString->Get( isolate ) ).As<String>();
		Local<Object> opts = GETV( indexDef, optName = strings->optsString->Get( isolate ) ).As<Object>();

		String::Utf8Value fieldName( isolate,  name );
		objStore::sack_vfs_os_file_ioctl( file, SOSFSFIO_CREATE_INDEX, *fieldName, fieldName.length() );
		//objStore::sack_vfs_os_write( file, *data, data.length() );
		objStore::sack_vfs_os_close( file );
		//sack_vfs_os_polish_volume( vol->vol );
	}
}

#ifdef LOG_DISK_TIME
static uint64_t writeTotal = 0;
static uint64_t otherTotal = 0;
static uint64_t lastTick;
#endif

void ObjectStorageObject::fileWrite( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, TranslateText( "Requires filename to open and data to write" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Function> cb;
	ObjectStorageObject *vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	String::Utf8Value fName( isolate,  args[0] );
#ifdef LOG_DISK_TIME
	{
		uint64_t sTick = GetTickCount64();
		if( lastTick )
			otherTotal += (sTick - lastTick);
		lastTick = sTick;
	}
	//lprintf( "OPEN FILE:%s %lld %lld", *fName, writeTotal, otherTotal );
#endif
	int arg = 2;
	while( arg < args.Length() ) {
		if( args[arg]->IsFunction() ) {
			cb = Local<Function>::Cast( args[arg] );
			arg++;
		}
		else
			arg++;
	}

	if( vol->volNative ) {
		struct objStore::sack_vfs_os_file *file = objStore::sack_vfs_os_openfile( vol->vol, (*fName) );
		// is  binary thing... or is a string buffer... or...

		if( file ) {
			String::Utf8Value data( isolate,  args[1] );
#ifdef DEBUG_LOG_OUTPUT
			char* tmp;
			lprintf( "Write %d to %s\nWrite Data %s", data.length(), ( *fName ), tmp = jsox_escape_string_length( *data, data.length(), NULL ) );
			Release( tmp );
#endif
			objStore::sack_vfs_os_truncate( file ); // allow new content to allocate in large blocks?
			objStore::sack_vfs_os_write( file, *data, data.length() );
			objStore::sack_vfs_os_close( file );
			sack_vfs_os_polish_volume( vol->vol );
			//sack_vfs_os_flush_volume( vol->vol, FALSE );
			if( !cb.IsEmpty() ) {
				cb->Call( isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 0, NULL );
			}

		}
	} else {
		lprintf( "Write to native volume not supported?" );
	}
#ifdef LOG_DISK_TIME
	{
		uint64_t thisTick = GetTickCount64();
		writeTotal += (thisTick - lastTick);
		lastTick = thisTick;
	}
#endif
}


void ObjectStorageObject::fileRead( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	ObjectStorageObject* vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );

	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, TranslateText( "Requires filename to open and data callback" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Function> cb;
	String::Utf8Value fName( isolate, args[0] );
	int arg = 1;
	while( arg < args.Length() ) {
		if( args[arg]->IsFunction() ) {
			cb = Local<Function>::Cast( args[arg] );
			arg++;
		}
		else if( args[arg]->IsObject() ) {
			arg++;
		}
	}

	if( vol->volNative ) {
		if( !objStore::sack_vfs_os_exists( vol->vol, ( *fName ) ) ) {
			if( !cb.IsEmpty() ) {
				cb->Call( isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 0, NULL );
			}
			return;
		}

		//lprintf( "OPEN FILE:%s", *fName );
		struct objStore::sack_vfs_os_file* file = objStore::sack_vfs_os_openfile( vol->vol, ( *fName ) );
		if( file ) {
			size_t len = objStore::sack_vfs_os_size( file );
			size_t read = 0;
			size_t newRead;
			char* buf = NewArray( char, len );

			// CAN open directories; and they have 7ffffffff sizes.
			while( ( read < len ) && ( newRead = objStore::sack_vfs_os_read( file, buf, len ) ) ) {
				read += newRead;
			}

			objStore::sack_vfs_os_close( file );
			if( !cb.IsEmpty() ) {
#if ( NODE_MAJOR_VERSION >= 14 )
				std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( buf, len, releaseBufferBackingStore, NULL );
				Local<Object> arrayBuffer = ArrayBuffer::New( isolate, bs );
#else
				Local<Object> arrayBuffer = ArrayBuffer::New( isolate, buf, len );
				PARRAY_BUFFER_HOLDER holder = GetHolder();
				holder->o.Reset( isolate, arrayBuffer );
				holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
				holder->buffer = buf;
#endif
				Local<Value> args[1];
				args[0] = arrayBuffer;
				cb->Call( isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, args );
			}

		}

	}
	else {
		FILE* file = sack_fopenEx( 0, ( *fName ), "rb", vol->fsMount );
		if( file ) {
			size_t len = sack_fsize( file );
			char* buf = NewArray( char, len );
			size_t read = 0;
			size_t newRead;
			// CAN open directories; and they have 7ffffffff sizes.
			while( ( read < len ) && ( newRead = sack_fread( buf, len, 1, file ) ) ) {
				read += newRead;
			}
			sack_fclose( file );

			if( !cb.IsEmpty() ) {

#if ( NODE_MAJOR_VERSION >= 14 )
				std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( buf, len, releaseBufferBackingStore, NULL );
				Local<Object> arrayBuffer = ArrayBuffer::New( isolate, bs );
#else
				Local<Object> arrayBuffer = ArrayBuffer::New( isolate, buf, len );
				PARRAY_BUFFER_HOLDER holder = GetHolder();
				holder->o.Reset( isolate, arrayBuffer );
				holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
				holder->buffer = buf;
#endif

				Local<Value> args[1];
				args[0] = arrayBuffer;
				cb->Call( isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, args );
			}
		}
	}



}

void ObjectStorageObject::fileReadJSOX( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	ObjectStorageObject *vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	struct jsox_parse_state *parser = NULL;
	LOGICAL tempParser = FALSE;
	JSOXObject *parserObject = NULL;

	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, TranslateText( "Requires filename to open and data callback" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Function> cb;
	String::Utf8Value fName( isolate,  args[0] );
	int arg = 1;
	while( arg < args.Length() ) {
		if( args[arg]->IsFunction() ) {
			cb = Local<Function>::Cast( args[arg] );
			arg++;
		}
		else if( args[arg]->IsObject() ) {
			Local<Object> useParser = args[arg]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
			parserObject = ObjectWrap::Unwrap<JSOXObject>( useParser );
			parser = parserObject->state;
			arg++;
		}
	}

	if( !parser ) {
		parser = jsox_begin_parse();
		tempParser = TRUE;
	}

	if( vol->volNative ) {
		if( !objStore::sack_vfs_os_exists( vol->vol, ( *fName ) ) ) {
			//lprintf( "object is not found: %s", *fName );
			if( !cb.IsEmpty() ) {
				cb->Call( isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 0, NULL );
			}
			return;
		}

		//lprintf( "OPEN FILE:%s", *fName );
		struct objStore::sack_vfs_os_file *file = objStore::sack_vfs_os_openfile( vol->vol, (*fName) );
		if( file ) {
			char *buf = NewArray( char, 4096 );
			size_t len = objStore::sack_vfs_os_size( file );
			size_t read = 0;
			size_t newRead;
			size_t timeCount;
			uint64_t *timeArray;

			objStore::sack_vfs_os_get_times( file, &timeArray, &timeCount );
			Local<Array> arr = Array::New( isolate, (int)timeCount );
			for( int n = 0; n < timeCount; n++ ) {
				//arr->Set( n, Date::New( isolate, timeArray[n] / 1000000000.0 ) );
				{
					SACK_TIME st;
					ConvertTickToTime( timeArray[n], &st );
					Local<Script> script;
					char buf[64];
					int tz;
					int negTz = 0;
					if( st.zhr < 0 ) {
						tz = -st.zhr;
						negTz = 1;
					}
					else
						tz = st.zhr;

					snprintf( buf, 64, "new Date('%04d-%02d-%02dT%02d:%02d:%02d.%03d%c%02d:%02d')", st.yr, st.mo, st.dy, st.hr, st.mn, st.sc, st.ms, negTz?'-':'+', tz, st.zmn );
					script = Script::Compile( isolate->GetCurrentContext()
						, String::NewFromUtf8( isolate, buf, NewStringType::kNormal ).ToLocalChecked()
						, new ScriptOrigin( String::NewFromUtf8( isolate, "DateFormatter"
							, NewStringType::kInternalized ).ToLocalChecked() ) ).ToLocalChecked();
					arr->Set( isolate->GetCurrentContext(), n, script->Run( isolate->GetCurrentContext() ).ToLocalChecked() );
				}

			}
			// CAN open directories; and they have 7ffffffff sizes.
			while( (read < len) && (newRead = objStore::sack_vfs_os_read( file, buf, 4096 )) ) {
				read += newRead;
				int result;
#ifdef DEBUG_LOG_PARSING
				lprintf( "Parse file: %.*s", newRead, buf );
#endif
				for( (result = jsox_parse_add_data( parser, buf, newRead ));
					result > 0;
					result = jsox_parse_add_data( parser, NULL, 0 ) ) {
					//Local<Object> obj = Object::New( isolate );
					PDATALIST data;
					data = jsox_parse_get_data( parser );
					struct reviver_data r,*r_;
					r.revive = FALSE;
					r._this = args.This();
					r.isolate = isolate;
					r.context = isolate->GetCurrentContext();
					r.parser = parserObject;
					r.failed = FALSE;
					r.reviveStack = NULL;
					r_ = parserObject->currentReviver;
					parserObject->currentReviver = &r;
					Local<Value> val = convertMessageToJS2( data, &r );
					{
						Local<Value> argv[2] = { val, arr };
						MaybeLocal<Value> result = cb->Call( r.context, isolate->GetCurrentContext()->Global(), 2, argv );
						if( result.IsEmpty() ) { // if an exception occurred stop, and return it.
							parserObject->currentReviver = r_;
							jsox_dispose_message( &data );
							jsox_parse_dispose_state( &parser );
							return;
						}
					}
					parserObject->currentReviver = r_;
					jsox_dispose_message( &data );
					if( result == 1 )
						break;
				}
				if( result < 0 ) {
					PTEXT error = jsox_parse_get_error( parser );
					if( error )
						isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, GetText( error ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
					else
						isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "No Error Text" STRSYM(__LINE__), v8::NewStringType::kNormal ).ToLocalChecked() ) );
					LineRelease( error );
				}
			}
			Deallocate( char *, buf );
			objStore::sack_vfs_os_close( file );
		}

	}
	else {
		FILE *file = sack_fopenEx( 0, (*fName), "rb", vol->fsMount );
		if( file ) {
			char *buf = NewArray( char, 4096 );
			size_t len = sack_fsize( file );
			size_t read = 0;
			size_t newRead;
			struct jsox_parse_state *parser = jsox_begin_parse();
			// CAN open directories; and they have 7ffffffff sizes.
			while( (read < len) && (newRead = sack_fread( buf, 4096, 1, file )) ) {
				read += newRead;
				int result;
#ifdef DEBUG_LOG_PARSING
				lprintf( "Parse file: %.*s", newRead, buf );
#endif
				for( (result = jsox_parse_add_data( parser, buf, newRead ));
					result > 0;
					result = jsox_parse_add_data( parser, NULL, 0 ) ) {
					PDATALIST data;
					data = jsox_parse_get_data( parser );
					if( data->Cnt ) {
						struct reviver_data r, *r_;
						r.revive = FALSE;
						r.isolate = isolate;
						r.context = isolate->GetCurrentContext();
						r.failed = FALSE;
						r_ = parserObject->currentReviver;
						parserObject->currentReviver = &r;
						Local<Value> val = convertMessageToJS2( data, &r );
						{
							MaybeLocal<Value> result = cb->Call( r.context, isolate->GetCurrentContext()->Global(), 1, &val );
							if( result.IsEmpty() ) { // if an exception occurred stop, and return it.
								parserObject->currentReviver = r_;
								jsox_dispose_message( &data );
								jsox_parse_dispose_state( &parser );
								return;
							}
						}
						parserObject->currentReviver = r_;
					}
					jsox_dispose_message( &data );
					if( result == 1 )
						break;
				}
			}
			Deallocate( char *, buf );
			sack_fclose( file );
		}
	}

	if( tempParser )
		jsox_parse_dispose_state( &parser );


}

ObjectStorageObject*  openInVFS( Isolate *isolate, const char *mount, const char *name, const char *key1, const char *key2, struct file_system_mounted_interface* useMount ) {

//ObjectStorageObject*  ObjectStorageObject::openInVFS( Isolate *isolate, const char *mount, const char *name, const char *key1, const char *key2 ) {

	// Invoked as constructor: `new MyObject(...)`
	ObjectStorageObject* obj = new ObjectStorageObject( mount, name, 0, key1, key2, useMount );
	if( !obj->vol ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "ObjectStorage failed to open." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		delete obj;
		obj = NULL;
	}
	return obj;

}

void ObjectStorageInit( Isolate *isolate, Local<Object> exports ) {
	ObjectStorageObject::Init( isolate, exports );
}
