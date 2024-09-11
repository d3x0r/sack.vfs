#include "global.h"
using namespace sack::SACK_VFS::objStore;

//#define LOG_DISK_TIME

// these use 'printf()' now, because lprintf has safety buffers limited at 4096
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
	ObjectStorageObject( const char *mount, const char *filename, uintptr_t version, const char *key, const char *key2, struct file_system_mounted_interface* useMount, int flags );

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

	static void getTimeline( const v8::FunctionCallbackInfo<Value>& args );
	static void haltVolume( const v8::FunctionCallbackInfo<Value>& args );
	
	// utility to remove the key so it can be diagnosed.
	static void volDecrypt( const v8::FunctionCallbackInfo<Value>& args );

	static void fileReadJSOX( const v8::FunctionCallbackInfo<Value>& args );
	static void fileGetTimes( const v8::FunctionCallbackInfo<Value>& args );
	static void fileSetTime( const v8::FunctionCallbackInfo<Value>& args );
	static void getTimelineCursor( const v8::FunctionCallbackInfo<Value>& args );
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

class TimelineCursorObject : public node::ObjectWrap {
public:
	class ObjectStorageObject* oso;
	struct sack_vfs_os_time_cursor* cursor;
public:
	TimelineCursorObject( class ObjectStorageObject* vol ) {
		this->oso = vol;
		cursor = sack_vfs_os_get_time_cursor( this->oso->vol );
	}
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void read( const v8::FunctionCallbackInfo<Value>& args );

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
	Eternal<String>* idString;
	Eternal<String>* versionString;
	Eternal<String>* timeString;
	Eternal<String>* readString;
	Eternal<String>* limitString;
	Eternal<String>* fromString;
	Eternal<String>* getTimezoneOffsetString;
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
		DEFSTRING( id );
		DEFSTRING( version );
		DEFSTRING( time );
		DEFSTRING( from );
		DEFSTRING( limit );
		DEFSTRING( read );
		DEFSTRING( opts );
		DEFSTRING( getTimezoneOffset );
	}
	return check;
}

#if 0
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
#endif

#if 0
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
#endif

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
			//isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Failed to find target accepting thread" ) ) );
		}
	}
	return TRUE;
}

static void postObjectStorage( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Required parameter missing: (unique,socket)" ) ) );
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
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Second paramter is not an accepted socket" ) ) );
	}
}


/*
static void postObjectStorageObject( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 1 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Required parameter missing: (unique)" ) ) );
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
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Object is not an accepted socket" ) ) );
	}
}
*/

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
	trans->oso->volNative = false;
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

	Local<FunctionTemplate> clsTemplate_timelineCursor;
	clsTemplate_timelineCursor = FunctionTemplate::New( isolate, TimelineCursorObject::New );
	clsTemplate_timelineCursor->SetClassName( String::NewFromUtf8Literal( isolate, "sack.ObjectStorage.TimelineCursor" ) );
	clsTemplate_timelineCursor->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	NODE_SET_PROTOTYPE_METHOD( clsTemplate_timelineCursor, "get", TimelineCursorObject::read );


	Local<FunctionTemplate> clsTemplate;
	clsTemplate = FunctionTemplate::New( isolate, New );
	clsTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.ObjectStorage" ) );
	clsTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "read", ObjectStorageObject::fileReadJSOX );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "getTimes", ObjectStorageObject::fileGetTimes );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "setTime", ObjectStorageObject::fileSetTime );

	clsTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "timeline" )
		, FunctionTemplate::New( isolate, ObjectStorageObject::getTimeline )
		, Local<FunctionTemplate>()
	);
	//NODE_SET_PROTOTYPE_METHOD( clsTemplate, "timeline", ObjectStorageObject::getTimelineCursor );

	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "halt", ObjectStorageObject::haltVolume );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "readRaw", ObjectStorageObject::fileRead );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "writeRaw", ObjectStorageObject::fileWrite );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "createIndex", ObjectStorageObject::createIndex );
	//NODE_SET_PROTOTYPE_METHOD( clsTemplate, "store", ObjectStorageObject::fileStore );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "put", ObjectStorageObject::putObject );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "get", ObjectStorageObject::getObject );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "delete", ObjectStorageObject::removeObject );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "flush", ObjectStorageObject::flush );

	//Local<Function> VolFunc = clsTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();

	class constructorSet* c = getConstructors( isolate );
	Local<Function> objectStoreFunc = clsTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
	Local<Function> timelineCursorFunc = clsTemplate_timelineCursor->GetFunction( isolate->GetCurrentContext() ).ToLocalChecked();
	c->ObjectStorageObject_constructor.Reset( isolate, objectStoreFunc );
	c->TimelineCursorObject_constructor.Reset( isolate, timelineCursorFunc );

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
	, const char *key2, struct file_system_mounted_interface*useMount, int flags ) {
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

		vol = objStore::sack_vfs_os_load_volume_v2( flags, filename, version, key, key2, useMount );
		//vol = objStore::sack_vfs_os_load_crypt_volume( filename, version, key, key2, useMount );

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
		if( storeId ) {
			lprintf( "Sealed ID is not implemented fully..." );
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
				if( osoOpts.readKey ) {
					lprintf( "Readkey is set on write?" );

					sack_vfs_os_ioctl_store_crypt_owned_object( osoOpts.vol->fsMount
						, osoOpts.data, osoOpts.dataLen
						, NULL, 0 
						, osoOpts.readKey, osoOpts.readKeyLen
						, osoOpts.result, sizeof( osoOpts.result ) );
				} else
					sack_vfs_os_ioctl_store_rw_object( osoOpts.vol->fsMount
						, osoOpts.data, osoOpts.dataLen
						, osoOpts.result, sizeof( osoOpts.result ) );
			}
		}
	}
	return 0;
}

void ObjectStorageObject::flush( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	//Local<Context> context = isolate->GetCurrentContext();
	//Local<Function> cb;
	ObjectStorageObject* vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	// And unload?
	sack_vfs_os_flush_volume( vol->vol, FALSE );

}


void ObjectStorageObject::removeObject( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	//Local<Context> context = isolate->GetCurrentContext();
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

void TimelineCursorObject::read( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct optionStrings* strings = getStrings( isolate );
	Local<String> optName;
	Local<Object> options = args[0].As<Object>();// ToObject( isolate->GetCurrentContext() ).ToLocalChecked();

	int32_t limit = GETV( options, optName = strings->limitString->Get( isolate ) )->Int32Value(context).ToChecked();
	bool doRead = GETV( options, optName = strings->readString->Get( isolate ) )->BooleanValue(isolate);
	Local<Value> from;
	optName = strings->fromString->Get( isolate );
	if( !(options->Has( context, optName ).ToChecked()) ) {
		from.Clear();
	}else
		from = GETV( options, optName ).As<Value>();
	TimelineCursorObject* tlc = ObjectWrap::Unwrap<TimelineCursorObject>( args.Holder() );
	Local<Array> arr = Array::New( isolate, 0 );
	const char* buffer;
	size_t length;
	uint64_t time;
	uint64_t entry;
	int8_t tz;
	const char* filename;
	LOGICAL result;
	int got = 0;
	class constructorSet* c = getConstructors( isolate );
	do {
		if( got == 0 ) {
			if( !from.IsEmpty() ) {
				if( from->IsDate() ) {
					Local<Date> fd = from.As<Date>();
					result = sack_vfs_os_read_time_cursor( tlc->cursor, 0, (uint64_t)( fd->ValueOf() * 1000.0 ) * 1000, &entry, &filename, &time, &tz, doRead ? &buffer : NULL, &length );
				} else if( from->IsNumber() ) {
					Local<Number> fd = from.As<Number>();
					result = sack_vfs_os_read_time_cursor( tlc->cursor, 1, (int)fd->Value(), &entry, &filename, &time, &tz, doRead ? &buffer : NULL, &length );
				} else if( from->InstanceOf( context, c->dateNsCons.Get( isolate ) ).ToChecked() ) {
					Local<Date> fd = from.As<Date>();
					//Local<Value> ns = fd->Get( context, String::NewFromUtf8Literal( isolate, "ns" ) ).ToLocalChecked();

					result = sack_vfs_os_read_time_cursor( tlc->cursor, 0, (uint64_t)( fd->ValueOf() * 1000.0 ) * 1000, &entry, &filename, &time, &tz, doRead ? &buffer : NULL, &length );

				} else {
					lprintf( "Unhandled from argument..." );
					result = FALSE;
				}
			} else {
				result = sack_vfs_os_read_time_cursor( tlc->cursor, 2, 0, &entry, &filename, &time, &tz, doRead ? &buffer : NULL, &length );
			}
		} else {
			result = sack_vfs_os_read_time_cursor( tlc->cursor, 2, 0, &entry, &filename, &time, &tz, doRead ? &buffer : NULL, &length );
		}
		if( result ) {

			SACK_TIME st;
			// time is stored as UTC so all times are universal and have no bias between them.
			// though decoding a timestamp with a timezone requires the local time to be used along with the timezone code
			// so this has to adjust the value before decoding to parts and building a resulting string.
			uint64_t timeValue = ((time / 1000000 + ( tz * 900000LL ) ) << 8) | (tz&0xFF);
			ConvertTickToTime( timeValue, &st );
			char buf[64];
			int tz;
			int negTz = 0;
			if( st.zhr < 0 ) {
				tz = -st.zhr;
				negTz = 1;
			} else
				tz = st.zhr;

			snprintf( buf, 64, "%04d-%02d-%02dT%02d:%02d:%02d.%03d%c%02d:%02d", st.yr, st.mo, st.dy, st.hr, st.mn, st.sc, st.ms, negTz ? '-' : '+', tz, st.zmn );
			Local<Value> args[2] = { String::NewFromUtf8( isolate, buf, NewStringType::kNormal ).ToLocalChecked(), Number::New( isolate, (double)(time%1000000) ) };
			Local<Value> newDate;
			if( time % 1000000 )
				newDate = Local<Function>::New( isolate, c->dateNsCons )->NewInstance( isolate->GetCurrentContext(), 2, args ).ToLocalChecked();
			else
				newDate = Local<Function>::New( isolate, c->dateCons )->NewInstance( isolate->GetCurrentContext(), 1, args ).ToLocalChecked();
			Local<Object> obj = Object::New( isolate );
			obj->Set( context, String::NewFromUtf8Literal( isolate, "time" ), newDate );
			obj->Set( context, String::NewFromUtf8Literal( isolate, "entry" ), Number::New( isolate, (double)entry ) );

			obj->Set( context, String::NewFromUtf8Literal( isolate, "id" ), String::NewFromUtf8( isolate, filename, NewStringType::kNormal ).ToLocalChecked() );
			obj->Set( context, String::NewFromUtf8Literal( isolate, "length" ), Number::New( isolate, (double)length ) );
			if( doRead ) {
				Local<ArrayBuffer> ab;
				POINTER newBuf = NewArray( uint8_t, length );
				MemCpy( newBuf, buffer, length );
#if ( NODE_MAJOR_VERSION >= 14 )
				std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( (POINTER)newBuf, length, releaseBufferBackingStore, NULL );
				ab = ArrayBuffer::New( isolate, bs );
#else
				ab =
					ArrayBuffer::New( isolate
						, (void*)newBuf
						, length );

				PARRAY_BUFFER_HOLDER holder = GetHolder();
				holder->o.Reset( isolate, ab );
				holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
				holder->buffer = newBuf;
#endif
				obj->Set( context, String::NewFromUtf8Literal( isolate, "data" ), ab );

			}
			arr->Set( isolate->GetCurrentContext(), got, obj );
			Deallocate( const char*, filename );
		} else break;
		got++;
	} while( got < limit );
	args.GetReturnValue().Set( arr );

}

void TimelineCursorObject::New( const v8::FunctionCallbackInfo<Value>& args ) {

	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		Local<Object> storage = args[0].As<Object>();
		ObjectStorageObject* vol = ObjectWrap::Unwrap<ObjectStorageObject>( storage );
		TimelineCursorObject* obj = new TimelineCursorObject( vol );

		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );

	} else {
		int argc = args.Length();
		Local<Value>* argv = new Local<Value>[argc];
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


void ObjectStorageObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		char *mount_name = NULL;
		char *filename = (char*)"default.os";
		LOGICAL defaultFilename = TRUE;
		char *key = NULL;
		char *key2 = NULL;
		uintptr_t version = 0;
		VolumeObject* vol = NULL;
		ObjectStorageObject* oso = NULL;
		int arg = 0;
		int argc = args.Length();
		if( argc > 0 && args[0]->IsObject() ) {
			class constructorSet* c = getConstructors( isolate );
			if( args[0]->InstanceOf( isolate->GetCurrentContext(), c->volConstructor.Get( isolate ) ).ToChecked() ) {
				vol = ObjectWrap::Unwrap<VolumeObject>( args[0].As<Object>() );
				arg++;
			}
			else if( args[0]->InstanceOf( isolate->GetCurrentContext(), c->ObjectStorageObject_constructor.Get( isolate ) ).ToChecked() ) {
				oso = ObjectWrap::Unwrap<ObjectStorageObject>( args[0].As<Object>() );
				//vol = oso->vol;
				arg++;
			}
		}
		if( argc == arg ) {
			ObjectStorageObject* obj = new ObjectStorageObject( NULL, NULL, 0, NULL, NULL, NULL, 0 );
			obj->Wrap( args.This() );
			args.GetReturnValue().Set( args.This() );
		}
		else {
			int readonly = 0;
			int namearg = 0;
			//int arg = 0;
			if( args[arg]->IsBoolean() ) {
				readonly = args[arg]->BooleanValue(isolate);
				arg++;
			}
			if( (arg < argc) && args[arg]->IsString() ) {
				//TooObject( isolate.getCurrentContext().FromMaybe( Local<Object>() )
				String::Utf8Value fName( isolate,  args[arg++]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
				if( !vol && !oso && ( argc > (arg) )  )
					mount_name = StrDup( *fName );
				else {
					filename = StrDup( *fName );
				}
				arg++;
			}
			else {
				if( argc > arg ) {
					arg++; // assume null mount name
					namearg++;
				}
				mount_name = NULL;// SRG_ID_Generator();
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
			else if( namearg==0 ){
				char* sep;
				if( filename && (sep = strchr( filename, '@' )) ) {
					lprintf( "find mount name to get volume..." );
					mount_name = filename;
					sep[0] = 0;
					filename = sep + 1;
				}
				else {
					defaultFilename = FALSE;
					//filename = mount_name;
					mount_name = SRG_ID_Generator();
				}
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
			ObjectStorageObject* obj = new ObjectStorageObject( mount_name, filename, version, key, key2
					, vol
							?vol->fsMount
							:mount_name
								? sack_get_mounted_filesystem(mount_name)
								:NULL, readonly?SACK_VFS_LOAD_FLAG_NO_REPLAY:0 );
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
			else if( mount_name ) Deallocate( char*, mount_name );
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

		//Local<Object> theArray = GETV( indexDef, optName = strings->dataString->Get(isolate) ).As<Object>();
		Local<String> name = GETV( indexDef, optName = strings->nameString->Get( isolate ) ).As<String>();
		//Local<Object> opts = GETV( indexDef, optName = strings->optsString->Get( isolate ) ).As<Object>();

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
	Local<Object> opts;
	Local<Date> useTime;
	uint64_t dateValToUse = 0;
	int tzToUse = 0;
	bool version = false;
	ObjectStorageObject *vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	Local<Value> nameArg = args[0];
	if( args[0]->IsObject() ) {
		opts = args[0].As<Object>();
		Local<Context> ctx = isolate->GetCurrentContext();
		struct optionStrings* strings = getStrings( isolate );
		Local<String> name;
		Local<Value> versionVal;
		if( opts->Has( ctx, name = strings->versionString->Get( isolate ) ).ToChecked() ) {
			versionVal = opts->Get( ctx, name ).ToLocalChecked();
			version = versionVal->BooleanValue(isolate);
		}
		if( opts->Has( ctx, name = strings->idString->Get( isolate ) ).ToChecked() ) {
			nameArg = opts->Get( ctx, name ).ToLocalChecked();
		}
		if(1)
		if( opts->Has( ctx, name = strings->timeString->Get( isolate ) ).ToChecked() ) {
			useTime = opts->Get( ctx, name ).ToLocalChecked().As<Date>();
			Local<Value> offset = useTime->Get( ctx, strings->getTimezoneOffsetString->Get( isolate ) ).ToLocalChecked().As<Function>()->Call( ctx, useTime, 0, NULL ).ToLocalChecked();
			int64_t intVal = offset.As<Number>()->IntegerValue(ctx).ToChecked();
			double dateVal = useTime->ValueOf();
			//dateVal -= intVal * 900;
			tzToUse = (int)-intVal;
			dateValToUse = ( ((uint64_t)( dateVal )<<8)| ( (tzToUse/15)&0xFF) );
			//lprintf( " COnverted time val: %g, %d", dateVal, (int)intVal );
		}
	}


	String::Utf8Value fName( isolate, nameArg );
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
		} else if( args[arg]->IsObject() ) {
			if( opts.IsEmpty() )
				opts = args[arg].As<Object>();
			arg++;
		} else {
			arg++;
		}
	}

	if( !opts.IsEmpty() ) {

	}

	if( vol->volNative ) {
		struct objStore::sack_vfs_os_file *file = version
			?(struct objStore::sack_vfs_os_file *)sack_vfs_os_system_ioctl( vol->vol, SOSFSSIO_NEW_VERSION, (*fName) )
			:objStore::sack_vfs_os_openfile( vol->vol, (*fName) );
		// is  binary thing... or is a string buffer... or...

		if( file ) {
			String::Utf8Value data( isolate,  args[1] );
#ifdef DEBUG_LOG_OUTPUT
			char* tmp;
			printf( "Write %d to %s\nWrite Data %s\n", data.length(), ( *fName ), tmp = jsox_escape_string_length( *data, data.length(), NULL ) );
			Release( tmp );
#endif
			objStore::sack_vfs_os_truncate( file ); // allow new content to allocate in large blocks?

			//size_t lengthWritten = 
			objStore::sack_vfs_os_write( file, *data, data.length() );
			// compare lengthWritten with data.length() ?

			if( dateValToUse )
				objStore::sack_vfs_os_set_time( file, dateValToUse, tzToUse );
			uint64_t lastTime = sack_vfs_os_ioctl_get_time( file );
			Local<Number> lastTimeNum = Number::New( isolate, (double)lastTime );
			objStore::sack_vfs_os_close( file );
			sack_vfs_os_polish_volume( vol->vol );
			args.GetReturnValue().Set( lastTimeNum );

			//sack_vfs_os_flush_volume( vol->vol, FALSE );
			if( !cb.IsEmpty() ) {
				Local<Value> args[1] = {lastTimeNum};
				cb->Call( isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, args );
			}

		}
	} else {
		lprintf( "Write to mounted volume not supported?" );
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

void ObjectStorageObject::getTimeline( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	//ObjectStorageObject* obj = ObjectWrap::Unwrap<ObjectStorageObject>( args.This() );
	class constructorSet* c = getConstructors( isolate );
	Local<Value> tl_args[] = { args.This() };
	Local<Value> timeline = c->TimelineCursorObject_constructor.Get( isolate )->CallAsConstructor( isolate->GetCurrentContext(), 1, tl_args ).ToLocalChecked();


	args.GetReturnValue().Set( timeline );
}


void ObjectStorageObject::getTimelineCursor( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	
}

void ObjectStorageObject::haltVolume( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	ObjectStorageObject* vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	sack_vfs_os_halt( vol->vol );
}

void ObjectStorageObject::fileSetTime( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, TranslateText( "Requires filename to and new time to use" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	ObjectStorageObject* vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	String::Utf8Value fName( isolate, args[0] );

	Local<Date> useTime;
	uint64_t dateValToUse = 0;
	int tzToUse = 0;
	struct optionStrings* strings = getStrings( isolate );

	useTime = args[1].As<Date>();
	if( !useTime.IsEmpty() ) {
		Local<Value> offset = useTime->Get( isolate->GetCurrentContext(), strings->getTimezoneOffsetString->Get( isolate ) ).ToLocalChecked().As<Function>()->Call( isolate->GetCurrentContext(), useTime, 0, NULL ).ToLocalChecked();
		int64_t intVal = offset.As<Number>()->IntegerValue( isolate->GetCurrentContext() ).ToChecked();
		double dateVal = useTime->ValueOf();
		//dateVal -= intVal * 900;
		tzToUse = (int)-intVal;
		dateValToUse = (uint64_t)(dateVal * 1000000);

		struct objStore::sack_vfs_os_file* file = objStore::sack_vfs_os_openfile( vol->vol, ( *fName ) );
		objStore::sack_vfs_os_set_time( file, dateValToUse, tzToUse );
		objStore::sack_vfs_os_close( file );
		//lprintf( " COnverted time val: %g, %d", dateVal, (int)intVal );
	}

}

Local<Array> makeTimes( Isolate* isolate, uint64_t* timeArray, int8_t* tzArray, size_t timeCount ) {
	Local<Array> arr = Array::New( isolate, (int)timeCount );
	class constructorSet* c = getConstructors( isolate );
	for( int n = 0; n < timeCount; n++ ) {
		SACK_TIME st;
		int8_t use_tz = ( (int8_t)tzArray[n] );
		// time is stored as UTC so all times are universal and have no bias between them.
		// though decoding a timestamp with a timezone requires the local time to be used along with the timezone code
		// so this has to adjust the value before decoding to parts and building a resulting string.
		timeArray[n] += ( use_tz * 900000LL ) * 1000000;
		
		ConvertTickToTime( ( (timeArray[n]/1000000)<<8)| (use_tz&0xFF), &st );
		//Local<Script> script;
		char buf[64];
		int tz;
		int negTz = 0;
		if( st.zhr < 0 ) {
			tz = -st.zhr;
			negTz = 1;
		} else
			tz = st.zhr;

		snprintf( buf, 64, "%04d-%02d-%02dT%02d:%02d:%02d.%03d%c%02d:%02d", st.yr, st.mo, st.dy, st.hr, st.mn, st.sc, st.ms, negTz ? '-' : '+', tz, st.zmn );
		Local<Value> args[2] = { String::NewFromUtf8( isolate, buf, NewStringType::kNormal ).ToLocalChecked(), Number::New( isolate, (double)(timeArray[n] % 1000000) ) };
		Local<Value> newDate = Local<Function>::New( isolate, c->dateNsCons )->NewInstance( isolate->GetCurrentContext(), 2, args ).ToLocalChecked();
		arr->Set( isolate->GetCurrentContext(), n, newDate );
	}
	return arr;
}

void ObjectStorageObject::fileGetTimes( const v8::FunctionCallbackInfo<Value>& args ) {

	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 1 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, TranslateText( "Requires filename to get time info for" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	ObjectStorageObject* vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	String::Utf8Value fName( isolate, args[0] );

	size_t timeCount = 0;
	uint64_t* timeArray = NULL;
	int8_t* tzArray = NULL;

	if( vol->volNative ) {
		if( !objStore::sack_vfs_os_exists( vol->vol, ( *fName ) ) ) {
			//lprintf( "object is not found: %s", *fName );

			return;
		}

		//lprintf( "OPEN FILE:%s", *fName );
		struct objStore::sack_vfs_os_file* file = objStore::sack_vfs_os_openfile( vol->vol, ( *fName ) );
		if( file ) {

			objStore::sack_vfs_os_get_times( file, &timeArray, &tzArray, &timeCount );
			objStore::sack_vfs_os_close( file );
		}
	} else {
		FILE* file = sack_fopenEx( 0, ( *fName ), "rb", vol->fsMount );
		if( file ) {
			sack_ioctl( file, SOSFSFIO_GET_TIMES, &timeArray, &tzArray, &timeCount );
			sack_fclose( file );
		}
	}

	args.GetReturnValue().Set( makeTimes( isolate, timeArray, tzArray, timeCount ) );

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
	uint64_t version = 0;
	while( arg < args.Length() ) {
		if( args[arg]->IsNumber() ) {
			// maybe make sure (arg==1) ?
			Local<Number> version_num = args[arg].As<Number>();
			version = (uint64_t)version_num->Value();
			arg++;
		} else if( args[arg]->IsFunction() ) {
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
		struct objStore::sack_vfs_os_file *file = 
			version
				?(struct objStore::sack_vfs_os_file*)objStore::sack_vfs_os_system_ioctl( vol->vol, SOSFSSIO_OPEN_VERSION, (*fName), version )
				:objStore::sack_vfs_os_openfile( vol->vol, (*fName) );
		if( file ) {
			char *buf = NewArray( char, 4096 );
			size_t len = objStore::sack_vfs_os_size( file );
			size_t read = 0;
			size_t newRead;
			size_t timeCount;
			LOGICAL resulted = FALSE;
			uint64_t *timeArray;
			int8_t* tzArray;
			Local<Array> arr;
			if( version )
				arr = Array::New(isolate,0);
			else {
				objStore::sack_vfs_os_get_times( file, &timeArray, &tzArray, &timeCount );
				arr = makeTimes( isolate, timeArray, tzArray, timeCount );
			}


			// CAN open directories; and they have 7ffffffff sizes.
			while( (read < len) && (newRead = objStore::sack_vfs_os_read( file, buf, 4096 )) ) {
				read += newRead;
				int result;
#ifdef DEBUG_LOG_PARSING
				printf( "B Parse file: %d %.*s\n", (int)newRead, (int)newRead, buf );
#endif
				for( (result = jsox_parse_add_data( parser, buf, newRead ));
					result > 0 || ( (read == len) && ( result = jsox_parse_add_data( parser, NULL, 0 ) ) );
					result = jsox_parse_add_data( parser, NULL, 0 ) ) {
					if( result < 0 ) {
						break;
					}
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
					resulted = TRUE;
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
					resulted = TRUE;
					PTEXT error = jsox_parse_get_error( parser );
					jsox_parse_clear_state(parser);
					if( error )
						isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, GetText( error ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
					else
						isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "No Error Text" STRSYM(__LINE__) ) ) );
					LineRelease( error );
				}
			}
			if( !resulted ) {
				//lprintf( "Disk data is short." );
				isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Disk Data is an incomplete object." ) ) );
			}
			Deallocate( char *, buf );
			objStore::sack_vfs_os_close( file );
		}

	} else {


		FILE *file = sack_fopenEx( 0, (*fName), "rb", vol->fsMount );

		if( file ) {
			char *buf = NewArray( char, 4096 );
			size_t len = sack_fsize( file );
			
			size_t timeCount;
			uint64_t* timeArray;
			int8_t* tzArray;

			sack_ioctl( file, SOSFSFIO_GET_TIMES, &timeArray, &tzArray, &timeCount );

			Local<Array> arr = makeTimes( isolate, timeArray, tzArray, timeCount );

			size_t read = 0;
			size_t newRead;
			struct jsox_parse_state *parser = jsox_begin_parse();
			// CAN open directories; and they have 7ffffffff sizes.
			while( (read < len) && (newRead = sack_fread( buf, 4096, 1, file )) ) {
				read += newRead;
				int result;
#ifdef DEBUG_LOG_PARSING
				printf( "A Parse file: %d %.*s\n", (int)newRead, (int)newRead, buf );
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
	ObjectStorageObject* obj = new ObjectStorageObject( mount, name, 0, key1, key2, useMount, 0 );
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
