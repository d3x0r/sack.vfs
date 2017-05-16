
#include "global.h"



static void moduleExit( void *arg ) {
	InvokeExits();
}

void VolumeObject::Init( Handle<Object> exports ) {
	InvokeDeadstart();
	node::AtExit( moduleExit );
	//SetAllocateLogging( TRUE );
	SetAllocateDebug( FALSE );
	SetManualAllocateCheck( TRUE );
	SetSystemLog( SYSLOG_FILE, stdout );
	//lprintf( "Stdout Logging Enabled." );

	Isolate* isolate = Isolate::GetCurrent();
	Local<FunctionTemplate> volumeTemplate;
	ThreadObject::Init( exports );
	FileObject::Init();
	SqlObject::Init( exports );
	ComObject::Init( exports );
	InitJSON( isolate, exports );
#ifdef WIN32
	RegObject::Init( exports );
#endif
	// Prepare constructor template
	volumeTemplate = FunctionTemplate::New( isolate, New );
	volumeTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.Volume" ) );
	volumeTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "File", FileObject::openFile );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "dir", getDirectory );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "exists", fileExists );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "read", fileRead );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "write", fileWrite );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "mkdir", makeDirectory );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "Sqlite", openVolDb );

	NODE_SET_METHOD(exports, "mkdir", mkdir );

	constructor.Reset( isolate, volumeTemplate->GetFunction() );
	exports->Set( String::NewFromUtf8( isolate, "Volume" ),
		volumeTemplate->GetFunction() );
	//NODE_SET_METHOD( exports, "InitFS", InitFS );
}


VolumeObject::VolumeObject( const char *mount, const char *filename, const char *key, const char *key2 )  {
	mountName = (char *)mount;
	if( !mount && !filename ) {
		volNative = false;
		fsMount = sack_get_default_mount();
	} else {
		//lprintf( "volume: %s %p %p", filename, key, key2 );
		fileName = StrDup( filename );
		volNative = true;
		vol = sack_vfs_load_crypt_volume( filename, key, key2 );
		if( vol )
			fsMount = sack_mount_filesystem( mount, sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME )
					, 2000, (uintptr_t)vol, TRUE );
		else
			fsMount = NULL;
	}
}



void logBinary( char *x, int n )
{
	int m;
	for( m = 0; m < n; ) {
		int o;
		for( o = 0; o < 16 && m < n; o++, m++ ) {
			printf( "%02x ", x[m] );
		}
		m -= o;
		for( o = 0; o < 16 && m < n; o++, m++ ) {
			printf( "%c", (x[m]>32 && x[m]<127)?x[m]:'.' );
		}
	}
}

void VolumeObject::mkdir( const FunctionCallbackInfo<Value>& args ){
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc > 0 ) {
  		String::Utf8Value fName( args[0] );
  		MakePath( *fName );
	}
}

void VolumeObject::makeDirectory( const FunctionCallbackInfo<Value>& args ){
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc > 0 ) {
		VolumeObject *vol = ObjectWrap::Unwrap<VolumeObject>( args.Holder() );
		if( vol ) {
			String::Utf8Value fName( args[0] );
			if( vol->volNative ) {
				// no directory support; noop.
			} else {
				MakePath( *fName );
			}
		}
	}
}


void VolumeObject::openVolDb( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( args.IsConstructCall() ) {
		if( argc == 0 ) {
			isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, "Required filename missing." ) ) );
			return;
		}
		else {

			VolumeObject *vol = ObjectWrap::Unwrap<VolumeObject>( (argc > 1)?args[1]->ToObject():args.Holder() );
			if( !vol->mountName )
			{
				isolate->ThrowException( Exception::Error(
						String::NewFromUtf8( isolate, "Volume is not mounted; cannot be used to open Sqlite database." ) ) );
				return;
				
			}
			String::Utf8Value fName( args[0] );
			SqlObject* obj;
			char dbName[256];
			snprintf( dbName, 256, "$sack@%s$%s", vol->mountName, (*fName) );
			obj = new SqlObject( dbName );
			SqlObject::doWrap( obj, args.This() );

			args.GetReturnValue().Set( args.This() );
		}

	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
  		VolumeObject *vol = ObjectWrap::Unwrap<VolumeObject>( (argc > 1)?args[1]->ToObject():args.Holder() );
  		if( !vol->mountName )
  		{
  			isolate->ThrowException( Exception::Error(
  					String::NewFromUtf8( isolate, "Volume is not mounted; cannot be used to open Sqlite database." ) ) );
  			return;
  		}
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[2];
		char dbName[256];
		String::Utf8Value fName( args[0] );
  		snprintf( dbName, 256, "$sack@%s$%s", vol->mountName, (*fName) );
  		argv[0] = String::NewFromUtf8( isolate, dbName );
		argv[1] = args.Holder();

		Local<Function> cons = Local<Function>::New( isolate, SqlObject::constructor );
		MaybeLocal<Object> mo = Nan::NewInstance( cons, argc, argv );
		if( !mo.IsEmpty() )
			args.GetReturnValue().Set( mo.ToLocalChecked() );
		delete[] argv;
	}

}


static void fileBufToString( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = Isolate::GetCurrent();
	// can't get to this function except if it was an array buffer I allocated and attached this to.
	Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args.This() );
	MaybeLocal<String> retval = String::NewFromUtf8( isolate, (const char*)ab->GetContents().Data(), NewStringType::kNormal, (int)ab->ByteLength() );
	args.GetReturnValue().Set( retval.ToLocalChecked() );
}


	void VolumeObject::fileRead( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		VolumeObject *vol = ObjectWrap::Unwrap<VolumeObject>( args.Holder() );

		if( args.Length() < 1 ) {
			isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( isolate, "Requires filename to open" ) ) );
			return;
		}

		String::Utf8Value fName( args[0] );

		if( vol->volNative ) {
			struct sack_vfs_file *file = sack_vfs_openfile( vol->vol, (*fName) );
			if( file ) {
				size_t len = sack_vfs_size( file );
				uint8_t *buf = NewArray( uint8_t, len );
				sack_vfs_read( file, (char*)buf, len );
				
				Local<Object> arrayBuffer = ArrayBuffer::New( isolate, buf, len );
				NODE_SET_METHOD( arrayBuffer, "toString", fileBufToString );
				args.GetReturnValue().Set( arrayBuffer );
			
				sack_vfs_close( file );
			}

		} else {
			FILE *file = sack_fopenEx( 0, (*fName), "rb", vol->fsMount );
			if( file ) {
				size_t len = sack_fsize( file );
				// CAN open directories; and they have 7ffffffff sizes.
				if( len < 0x10000000 ) {
					uint8_t *buf = NewArray( uint8_t, len );
					sack_fread( buf, len, 1, file );

					Local<Object> arrayBuffer = ArrayBuffer::New( isolate, buf, len );
					NODE_SET_METHOD( arrayBuffer, "toString", fileBufToString );
					args.GetReturnValue().Set( arrayBuffer );
				}
				sack_fclose( file );
			}

		}
	}

	void VolumeObject::fileWrite( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		VolumeObject *vol = ObjectWrap::Unwrap<VolumeObject>( args.Holder() );

		if( args.Length() < 2 ) {
			isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( isolate, "Requires filename to open and data to write" ) ) );
			return;
		}

		String::Utf8Value fName( args[0] );
		
		if(args[1]->IsArrayBuffer()) {
			Local<ArrayBuffer> myarr = args[1].As<ArrayBuffer>();
			Nan::TypedArrayContents<uint8_t> dest(myarr);
			uint8_t *buf = *dest;

			if( vol->volNative ) {
				struct sack_vfs_file *file = sack_vfs_openfile( vol->vol, *fName );
				sack_vfs_write( file, (const char*)buf, myarr->ByteLength() );
				sack_vfs_close( file );
	        
				args.GetReturnValue().Set( True(isolate) );
			} else {
				FILE *file = sack_fopenEx( 0, *fName, "wb", vol->fsMount );
				sack_fwrite( buf, myarr->ByteLength(), 1, file );
				sack_fclose( file );

				args.GetReturnValue().Set( True(isolate) );
			}
		}
		else if(args[1]->IsString()) {
			char *f = StrDup( *fName );
			String::Utf8Value buffer( args[1] );
			//Local<ArrayBuffer> myarr = args[1].As<ArrayBuffer>();
			//Nan::TypedArrayContents<uint8_t> dest(myarr);
			char *buf = *buffer;

			if( vol->volNative ) {
				struct sack_vfs_file *file = sack_vfs_openfile( vol->vol, f );
				sack_vfs_write( file, (const char*)buf, buffer.length()  );
				sack_vfs_close( file );
	        
				args.GetReturnValue().Set( True(isolate) );
			} else {
				FILE *file = sack_fopenEx( 0, f, "wb", vol->fsMount );
				sack_fwrite( buf, buffer.length(), 1, file );
				sack_fclose( file );

				args.GetReturnValue().Set( True(isolate) );
			}
			Deallocate( char*, f );
		}
	}


	void VolumeObject::fileExists( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		VolumeObject *vol = ObjectWrap::Unwrap<VolumeObject>( args.Holder() );
		String::Utf8Value fName( args[0] );
		if( vol->volNative ) {
			args.GetReturnValue().Set( sack_vfs_exists( (uintptr_t)vol->vol, *fName ) );
		}else {
			args.GetReturnValue().Set( sack_existsEx( *fName, vol->fsMount )?True(isolate):False(isolate) );
		}
	}

	void VolumeObject::getDirectory( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		VolumeObject *vol = ObjectWrap::Unwrap<VolumeObject>( args.Holder() );
		if( !vol->vol )
			return;
		struct find_info *fi = sack_vfs_find_create_cursor( (uintptr_t)vol->vol, NULL, NULL );
		Local<Array> result = Array::New( isolate );
		int found;
		int n = 0;
		for( found = sack_vfs_find_first( fi ); found; found = sack_vfs_find_next( fi ) ) {
			char *name = sack_vfs_find_get_name( fi );
			result->Set( n++, String::NewFromUtf8( isolate, name ) );
		} 
		args.GetReturnValue().Set( result );
	}

	void VolumeObject::New( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		if( args.IsConstructCall() ) {
			char *mount_name;
			char *filename = (char*)"default.vfs";
			LOGICAL defaultFilename = TRUE;
			char *key = NULL;
			char *key2 = NULL;
			int argc = args.Length();
			if( argc == 0 ) {
				VolumeObject* obj = new VolumeObject( NULL, NULL, NULL, NULL );
				obj->Wrap( args.This() );
				args.GetReturnValue().Set( args.This() );
			}
			else {
				//if( argc > 0 ) {
				String::Utf8Value fName( args[0]->ToString() );
				mount_name = StrDup( *fName );
				//}
				if( argc > 1 ) {
					String::Utf8Value fName( args[1]->ToString() );
					defaultFilename = FALSE;
					filename = StrDup( *fName );
				}
				if( argc > 2 ) {
					String::Utf8Value k( args[2] );
					if( args[2]->IsNull() || args[2]->IsUndefined() )
						key = NULL;
					else
					key = StrDup( *k );
				}
				if( argc > 3 ) {
					String::Utf8Value k( args[3] );
					if( args[3]->IsNull() || args[3]->IsUndefined() )
						key2 = NULL;
					else
						key2 = StrDup( *k );
				}
				// Invoked as constructor: `new MyObject(...)`
				VolumeObject* obj = new VolumeObject( mount_name, filename, key, key2 );
				if( !obj->vol ) {
					isolate->ThrowException( Exception::Error(
						String::NewFromUtf8( isolate, "Volume failed to open." ) ) );

				} else {
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
			MaybeLocal<Object> mo = Nan::NewInstance( cons, argc, argv );
			if( !mo.IsEmpty() )
				args.GetReturnValue().Set( mo.ToLocalChecked() );
			delete[] argv;
		}
	}


void FileObject::Emitter(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = Isolate::GetCurrent();
	//HandleScope scope;
	Handle<Value> argv[2] = {
		v8::String::NewFromUtf8( isolate, "ping"), // event name
		args[0]->ToString()  // argument
	};

	node::MakeCallback(isolate, args.This(), "emit", 2, argv);
}

void FileObject::readFile(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	FileObject *file = ObjectWrap::Unwrap<FileObject>( args.This() );
	//SACK_VFS_PROC size_t CPROC sack_vfs_read( struct sack_vfs_file *file, char * data, size_t length );
	if( args.Length() == 0 ) {
		if( !file->buf ) {
			file->size = sack_vfs_size( file->file );
			file->buf = NewArray( char, file->size );
			sack_vfs_read( file->file, file->buf, file->size );
		}
		{
			Local<Object> arrayBuffer = ArrayBuffer::New( isolate, file->buf, file->size );
			NODE_SET_METHOD( arrayBuffer, "toString", fileBufToString );

			args.GetReturnValue().Set( arrayBuffer );
		}
		//Local<String> result = String::NewFromUtf8( isolate, buf );
		//args.GetReturnValue().Set( result );
	}
}


void FileObject::writeFile(const FunctionCallbackInfo<Value>& args) {
	//Isolate* isolate = Isolate::GetCurrent();
	FileObject *file = ObjectWrap::Unwrap<FileObject>( args.This() );

	//SACK_VFS_PROC size_t CPROC sack_vfs_write( struct sack_vfs_file *file, char * data, size_t length );
	if( args.Length() == 1 ) {
		if( args[0]->IsString() ) {
			String::Utf8Value data( args[0]->ToString() );
			sack_vfs_write( file->file, *data, data.length() );
		} else if( args[0]->IsArrayBuffer() ) {
			Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
			ArrayBuffer::Contents contents = ab->GetContents();
			//file->buf = (char*)contents.Data();
			//file->size = contents.ByteLength();
			sack_vfs_write( file->file, (char*)contents.Data(), contents.ByteLength() );
		}
	}

}

void FileObject::truncateFile(const FunctionCallbackInfo<Value>& args) {
	//Isolate* isolate = Isolate::GetCurrent();
	FileObject *file = ObjectWrap::Unwrap<FileObject>( args.This() );
	sack_vfs_truncate( file->file ); // sets end of file mark to current position.
}

void FileObject::seekFile(const FunctionCallbackInfo<Value>& args) {
#if  V8_AT_LEAST(5, 4)
//	Isolate* isolate = Isolate::GetCurrent();
	size_t num1 = (size_t)args[0]->ToNumber(Isolate::GetCurrent()->GetCurrentContext()).FromMaybe(Local<Number>())->Value();
#endif
	FileObject *file = ObjectWrap::Unwrap<FileObject>( args.This() );
	if( args.Length() == 1 && args[0]->IsNumber() ) {
#if  V8_AT_LEAST(5, 4)
		sack_vfs_seek( file->file, num1, SEEK_SET );
#else
		sack_vfs_seek( file->file, (size_t)args[0]->ToNumber()->Value(), SEEK_SET );
#endif
	}
	if( args.Length() == 2 && args[0]->IsNumber() && args[1]->IsNumber() ) {
#if  V8_AT_LEAST(5, 4)
		int num2 = (int)args[1]->ToNumber(Isolate::GetCurrent()->GetCurrentContext()).FromMaybe(Local<Number>())->Value();
		sack_vfs_seek( file->file, num1, (int)num2 );
#else
		sack_vfs_seek( file->file, (size_t)args[0]->ToNumber()->Value(), (int)args[1]->ToNumber()->Value() );
#endif
	}

}


	void FileObject::Init(  ) {
		Isolate* isolate = Isolate::GetCurrent();

		Local<FunctionTemplate> fileTemplate;
		// Prepare constructor template
		fileTemplate = FunctionTemplate::New( isolate, openFile );
		fileTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.File" ) );
		fileTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

		// Prototype
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "read", readFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "write", writeFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "seek", seekFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "trunc", truncateFile );
		// read stream methods
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "pause", openFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "resume", openFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "setEncoding", openFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "unpipe", openFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "unshift", openFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "wrap", openFile );
		// write stream methods
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "cork", openFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "uncork", openFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "end", openFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "setDefaultEncoding", openFile );

		//NODE_SET_PROTOTYPE_METHOD( fileTemplate, "isPaused", openFile );

		constructor.Reset( isolate, fileTemplate->GetFunction() );
		//exports->Set( String::NewFromUtf8( isolate, "File" ),
		//	fileTemplate->GetFunction() );
	}

	void FileObject::openFile( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		if( args.Length() < 1 ) {
			isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( isolate, "Requires filename to open" ) ) );
			return;
		}
		VolumeObject* vol;
		char *filename;
		String::Utf8Value fName( args[0] );
		filename = *fName;

		if( args.IsConstructCall() ) {
			// Invoked as constructor: `new MyObject(...)`
			FileObject* obj;
			if( args.Length() < 2 ) {
				vol = ObjectWrap::Unwrap<VolumeObject>( args.Holder() );
				obj = new FileObject( vol, filename, isolate, args.Holder() );
			}
			else {
				vol = ObjectWrap::Unwrap<VolumeObject>( args[1]->ToObject() );
				obj = new FileObject( vol, filename, isolate, args[1]->ToObject() );
			}
			obj->Wrap( args.This() );
			args.GetReturnValue().Set( args.This() );
		}
		else {
			const int argc = 2;
			Local<Value> argv[argc] = { args[0], args.Holder() };
			Local<Function> cons = Local<Function>::New( isolate, constructor );
			args.GetReturnValue().Set( Nan::NewInstance( cons, argc, argv ).ToLocalChecked() );
		}
	}

	FileObject::FileObject( VolumeObject* vol, const char *filename, Isolate* isolate, Local<Object> o ) : 
		volume( isolate, o )
	{
		buf = NULL;
		if( vol->volNative ) {
			if( !vol->vol )
				return;
			file = sack_vfs_openfile( vol->vol, filename );
		} else {
			cfile = sack_fopenEx( 0, filename, "rb", vol->fsMount );
		}
	}


Persistent<Function> VolumeObject::constructor;
Persistent<Function> FileObject::constructor;

VolumeObject::~VolumeObject() {
	if( volNative ) {
		Deallocate( char*, mountName );
		Deallocate( char*, fileName );
		//printf( "Volume object evaporated.\n" );
		sack_unmount_filesystem( fsMount );
		sack_vfs_unload_volume( vol );
	}
}

FileObject::~FileObject() {
	//printf( "File object evaporated.\n" );
	if( buf )
		Deallocate( char*, buf );
	if( vol->volNative )
		sack_vfs_close( file );
	else
		sack_fclose( cfile );
	volume.Reset();
}


//-----------------------------------------------------------


NODE_MODULE( vfs_module, VolumeObject::Init)

