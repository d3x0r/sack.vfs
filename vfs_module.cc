
#include <node.h>
#include <nan.h>
#include <node_object_wrap.h>
#include <v8.h>
#include <uv.h>
//#include <nan.h>

#include "src/sack.h"
#undef New

using namespace v8;

#include "global.h"


static struct local {
	PLIST volumes;
} l;

class VolumeObject : public node::ObjectWrap {
public:
	struct volume *vol;
	static v8::Persistent<v8::Function> constructor;
	
public:

	static void Init( Handle<Object> exports );
	VolumeObject( const char *mount, const char *filename, const char *key, const char *key2 );

	static void New( const FunctionCallbackInfo<Value>& args );
	static void getDirectory( const FunctionCallbackInfo<Value>& args );

   ~VolumeObject();
};


class ThreadObject : public node::ObjectWrap {
public:
	static v8::Persistent<v8::Function> constructor;
   	static Persistent<Function, CopyablePersistentTraits<Function>> idleProc;
public:

	static void Init( Handle<Object> exports );
	ThreadObject();

	static void New( const FunctionCallbackInfo<Value>& args );

	static void relinquish( const FunctionCallbackInfo<Value>& args );
	static void wake( const FunctionCallbackInfo<Value>& args );

   ~ThreadObject();
};


class FileObject : public node::ObjectWrap {
	struct sack_vfs_file *file;
	//Local<Object> volume;
   char* buf;
   size_t size;
   Persistent<Object> volume;
public:
	static v8::Persistent<v8::Function> constructor;
	static void Init(  );

	static void openFile( const FunctionCallbackInfo<Value>& args );
	static void readFile( const FunctionCallbackInfo<Value>& args );
	static void writeFile( const FunctionCallbackInfo<Value>& args );
	static void seekFile( const FunctionCallbackInfo<Value>& args );
	static void truncateFile( const FunctionCallbackInfo<Value>& args );

	//static void readFile( const FunctionCallbackInfo<Value>& args );

	static void Emitter( const FunctionCallbackInfo<Value>& args );

	FileObject( VolumeObject* vol, const char *filename, Isolate*, Local<Object> o );
   ~FileObject();
};

static void moduleExit( void *arg ) {
	InvokeExits();
}

void VolumeObject::Init( Handle<Object> exports ) {
	InvokeDeadstart();
	node::AtExit( moduleExit );
	
	SetSystemLog( SYSLOG_FILE, stdout );
	lprintf( "Stdout Logging Enabled." );
	{
		//extern void Syslog
	}
		Isolate* isolate = Isolate::GetCurrent();
		Local<FunctionTemplate> volumeTemplate;
		ThreadObject::Init( exports );
		FileObject::Init();
		SqlObject::Init( exports );
		ComObject::Init( exports );
		RegObject::Init( exports );
		// Prepare constructor template
		volumeTemplate = FunctionTemplate::New( isolate, New );
		volumeTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.Volume" ) );
		volumeTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

		// Prototype
		NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "File", FileObject::openFile );
		NODE_SET_PROTOTYPE_METHOD( volumeTemplate, "dir", getDirectory );

		
		constructor.Reset( isolate, volumeTemplate->GetFunction() );
		exports->Set( String::NewFromUtf8( isolate, "Volume" ),
			volumeTemplate->GetFunction() );
	}

VolumeObject::VolumeObject( const char *mount, const char *filename, const char *key, const char *key2 )  {
	vol = sack_vfs_load_crypt_volume( filename, key, key2 );
	if( !vol )
		;// lprintf( "Failed to open volume: %s %s %s", filename, key, key2 );
	else
		sack_mount_filesystem( mount, sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME )
				, 2000, (uintptr_t)vol, TRUE );
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
			if( argc > 0 ) {
				String::Utf8Value fName( args[0]->ToString() );
				mount_name = StrDup( *fName );
			}
			if( argc > 1 ) {
				String::Utf8Value fName( args[1]->ToString() );
				defaultFilename = FALSE;
				filename = StrDup( *fName );
			}
			if( argc > 2 ) {
				String::Utf8Value k( args[2] );
				key = StrDup( *k );
			}
			if( argc > 3 ) {
				String::Utf8Value k( args[3] );
				key2 = StrDup( *k );
			}
			// Invoked as constructor: `new MyObject(...)`
			VolumeObject* obj = new VolumeObject( mount_name, filename, key, key2 );
			if( !obj->vol ) {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, "Volume failed to open." ) ) );
			}
			else {
				obj->Wrap( args.This() );
				args.GetReturnValue().Set( args.This() );
			}

			Deallocate( char*, mount_name );
			if( !defaultFilename )
				Deallocate( char*, filename );
			Deallocate( char*, key );
			Deallocate( char*, key2 );
		}
		else {
			// Invoked as plain function `MyObject(...)`, turn into construct call.
			int argc = args.Length();
			Local<Value> *argv = new Local<Value>[argc];
			for( int n = 0; n < argc; n++ )
				argv[n] = args[n];

			Local<Function> cons = Local<Function>::New( isolate, constructor );
			args.GetReturnValue().Set( Nan::NewInstance( cons, argc, argv ).ToLocalChecked() );
			delete argv;
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
			args.GetReturnValue().Set( arrayBuffer );
		}
		//Local<String> result = String::NewFromUtf8( isolate, buf );
		//args.GetReturnValue().Set( result );
	}
}

void FileObject::writeFile(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
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
	Isolate* isolate = Isolate::GetCurrent();
	FileObject *file = ObjectWrap::Unwrap<FileObject>( args.This() );
	sack_vfs_truncate( file->file ); // sets end of file mark to current position.
}

void FileObject::seekFile(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	FileObject *file = ObjectWrap::Unwrap<FileObject>( args.This() );
	if( args.Length() == 1 && args[0]->IsNumber() ) {
		sack_vfs_seek( file->file, (size_t)args[0]->ToNumber()->Value(), SEEK_SET );
	}
	if( args.Length() == 2 && args[0]->IsNumber() && args[1]->IsNumber() ) {
		sack_vfs_seek( file->file, (size_t)args[0]->ToNumber()->Value(), (int)args[1]->ToNumber()->Value() );
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
		if( !vol->vol )
			return;
		file = sack_vfs_openfile( vol->vol, filename );
	}


Persistent<Function> VolumeObject::constructor;
Persistent<Function> FileObject::constructor;

VolumeObject::~VolumeObject() {
	//printf( "Volume object evaporated.\n" );
	sack_vfs_unload_volume( vol );
}

FileObject::~FileObject() {
	//printf( "File object evaporated.\n" );
	if( buf )
      Deallocate( char*, buf );
	sack_vfs_close( file );
	volume.Reset();
}


//-----------------------------------------------------------


void ThreadObject::Init( Handle<Object> exports ) {
	Isolate* isolate = Isolate::GetCurrent();

	NODE_SET_METHOD(exports, "Δ", relinquish );
	NODE_SET_METHOD(exports, "Λ", wake );
	Local<FunctionTemplate> threadTemplate;
	// Prepare constructor template
	threadTemplate = FunctionTemplate::New( isolate, New );
	threadTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.Thread" ) );
	threadTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap

	// Prototype

	constructor.Reset( isolate, threadTemplate->GetFunction() );
	exports->Set( String::NewFromUtf8( isolate, "Thread" ),
		threadTemplate->GetFunction() );
}

//-----------------------------------------------------------
Persistent<Function, CopyablePersistentTraits<Function>> ThreadObject::idleProc;

void ThreadObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() ) {
		if( idleProc.IsEmpty() ) {
			Handle<Function> arg0 = Handle<Function>::Cast( args[0] );
			Persistent<Function> cb( isolate, arg0 );
			idleProc = cb;
		}
	}
	else
		idleProc.Reset();
}
//-----------------------------------------------------------

static bool cbWoke;

void ThreadObject::wake( const FunctionCallbackInfo<Value>& args ) {
	cbWoke = true;
}

//-----------------------------------------------------------
void ThreadObject::relinquish( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = Isolate::GetCurrent();
	//int delay = 0;
	//if( args.Length() > 0 && args[0]->IsNumber() )
	//	delay = (int)args[0]->ToNumber()->Value();

	//	nodeThread = MakeThread();
	Local<Function>cb = Local<Function>::New( isolate, idleProc );
	/*Local<Value> r = */cb->Call(Null(isolate), 0, NULL );
	// r was always undefined.... so inner must wake.
	//String::Utf8Value fName( r->ToString() );
	//lprintf( "tick callback resulted %s", (char*)*fName);
	if( !cbWoke )
		if( uv_run( uv_default_loop(), UV_RUN_NOWAIT ) )
			uv_run( uv_default_loop(), UV_RUN_ONCE);
	cbWoke = false;
	/*
	if( delay ) {

		lprintf( "short sleep", delay, delay );
		WakeableSleep( 20 );
		lprintf( "short wake", delay, delay );
		cb->Call(Null(isolate), 0, NULL );
		uv_run( uv_default_loop(), UV_RUN_DEFAULT);
	
		lprintf( "sleep for %08x, %d", delay, delay );
		WakeableSleep( delay );
	}
	cb->Call(Null(isolate), 0, NULL );
	uv_run( uv_default_loop(), UV_RUN_DEFAULT);
	*/
}

//-----------------------------------------------------------

Persistent<Function> ThreadObject::constructor;
ThreadObject::ThreadObject( )
{
}

ThreadObject::~ThreadObject() {
	lprintf( "no thread object..." );
}

//-----------------------------------------------------------


NODE_MODULE( vfs_module, VolumeObject::Init)

