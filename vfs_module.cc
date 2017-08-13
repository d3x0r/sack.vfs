
#include "global.h"



static void moduleExit( void *arg ) {
	//SaveTranslationDataEx( "^/strings.dat" );
	SaveTranslationDataEx( "@/../../strings.json" );
	InvokeExits();
}

static void vfs_b64xor(const v8::FunctionCallbackInfo<Value>& args ){
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc > 1 ) {
  		String::Utf8Value xor1( args[0] );
  		String::Utf8Value xor2( args[1] );
		//lprintf( "is buffer overlapped? %s %s", *xor1, *xor2 );
		char *r = b64xor( *xor1, *xor2 );
		MaybeLocal<String> retval = String::NewFromUtf8( isolate, r );
		args.GetReturnValue().Set( retval.ToLocalChecked() );
		Deallocate( char*, r );
	}
}

static void vfs_u8xor(const v8::FunctionCallbackInfo<Value>& args ){
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc > 0 ) {
  		String::Utf8Value xor1( args[0] );
  		Local<Object> key = args[1]->ToObject();
		//Handle<Object> 
		Local<String> tmp;
		Local<Value> keyValue = key->Get( String::NewFromUtf8( isolate, "key" ) );
		Local<Value> stepValue = key->Get( tmp = String::NewFromUtf8( isolate, "step" ) );
		int step = (int)stepValue->IntegerValue();
		String::Utf8Value xor2( keyValue );
		//lprintf( "is buffer overlapped? %s %s %d", *xor1, *xor2, step );
		char *out = u8xor( *xor1, (size_t)xor1.length(), *xor2, (size_t)xor2.length(), &step );
		//lprintf( "encoded1:%s %d", out, step );
		key->Set( tmp, Integer::New( isolate, step ) );
		//lprintf( "length: %d %d", xor1.length(), StrLen( *xor1 ) );
		args.GetReturnValue().Set( String::NewFromUtf8( isolate, out, NewStringType::kNormal, (int)xor1.length() ).ToLocalChecked() );
		Deallocate( char*, out );
	}
}
static void idGenerator(const v8::FunctionCallbackInfo<Value>& args ){
	Isolate* isolate = args.GetIsolate();
	char *r = SRG_ID_Generator();
	args.GetReturnValue().Set( String::NewFromUtf8( isolate, r ) );
	Deallocate( char*, r );
}


static void dumpMem( const v8::FunctionCallbackInfo<Value>& args ) {
	DebugDumpMem( );
}


void VolumeObject::Init( Handle<Object> exports ) {
	InvokeDeadstart();
	node::AtExit( moduleExit );

	//SetAllocateLogging( TRUE );
	SetManualAllocateCheck( TRUE );
	SetAllocateDebug( TRUE );
	SetSystemLog( SYSLOG_FILE, stdout );

	//LoadTranslationDataEx( "^/strings.dat" );
	LoadTranslationDataEx( "@/../../strings.json" );
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
	TLSObject::Init( isolate, exports );

	// Prepare constructor template
	volumeTemplate = FunctionTemplate::New( isolate, New );
	volumeTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.Volume" ) );
	volumeTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "File", FileObject::openFile );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "dir", getDirectory );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "exists", fileExists );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "read", fileRead );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "readJSON6", fileReadJSON );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "write", fileWrite );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "mkdir", makeDirectory );
	NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "Sqlite", openVolDb );

	NODE_SET_METHOD( exports, "memDump", dumpMem );
	NODE_SET_METHOD(exports, "mkdir", mkdir );
	NODE_SET_METHOD(exports, "u8xor", vfs_u8xor );
	NODE_SET_METHOD(exports, "b64xor", vfs_b64xor );
	NODE_SET_METHOD(exports, "id", idGenerator );

	Local<Object> fileObject = Object::New( isolate );	
	fileObject->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8( isolate, "SeekSet" ), Integer::New( isolate, SEEK_SET ), ReadOnlyProperty );
	fileObject->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8( isolate, "SeekCurrent" ), Integer::New( isolate, SEEK_CUR ), ReadOnlyProperty );
	fileObject->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8( isolate, "SeekEnd" ), Integer::New( isolate, SEEK_END ), ReadOnlyProperty );
	exports->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8( isolate, "File" ), fileObject, ReadOnlyProperty );

	constructor.Reset( isolate, volumeTemplate->GetFunction() );
	exports->Set( String::NewFromUtf8( isolate, "Volume" ),
		volumeTemplate->GetFunction() );
	//NODE_SET_METHOD( exports, "InitFS", InitFS );
}


VolumeObject::VolumeObject( const char *mount, const char *filename, const char *key, const char *key2 )  {
	mountName = (char *)mount;
	if( !mount && !filename ) {
		volNative = false;
		fsInt = sack_get_filesystem_interface( "native" );
		//lprintf( "open native mount" );
		fsMount = sack_get_default_mount();
	} else {
		//lprintf( "volume: %s %p %p", filename, key, key2 );
		fileName = StrDup( filename );
		volNative = true;
		vol = sack_vfs_load_crypt_volume( filename, key, key2 );
		if( vol )
			fsMount = sack_mount_filesystem( mount, fsInt = sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME )
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

void VolumeObject::mkdir( const v8::FunctionCallbackInfo<Value>& args ){
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc > 0 ) {
  		String::Utf8Value fName( args[0] );
  		MakePath( *fName );
	}
}

void VolumeObject::makeDirectory( const v8::FunctionCallbackInfo<Value>& args ){
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


void VolumeObject::openVolDb( const v8::FunctionCallbackInfo<Value>& args ) {
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


static void fileBufToString( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = Isolate::GetCurrent();
	// can't get to this function except if it was an array buffer I allocated and attached this to.
	Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args.This() );
	char *output = NewArray( char, ab->ByteLength() );
	int out_index = 0;
	{
		const char *input = (const char*)ab->GetContents().Data();
		size_t index = 0;
		TEXTRUNE rune;
		size_t len = ab->ByteLength();
		//LogBinary( input, len );
		while( index < len ) {
			rune = GetUtfCharIndexed( input, &index, len );
			//lprintf( "rune:%d at %d of %d   to %d", rune, (int)index, (int)len, (int)out_index );
			if( rune != INVALID_RUNE )
				out_index += ConvertToUTF8( output+out_index, rune );
			else
				out_index += ConvertToUTF8( output+out_index, 0xFFFD );
			//lprintf( "new index:%d", (int)out_index );
		}
		//LogBinary( output, out_index );
	}
	MaybeLocal<String> retval = String::NewFromUtf8( isolate, (const char*)output, NewStringType::kNormal, (int)out_index );
	args.GetReturnValue().Set( retval.ToLocalChecked() );
	Deallocate( char*, output );
}


	void VolumeObject::fileReadJSON( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		VolumeObject *vol = ObjectWrap::Unwrap<VolumeObject>( args.Holder() );

		if( args.Length() < 1 ) {
			isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( isolate, "Requires filename to open and data callback" ) ) );
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

	void VolumeObject::fileRead( const v8::FunctionCallbackInfo<Value>& args ) {
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

		}
		else {
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

	void VolumeObject::fileWrite( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		VolumeObject *vol = ObjectWrap::Unwrap<VolumeObject>( args.Holder() );

		if( args.Length() < 2 ) {
			isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( isolate, "Requires filename to open and data to write" ) ) );
			return;
		}
		LOGICAL overlong = FALSE;
		if( args.Length() > 2 ) {
			if( args[2]->ToBoolean()->Value() )
				overlong = TRUE;
		}
		String::Utf8Value fName( args[0] );
		int type = 0;		
		if( ( (type=1), args[1]->IsArrayBuffer()) || ((type=2),args[1]->IsUint8Array()) ) {
			uint8_t *buf ;
			size_t length;
			if( type == 1 ) {
				Local<ArrayBuffer> myarr = args[1].As<ArrayBuffer>();
				Nan::TypedArrayContents<uint8_t> dest(myarr);
				buf = *dest;
				length = myarr->ByteLength();
			} else if( type == 2 ) {
				Local<Uint8Array> _myarr = args[1].As<Uint8Array>();
				//Local<ArrayBuffer> myarr = _myarr->Buffer();
				Nan::TypedArrayContents<uint8_t> dest( _myarr );
				//Nan::TypedArrayContents<uint8_t> dest(myarr);
				buf = *dest;
				length = _myarr->Length();
			}

			if( vol->volNative ) {
				struct sack_vfs_file *file = sack_vfs_openfile( vol->vol, *fName );
				sack_vfs_write( file, (const char*)buf, length );
				sack_vfs_close( file );

				args.GetReturnValue().Set( True(isolate) );
			} else {
				FILE *file = sack_fopenEx( 0, *fName, "wb", vol->fsMount );
				sack_fwrite( buf, length, 1, file );
				sack_fclose( file );

				args.GetReturnValue().Set( True(isolate) );
			}
		}
		else if(args[1]->IsString()) {
			char *f = StrDup( *fName );
			String::Utf8Value buffer( args[1] );
			//Local<ArrayBuffer> myarr = args[1].As<ArrayBuffer>();
			//Nan::TypedArrayContents<uint8_t> dest(myarr);
			const char *buf = *buffer;
			if( !overlong ) {
				if( vol->volNative ) {
					struct sack_vfs_file *file = sack_vfs_openfile( vol->vol, f );
					if( file ) {
						sack_vfs_write( file, (const char*)buf, buffer.length() );
						sack_vfs_close( file );

						args.GetReturnValue().Set( True( isolate ) );
					}
					else
						args.GetReturnValue().Set( False( isolate ) );
				}
				else {
					FILE *file = sack_fopenEx( 0, f, "wb", vol->fsMount );
					if( file ) {
						sack_fwrite( buf, buffer.length(), 1, file );
						sack_fclose( file );

						args.GetReturnValue().Set( True( isolate ) );
					}
					else
						args.GetReturnValue().Set( False( isolate ) );
				}
			}
			else {
				PVARTEXT pvtOut = VarTextCreate();
				TEXTRUNE c;
				while( c = GetUtfChar( &buf ) )
					VarTextAddRuneEx( pvtOut, c, TRUE DBG_SRC );
				PTEXT out = VarTextPeek( pvtOut );

				if( vol->volNative ) {
					struct sack_vfs_file *file = sack_vfs_openfile( vol->vol, f );
					if( file ) {
						sack_vfs_write( file, (const char*)GetText(out), GetTextSize( out ) );
						sack_vfs_close( file );

						args.GetReturnValue().Set( True( isolate ) );
					}
					else
						args.GetReturnValue().Set( False( isolate ) );
				}
				else {
					FILE *file = sack_fopenEx( 0, f, "wb", vol->fsMount );
					if( file ) {
						sack_fwrite( GetText( out ), GetTextSize( out ), 1, file );
						sack_fclose( file );

						args.GetReturnValue().Set( True( isolate ) );
					}
					else
						args.GetReturnValue().Set( False( isolate ) );
				}

			}
			Deallocate( char*, f );
		} else {
			isolate->ThrowException( Exception::Error(
						String::NewFromUtf8( isolate, "Data to write is not an ArrayBuffer or String." ) ) );

		}
	}


	void VolumeObject::fileExists( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		VolumeObject *vol = ObjectWrap::Unwrap<VolumeObject>( args.Holder() );
		String::Utf8Value fName( args[0] );
		if( vol->volNative ) {
			args.GetReturnValue().Set( sack_vfs_exists( (uintptr_t)vol->vol, *fName ) );
		}else {
			args.GetReturnValue().Set( sack_existsEx( *fName, vol->fsMount )?True(isolate):False(isolate) );
		}
	}

	void VolumeObject::getDirectory( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		VolumeObject *vol = ObjectWrap::Unwrap<VolumeObject>( args.Holder() );
		struct find_cursor *fi = vol->fsInt->find_create_cursor( (uintptr_t)vol->vol, ".", "*" );
		Local<Array> result = Array::New( isolate );
		int found;
		int n = 0;
		for( found = vol->fsInt->find_first( fi ); found; found = vol->fsInt->find_next( fi ) ) {
			char *name = vol->fsInt->find_get_name( fi );
			result->Set( n++, String::NewFromUtf8( isolate, name ) );
		} 
		vol->fsInt->find_close( fi );
		args.GetReturnValue().Set( result );
	}

	void VolumeObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
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
				else {
					defaultFilename = FALSE;
					filename = mount_name;
					mount_name = NULL;
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


void FileObject::Emitter(const v8::FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = Isolate::GetCurrent();
	//HandleScope scope;
	Handle<Value> argv[2] = {
		v8::String::NewFromUtf8( isolate, "ping"), // event name
		args[0]->ToString()  // argument
	};

	node::MakeCallback(isolate, args.This(), "emit", 2, argv);
}

void FileObject::readFile(const v8::FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	FileObject *file = ObjectWrap::Unwrap<FileObject>( args.This() );
	//SACK_VFS_PROC size_t CPROC sack_vfs_read( struct sack_vfs_file *file, char * data, size_t length );
	if( args.Length() == 0 ) {
		if( !file->buf ) {
			if( file->vol->volNative ) {
				file->size = sack_vfs_size( file->file );
				file->buf = NewArray( char, file->size );
				sack_vfs_read( file->file, file->buf, file->size );
			}
			else {
				file->size = sack_fsize( file->cfile );
				file->buf = NewArray( char, file->size );
				sack_fread( file->buf, file->size, 1, file->cfile );
			}
		}
		{
			Local<Object> arrayBuffer = ArrayBuffer::New( isolate, file->buf, file->size );
			NODE_SET_METHOD( arrayBuffer, "toString", fileBufToString );
			args.GetReturnValue().Set( arrayBuffer );
		}
		//Local<String> result = String::NewFromUtf8( isolate, buf );
		//args.GetReturnValue().Set( result );
	}
	else {
		size_t length;
		size_t position;
		int whence;
		if( args.Length() == 1 ) {
			length = args[1]->ToInteger()->Value();
			whence = SEEK_CUR;
		}
		else if( args.Length() == 2 ) {
			length = args[1]->ToInteger()->Value();
			position = args[2]->ToInteger()->Value();
			whence = SEEK_SET;
		}
		else {
			isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( isolate, "Too many parameters passed to read. ([length [,offset]])" ) ) );
			return;
		}
		if( length > file->size ) {
			AddLink( &file->buffers, file->buf );
			file->buf = NewArray( char, length );
			file->size = length;
		}
	}
}

static void vfs_string_read( char buf, size_t maxlen, struct sack_vfs_file *file ) {
	lprintf( "volume string read is not implemented yet." );
}

void FileObject::readLine(const v8::FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	FileObject *file = ObjectWrap::Unwrap<FileObject>( args.This() );
	//SACK_VFS_PROC size_t CPROC sack_vfs_read( struct sack_vfs_file *file, char * data, size_t length );
	if( args.Length() == 0 ) {
		if( !file->buf ) {
			file->buf = NewArray( char, 4096 );
		}
		if( file->vol->volNative ) {
			sack_vfs_read( file->file, file->buf, file->size );
		}
		else {
			int lastChar;
			if( sack_fgets( file->buf, 4096, file->cfile ) ) {
				lastChar = (int)strlen( file->buf );
				if( lastChar > 0 ) {
					if( file->buf[lastChar - 1] == '\n' )
						file->buf[lastChar - 1] = 0;
				}
			}
			else {
				args.GetReturnValue().Set( Null(isolate) );
				return;
			}
		}
		{
			MaybeLocal<String> result = String::NewFromUtf8( isolate, file->buf, NewStringType::kNormal, (int)strlen( file->buf ) );
			args.GetReturnValue().Set( result.ToLocalChecked() );
		}
		//Local<String> result = String::NewFromUtf8( isolate, buf );
		//args.GetReturnValue().Set( result );
	}
	else if( args.Length() == 1 ) {
		// get offset
	}
	else {
		// get length
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, "Too many parameters passed to readLine. ([offset])" ) ) );
	}
}


void FileObject::writeFile(const v8::FunctionCallbackInfo<Value>& args) {
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

void FileObject::writeLine(const v8::FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	FileObject *file = ObjectWrap::Unwrap<FileObject>( args.This() );

	//SACK_VFS_PROC size_t CPROC sack_vfs_write( struct sack_vfs_file *file, char * data, size_t length );
	if( args.Length() > 2 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, "Too many parameters passed to writeLine.  ( buffer, [,offset])" ) ) );
		return;
	}
	size_t offset;
	LOGICAL setOffset = FALSE;
	if( args.Length() == 2 ) {
		offset = args[1]->ToInteger()->Value();
		setOffset = TRUE;
	}
	if( args.Length() > 0 ) {
		if( args[0]->IsString() ) {
			String::Utf8Value data( args[0]->ToString() );
			size_t datalen = data.length();
			char *databuf = *data;
			size_t check;
			for( check = 0; check < datalen; check++ )
				if( !databuf[check] ) {
					lprintf( "Embedded NUL is unconverted." );
					// need to convert nul to \xc0\x80
					break;
				}
			if( file->vol->volNative ) {
				sack_fseek( file->cfile, offset, SEEK_SET );
				sack_vfs_write( file->file, *data, data.length() );
				sack_vfs_write( file->file, "\n", 1 );
			}
			else {
				if( setOffset )
					sack_fseek( file->cfile, offset, SEEK_SET );
				sack_fputs( *data, file->cfile );
				sack_fputs( "\n", file->cfile );
			}
		} else if( args[0]->IsArrayBuffer() ) {
			Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
			ArrayBuffer::Contents contents = ab->GetContents();
			//file->buf = (char*)contents.Data();
			//file->size = contents.ByteLength();
			lprintf( "Really should complain about binary data being passed to writeLine." );
			if( file->vol->volNative )
				sack_vfs_write( file->file, (char*)contents.Data(), contents.ByteLength() );
			else
				sack_fwrite( contents.Data(), contents.ByteLength(), 1, file->cfile );
		}
	}

}

void FileObject::truncateFile(const v8::FunctionCallbackInfo<Value>& args) {
	//Isolate* isolate = Isolate::GetCurrent();
	FileObject *file = ObjectWrap::Unwrap<FileObject>( args.This() );
	if( file->vol->volNative )
		sack_vfs_truncate( file->file ); // sets end of file mark to current position.
	else
		sack_ftruncate( file->cfile );
}

void FileObject::seekFile(const v8::FunctionCallbackInfo<Value>& args) {
#if  V8_AT_LEAST(5, 4)
//	Isolate* isolate = Isolate::GetCurrent();
	size_t num1 = (size_t)args[0]->ToNumber(Isolate::GetCurrent()->GetCurrentContext()).FromMaybe(Local<Number>())->Value();
#endif
	FileObject *file = ObjectWrap::Unwrap<FileObject>( args.This() );
	if( args.Length() == 1 && args[0]->IsNumber() ) {
		if( file->vol->volNative )
#if  V8_AT_LEAST(5, 4)
			sack_vfs_seek( file->file, num1, SEEK_SET );
#else
			sack_vfs_seek( file->file, (size_t)args[0]->ToNumber()->Value(), SEEK_SET );
#endif
		else
			sack_fseek( file->cfile, num1, SEEK_SET );
	}
	if( args.Length() == 2 && args[0]->IsNumber() && args[1]->IsNumber() ) {
#if  V8_AT_LEAST(5, 4)
		int num2 = (int)args[1]->ToNumber( Isolate::GetCurrent()->GetCurrentContext() ).FromMaybe( Local<Number>() )->Value();
#endif
		if( file->vol->volNative ) {
#if  V8_AT_LEAST(5, 4)
		sack_vfs_seek( file->file, num1, (int)num2 );
#else
			sack_vfs_seek( file->file, (size_t)args[0]->ToNumber()->Value(), (int)args[1]->ToNumber()->Value() );
#endif
		}
		else {
			sack_fseek( file->cfile, num1, (int)num2 );
		}
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
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "readLine", readLine );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "write", writeFile );
		NODE_SET_PROTOTYPE_METHOD( fileTemplate, "writeLine", writeLine );
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

	void FileObject::openFile( const v8::FunctionCallbackInfo<Value>& args ) {
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
		buffers = NULL;
		size = 0;
		buf = NULL;
		this->vol = vol;
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

