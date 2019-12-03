#include "global.h"
using namespace sack::SACK_VFS::objStore;

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
	
public:

	static void Init( Isolate *isolate, Local<Object> exports );
	ObjectStorageObject( const char *mount, const char *filename, uintptr_t version, const char *key, const char *key2 );

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	// get object pass object ID
	static void getObject( const v8::FunctionCallbackInfo<Value>& args );

	// get object and all recursive objects associated from here (for 1 level?)
	static void mapObject( const v8::FunctionCallbackInfo<Value>& args );

	// pass object, result with object ID.
	static void putObject( const v8::FunctionCallbackInfo<Value>& args );

	// pass object ID, get back a ObjectStorageFileObject ( support seek/read/write? )
	static void openObject( const v8::FunctionCallbackInfo<Value>& args );

	// utility to remove the key so it can be diagnosed.
	static void volDecrypt( const v8::FunctionCallbackInfo<Value>& args );

	static void fileReadJSOX( const v8::FunctionCallbackInfo<Value>& args );

	static void fileWrite( const v8::FunctionCallbackInfo<Value>& args );
	static void fileStore( const v8::FunctionCallbackInfo<Value>& args );
	static void createIndex( const v8::FunctionCallbackInfo<Value>& args );
	static ObjectStorageObject* openInVFS( Isolate *isolate, const char *mount, const char *name, const char *key1, const char *key2 );
	~ObjectStorageObject();
};


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

static struct objStoreLocal {
	PLIST open;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	POBJECT_STORAGE_EVENTSET osEvents;
	PLIST strings;
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
	//(*pevt).buf = NewArray( uint8_t*, buflen );
	//lprintf( "Send buffer %p", (*pevt).buf );
	//memcpy( (POINTER)(*pevt).buf, buffer, buflen );
	//(*pevt).buflen = buflen;
	//(*pevt)._this = _this;
	//(*pevt).from = DuplicateAddress( from );
	EnqueLink( &_this->plqEvents, pevt );

	uv_async_send( &_this->async );
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
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "write", ObjectStorageObject::fileWrite );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "createIndex", ObjectStorageObject::createIndex );
	//NODE_SET_PROTOTYPE_METHOD( clsTemplate, "store", ObjectStorageObject::fileStore );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "put", ObjectStorageObject::putObject );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "get", ObjectStorageObject::getObject );

	Local<Function> VolFunc = clsTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();

	class constructorSet* c = getConstructors( isolate );
	c->ObjectStorageObject_constructor.Reset( isolate, clsTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	SET( exports, "ObjectStorage"
		, clsTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

}


ObjectStorageObject::~ObjectStorageObject() {
	if( volNative ) {
		Deallocate( char*, mountName );
		Deallocate( char*, fileName );
		//printf( "Volume object evaporated.\n" );
		sack_unmount_filesystem( fsMount );
		objStore::sack_vfs_os_unload_volume( vol );
		DeleteLink( &osl.open, vol );
	}
}

ObjectStorageObject::ObjectStorageObject( const char *mount, const char *filename, uintptr_t version, const char *key, const char *key2 ) {
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
		vol = objStore::sack_vfs_os_load_crypt_volume( filename, version, key, key2 );
		AddLink( &osl.open, vol );
		//lprintf( "VOL: %p for %s %d %p %p", vol, filename, version, key, key2 );
		if( vol )
			fsMount = sack_mount_filesystem( mount, fsInt = sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME "-os" )
				, 2000, (uintptr_t)vol, TRUE );
		else
			fsMount = NULL;
	}
}

#if 0
static void idGenerator( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() ) {
		int version = -1;
		if( args[0]->IsString() ) {
			char *r;
			struct random_context *ctx = SRG_CreateEntropy4( NULL, 0 );

			SRG_FeedEntropy( ctx, (uint8_t*)*val, val.length() );
			uint32_t buf[256 / 32];
			SRG_GetEntropyBuffer( ctx, buf, 256 );
			size_t outlen;
			r = EncodeBase64Ex( (uint8_t*)buf, (16 + 16), &outlen, (const char *)1 );
			SRG_DestroyEntropy( &ctx );
			args.GetReturnValue().Set( localString( isolate, r, (int)outlen ) );
		}
	}
}

#endif

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

void ObjectStorageObject::putObject( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate *isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct optionStrings *strings = getStrings( isolate );
	String::Utf8Value data( USE_ISOLATE(isolate) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	Local<Object> opts = args[1]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
	Local<String> optName;
	
	struct objectStorageOptions *osoOpts = NULL;
	ThreadTo( DoPutObject, (uintptr_t)osoOpts );
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
		int argc = args.Length();
		if( argc == 0 ) {
			ObjectStorageObject* obj = new ObjectStorageObject( NULL, NULL, 0, NULL, NULL );
			obj->Wrap( args.This() );
			args.GetReturnValue().Set( args.This() );
		}
		else {
			int arg = 0;
			if( args[0]->IsString() ) {

				//TooObject( isolate.getCurrentContext().FromMaybe( Local<Object>() )
				String::Utf8Value fName( isolate,  args[arg++]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
				mount_name = StrDup( *fName );
			}
			else {
				mount_name = SRG_ID_Generator();
			}
			if( argc > 1 ) {
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
			ObjectStorageObject* obj = new ObjectStorageObject( mount_name, filename, version, key, key2 );
			if( !obj->vol ) {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "Volume failed to open." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
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

void ObjectStorageObject::fileWrite( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, TranslateText( "Requires filename to open and data to write" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	ObjectStorageObject *vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	String::Utf8Value fName( isolate,  args[0] );
	//lprintf( "OPEN FILE:%s", *fName );
	if( vol->volNative ) {
		struct objStore::sack_vfs_os_file *file = objStore::sack_vfs_os_openfile( vol->vol, (*fName) );
		if( file ) {
			String::Utf8Value data( isolate,  args[1] );
			objStore::sack_vfs_os_write( file, *data, data.length() );
			objStore::sack_vfs_os_close( file );
			sack_vfs_os_polish_volume( vol->vol );
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
		if( !objStore::sack_vfs_os_exists( vol->vol, (*fName) ) ) return;

		lprintf( "OPEN FILE:%s", *fName );
		struct objStore::sack_vfs_os_file *file = objStore::sack_vfs_os_openfile( vol->vol, (*fName) );
		if( file ) {
			char *buf = NewArray( char, 4096 );
			size_t len = objStore::sack_vfs_os_size( file );
			size_t read = 0;
			size_t newRead;

			// CAN open directories; and they have 7ffffffff sizes.
			while( (read < len) && (newRead = objStore::sack_vfs_os_read( file, buf, 4096 )) ) {
				read += newRead;
				int result;
				//lprintf( "Parse file: %s", buf, newRead );
				for( (result = jsox_parse_add_data( parser, buf, newRead ));
					result > 0;
					result = jsox_parse_add_data( parser, NULL, 0 ) ) {
					//Local<Object> obj = Object::New( isolate );
					PDATALIST data;
					data = jsox_parse_get_data( parser );
					struct reviver_data r;
					r.revive = FALSE;
					r._this = args.This();
					r.isolate = isolate;
					r.context = isolate->GetCurrentContext();
					r.parser = parserObject;
					Local<Value> val = convertMessageToJS2( data, &r );
					{
						MaybeLocal<Value> result = cb->Call( r.context, isolate->GetCurrentContext()->Global(), 1, &val );
						if( result.IsEmpty() ) { // if an exception occurred stop, and return it. 
							jsox_dispose_message( &data );
							jsox_parse_dispose_state( &parser );
							return;
						}
					}
					jsox_dispose_message( &data );
					if( result == 1 )
						break;
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
				//lprintf( "Parse file:", buf );
				for( (result = jsox_parse_add_data( parser, buf, newRead ));
					result > 0;
					result = jsox_parse_add_data( parser, NULL, 0 ) ) {
					PDATALIST data;
					data = jsox_parse_get_data( parser );
					if( data->Cnt ) {
						struct reviver_data r;
						r.revive = FALSE;
						r.isolate = isolate;
						r.context = isolate->GetCurrentContext();
						Local<Value> val = convertMessageToJS2( data, &r );
						{
							MaybeLocal<Value> result = cb->Call( r.context, isolate->GetCurrentContext()->Global(), 1, &val );
							if( result.IsEmpty() ) { // if an exception occurred stop, and return it. 
								jsox_dispose_message( &data );
								jsox_parse_dispose_state( &parser );
								return;
							}
						}
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

ObjectStorageObject*  openInVFS( Isolate *isolate, const char *mount, const char *name, const char *key1, const char *key2 ) {

//ObjectStorageObject*  ObjectStorageObject::openInVFS( Isolate *isolate, const char *mount, const char *name, const char *key1, const char *key2 ) {

	// Invoked as constructor: `new MyObject(...)`
	ObjectStorageObject* obj = new ObjectStorageObject( mount, name, 0, key1, key2 );
	if( !obj->vol ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Volume failed to open." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		delete obj;
		obj = NULL;
	}
	return obj;

}

void ObjectStorageInit( Isolate *isolate, Local<Object> exports ) {
	ObjectStorageObject::Init( isolate, exports );
}
