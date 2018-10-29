#include "global.h"


class ObjectStorageObject : public node::ObjectWrap {
public:
	struct objStore::volume *vol;
	bool volNative;
	char *mountName;
	char *fileName;
	struct file_system_interface *fsInt;
	struct file_system_mounted_interface* fsMount;
	static v8::Persistent<v8::Function> constructor;

public:

	static void Init( Isolate *isolate, Handle<Object> exports );
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
	~ObjectStorageObject();
};

static struct objStoreLocal {
	PLIST open;
} osl;

Persistent<Function> ObjectStorageObject::constructor;

ATEXIT( closeVolumes ) {
	INDEX idx;
	struct objStore::volume* vol;
	LIST_FORALL( osl.open, idx, struct objStore::volume*, vol ) {
		sack_vfs_os_unload_volume( vol );
	}
}

void ObjectStorageObject::Init( Isolate *isolate, Handle<Object> exports ) {
	Local<FunctionTemplate> clsTemplate;
	clsTemplate = FunctionTemplate::New( isolate, New );
	clsTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.objectStorage" ) );
	clsTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "read", ObjectStorageObject::fileReadJSOX );
	NODE_SET_PROTOTYPE_METHOD( clsTemplate, "write", ObjectStorageObject::fileWrite );

	Local<Function> VolFunc = clsTemplate->GetFunction();
	//SET_READONLY_METHOD( VolFunc, "mkdir", mkdir );


	constructor.Reset( isolate, clsTemplate->GetFunction() );
	exports->Set( String::NewFromUtf8( isolate, "objectStorage" ),
					 clsTemplate->GetFunction() );

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
		vol = (struct objStore::volume*)sack_get_mounted_filesystem_instance( fsMount );

		//lprintf( "open native mount" );
	}
	else {
		//lprintf( "volume: %s %p %p", filename, key, key2 );
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
				String::Utf8Value fName( USE_ISOLATE( isolate ) args[arg++]->ToString() );
				mount_name = StrDup( *fName );
			}
			else {
				mount_name = SRG_ID_Generator();
			}
			if( argc > 1 ) {
				if( args[arg]->IsString() ) {
					String::Utf8Value fName( USE_ISOLATE( isolate ) args[arg++]->ToString() );
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
				version = (uintptr_t)args[arg++]->ToNumber( isolate )->Value();
			}
			if( argc > arg ) {
				if( !args[arg]->IsNull() && !args[arg]->IsUndefined() ) {
					String::Utf8Value k( USE_ISOLATE( isolate ) args[arg] );
					key = StrDup( *k );
				}
				arg++;
			}
			if( argc > arg ) {
				if( !args[arg]->IsNull() && !args[arg]->IsUndefined() ) {
					String::Utf8Value k( USE_ISOLATE( isolate ) args[arg] );
					key2 = StrDup( *k );
				}
				arg++;
			}
			// Invoked as constructor: `new MyObject(...)`
			ObjectStorageObject* obj = new ObjectStorageObject( mount_name, filename, version, key, key2 );
			if( !obj->vol ) {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "Volume failed to open." ) ) ) );
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

		Local<Function> cons = Local<Function>::New( isolate, constructor );
		MaybeLocal<Object> mo = cons->NewInstance( isolate->GetCurrentContext(), argc, argv );
		if( !mo.IsEmpty() )
			args.GetReturnValue().Set( mo.ToLocalChecked() );
		delete[] argv;
	}
}

void ObjectStorageObject::fileWrite( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, TranslateText( "Requires filename to open and data to write" ) ) ) );
		return;
	}
	ObjectStorageObject *vol = ObjectWrap::Unwrap<ObjectStorageObject>( args.Holder() );
	String::Utf8Value fName( USE_ISOLATE( isolate ) args[0] );
	//lprintf( "OPEN FILE:%s", *fName );
	if( vol->volNative ) {
		struct objStore::sack_vfs_file *file = objStore::sack_vfs_os_openfile( vol->vol, (*fName) );
		if( file ) {
			String::Utf8Value data( USE_ISOLATE( isolate ) args[1] );
			objStore::sack_vfs_os_write( file, *data, data.length() );
			objStore::sack_vfs_os_close( file );
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
			String::NewFromUtf8( isolate, TranslateText( "Requires filename to open and data callback" ) ) ) );
		return;
	}
	Local<Function> cb;
	String::Utf8Value fName( USE_ISOLATE( isolate ) args[0] );
	int arg = 1;
	while( arg < args.Length() ) {
		if( args[arg]->IsFunction() ) {
			cb = Handle<Function>::Cast( args[arg] );
			arg++;
		}
		else if( args[arg]->IsObject() ) {
			Local<Object> useParser = args[arg]->ToObject();
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
		struct objStore::sack_vfs_file *file = objStore::sack_vfs_os_openfile( vol->vol, (*fName) );
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
						MaybeLocal<Value> result = cb->Call( isolate->GetCurrentContext()->Global(), 1, &val );
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
							MaybeLocal<Value> result = cb->Call( isolate->GetCurrentContext()->Global(), 1, &val );
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



void ObjectStorageInit( Isolate *isolate, Handle<Object> exports ) {
	ObjectStorageObject::Init( isolate, exports );
}
