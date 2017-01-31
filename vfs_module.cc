
#include <node.h>
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


class SqlObject : public node::ObjectWrap {
public:
	PODBC odbc;
   	int optionInitialized;
	static v8::Persistent<v8::Function> constructor;
	int columns;
	CTEXTSTR *result;
	CTEXTSTR *fields;
	//Persistent<Object> volume;
public:

	static void Init( Handle<Object> exports );
	SqlObject( const char *dsn, Isolate* isolate, Local<Object> o );

	static void New( const FunctionCallbackInfo<Value>& args );
	static void query( const FunctionCallbackInfo<Value>& args );
	static void option( const FunctionCallbackInfo<Value>& args );
	static void setOption( const FunctionCallbackInfo<Value>& args );
	static void makeTable( const FunctionCallbackInfo<Value>& args );

	static void enumOptionNodes( const FunctionCallbackInfo<Value>& args );
	static void findOptionNode( const FunctionCallbackInfo<Value>& args );
	static void getOptionNode( const FunctionCallbackInfo<Value>& args );

   ~SqlObject();
};

class OptionTreeObject : public node::ObjectWrap {
public:
	POPTION_TREE_NODE node;
	SqlObject *db;
	static v8::Persistent<v8::Function> constructor;
	
public:

	static void Init( );
	OptionTreeObject(  );

	static void New( const FunctionCallbackInfo<Value>& args );

	static void enumOptionNodes( const FunctionCallbackInfo<Value>& args );
	static void findOptionNode( const FunctionCallbackInfo<Value>& args );
	static void getOptionNode( const FunctionCallbackInfo<Value>& args );
	static void writeOptionNode( v8::Local<v8::String> field,
		v8::Local<v8::Value> val,
		const PropertyCallbackInfo<void>&info );
	static void readOptionNode( v8::Local<v8::String> field,
		const PropertyCallbackInfo<v8::Value>& info );

   ~OptionTreeObject();
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
		OptionTreeObject::Init();
		SqlObject::Init( exports );
		ComObject::Init( exports );
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
			char *filename = "default.vfs";
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
			args.GetReturnValue().Set( cons->NewInstance( argc, argv ) );
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
			args.GetReturnValue().Set( cons->NewInstance( argc, argv ) );
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
//   SQL Object
//-----------------------------------------------------------


void SqlObject::Init( Handle<Object> exports ) {
	Isolate* isolate = Isolate::GetCurrent();

	Local<FunctionTemplate> sqlTemplate;
	// Prepare constructor template
	sqlTemplate = FunctionTemplate::New( isolate, New );
	sqlTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.Sqlite" ) );
	sqlTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "do", query );

   // read a portion of the tree (passed to a callback)
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "eo", enumOptionNodes );
   // get without create
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "fo", findOptionNode );
   // get the node.
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "go", getOptionNode );
   // update the value from option node
	//NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "wo", writeOptionNode );
   // read value from the option node
	//NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "ro", readOptionNode );


	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "op", option );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "getOption", option );
	//NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "so", setOption );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "so", setOption );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "setOption", setOption );
	//NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "makeTable", makeTable );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "makeTable", makeTable );
	constructor.Reset( isolate, sqlTemplate->GetFunction() );
	exports->Set( String::NewFromUtf8( isolate, "Sqlite" ),
		sqlTemplate->GetFunction() );
}

//-----------------------------------------------------------

void SqlObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		char *dsn;
		String::Utf8Value arg( args[0] );
		dsn = *arg;
		SqlObject* obj;
		if( args.Length() < 2 ) {
			obj = new SqlObject( dsn, isolate, args.Holder() );
		}
		else {
			obj = new SqlObject( dsn, isolate, args[1]->ToObject() );
		}
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	} else {
		const int argc = 2;
		Local<Value> argv[argc] = { args[0], args.Holder() };
		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( argc, argv ) );
	}
}

//-----------------------------------------------------------

int IsTextAnyNumber( CTEXTSTR text, double *fNumber, int64_t *iNumber )
{
	CTEXTSTR pCurrentCharacter;
	int decimal_count, s, begin = TRUE, digits;
	// remember where we started...
	// if the first segment is indirect, collect it and only it
	// as the number... making indirects within a number what then?

	decimal_count = 0;
	s = 0;
	digits = 0;
	pCurrentCharacter = text;
	while( pCurrentCharacter[0] )
	{
		pCurrentCharacter = text;
		while( pCurrentCharacter && *pCurrentCharacter )
		{
			if( *pCurrentCharacter == '.' )
			{
				if( !decimal_count && digits )
					decimal_count++;
				else
					break;
			}
			else if( ((*pCurrentCharacter) == '-') && begin)
			{
				s++;
			}
			else if( ((*pCurrentCharacter) < '0') || ((*pCurrentCharacter) > '9') )
			{
				break;
			}
			else {
				digits++;
				if( !decimal_count && digits > 11 )
               return 0;
			}
			begin = FALSE;
			pCurrentCharacter++;
		}
		// invalid character - stop, we're to abort.
		if( *pCurrentCharacter )
			break;

		//while( pText );
	}

	if( *pCurrentCharacter || ( decimal_count > 1 ) || !digits )
	{
		// didn't collect enough meaningful info to be a number..
		// or information in this state is
		return 0;
	}
	if( decimal_count == 1 )
	{
		if( fNumber )
			(*fNumber) = FloatCreateFromText( text, NULL );
		// return specifically it's a floating point number
		return 2;
	}
	if( iNumber )
		(*iNumber) = IntCreateFromText( text );
	// return yes, and it's an int number
	return 1;
}
//-----------------------------------------------------------


void SqlObject::query( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	char *query;
	String::Utf8Value tmp( args[0] );
	query = StrDup( *tmp );


	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	sql->fields = 0;
	if( !SQLRecordQuery( sql->odbc, query, &sql->columns, &sql->result, &sql->fields ) ) {
		const char *error;
		FetchSQLError( sql->odbc, &error );
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, error ) ) );
		return;
	}
	if( sql->columns )
	{
		Local<Array> records = Array::New( isolate );
		Local<Object> record = Object::New( isolate );
		if( sql->result ) {
			int row = 0;
			do {
				record = Object::New( isolate );
				for( int n = 0; n < sql->columns; n++ ) {
					if( sql->result[n] ) {
						double f;
						int64_t i;
						int type = IsTextAnyNumber( sql->result[n], &f, &i );
						if( type == 2 )
							record->Set( String::NewFromUtf8( isolate, sql->fields[n] )
										  , Number::New( isolate, f )
										  );
						else if( type == 1 )
							record->Set( String::NewFromUtf8( isolate, sql->fields[n] )
										  , Number::New( isolate, (double)i )
										  );
						else
							record->Set( String::NewFromUtf8( isolate, sql->fields[n] )
										  , String::NewFromUtf8( isolate, sql->result[n] )
										  );
					}
					else
						record->Set( String::NewFromUtf8( isolate, sql->fields[n] )
									  , Null(isolate)
									  );
				}
				records->Set( row++, record );
			} while( FetchSQLRecord( sql->odbc, &sql->result ) );
		}
		args.GetReturnValue().Set( records );
	}
	else
	{
		SQLEndQuery( sql->odbc );
		args.GetReturnValue().Set( True( isolate ) );
	}
	Deallocate( char*, query );
}

//-----------------------------------------------------------


Persistent<Function> SqlObject::constructor;
SqlObject::SqlObject( const char *dsn, Isolate* isolate, Local<Object> o )
{
   odbc = ConnectToDatabase( dsn );
   optionInitialized = FALSE;
}

SqlObject::~SqlObject() {
	CloseDatabase( odbc );
}

//-----------------------------------------------------------

Persistent<Function> OptionTreeObject::constructor;
OptionTreeObject::OptionTreeObject()  {
}

OptionTreeObject::~OptionTreeObject() {
}

void OptionTreeObject::New(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (args.IsConstructCall()) {
		// Invoked as constructor: `new MyObject(...)`

		//double value = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
		OptionTreeObject* obj = new OptionTreeObject();
		//lprintf( "Wrap a new OTO %p in %p", obj, args.This()->ToObject() );
		obj->Wrap(args.This());
		args.GetReturnValue().Set(args.This());
	} else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Function> cons = Local<Function>::New(isolate, constructor);
		Local<Object> instance = cons->NewInstance( 0, NULL );
		//cons->NewInstance(context, argc, argv).ToLocalChecked();
		args.GetReturnValue().Set(instance);
	}
}


void OptionTreeObject::Init(  ) {
	Isolate* isolate = Isolate::GetCurrent();

	Local<FunctionTemplate> optionTemplate;
	// Prepare constructor template
	optionTemplate = FunctionTemplate::New( isolate, New );
	optionTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.option.node" ) );
	optionTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( optionTemplate, "eo", enumOptionNodes );
	NODE_SET_PROTOTYPE_METHOD( optionTemplate, "fo", findOptionNode );
	NODE_SET_PROTOTYPE_METHOD( optionTemplate, "go", getOptionNode );
	Local<Template> proto = optionTemplate->InstanceTemplate();

	proto->SetNativeDataProperty( String::NewFromUtf8( isolate, "value" )
			, readOptionNode
			, writeOptionNode );

	//NODE_SET_PROTOTYPE_METHOD( optionTemplate, "ro", readOptionNode );
	//NODE_SET_PROTOTYPE_METHOD( optionTemplate, "wo", writeOptionNode );

	constructor.Reset( isolate, optionTemplate->GetFunction() );
}


void SqlObject::getOptionNode( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();

	if( argc < 1 ) {
		return;
	}

	SqlObject *sqlParent = ObjectWrap::Unwrap<SqlObject>( args.This() );
	if( !sqlParent->optionInitialized ) {
		SetOptionDatabaseOption( sqlParent->odbc );
		sqlParent->optionInitialized = TRUE;
	}

	String::Utf8Value tmp( args[0] );
	char *optionPath = StrDup( *tmp );

	Local<Function> cons = Local<Function>::New( isolate, OptionTreeObject::constructor );
	Local<Object> o;
	args.GetReturnValue().Set( o = cons->NewInstance( 0, NULL ) );

	OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( o );
	oto->db = sqlParent;
	//lprintf( "SO Get %p ", sqlParent->odbc );
	oto->node =  GetOptionIndexExx( sqlParent->odbc, NULL, optionPath, NULL, NULL, NULL, TRUE, TRUE DBG_SRC );
}


void OptionTreeObject::getOptionNode( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();

	if( argc < 1 ) {
		return;
	}

	OptionTreeObject *parent = ObjectWrap::Unwrap<OptionTreeObject>( args.Holder() );

	String::Utf8Value tmp( args[0] );
	char *optionPath = StrDup( *tmp );

	Local<Function> cons = Local<Function>::New( isolate, constructor );
	Local<Object> o;
	//lprintf( "objecttreeobject constructor..." );
	args.GetReturnValue().Set( o = cons->NewInstance( 0, NULL ) );

	OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( o );
	oto->db = parent->db;
	//lprintf( "OTO Get %p  %p", parent->db->odbc, parent->node );
	oto->node =  GetOptionIndexExx( parent->db->odbc, parent->node, optionPath, NULL, NULL, NULL, TRUE, TRUE DBG_SRC );
	//lprintf( "result node: %s %p %p", optionPath, oto->node, o );
	Release( optionPath );
}

void SqlObject::findOptionNode( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();

	if( argc < 1 ) {
		return;
	}
	String::Utf8Value tmp( args[0] );
	char *optionPath = StrDup( *tmp );
	SqlObject *sqlParent = ObjectWrap::Unwrap<SqlObject>( args.This() );
	if( !sqlParent->optionInitialized ) {
		SetOptionDatabaseOption( sqlParent->odbc );
		sqlParent->optionInitialized = TRUE;
	}
	POPTION_TREE_NODE newNode = GetOptionIndexExx( sqlParent->odbc, NULL, optionPath, NULL, NULL, NULL, FALSE, TRUE DBG_SRC );

	if( newNode ) {
		Local<Function> cons = Local<Function>::New( isolate, OptionTreeObject::constructor );
		Local<Object> o;
		args.GetReturnValue().Set( o = cons->NewInstance( 0, NULL ) );

		OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( o );
		oto->db = sqlParent;
		oto->node = newNode;
	}
	Release( optionPath );
	args.GetReturnValue().Set( Null(isolate) );
}


void OptionTreeObject::findOptionNode( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();

	if( argc < 1 ) {
		return;
	}

	POPTION_TREE_NODE newOption;
	OptionTreeObject *parent = ObjectWrap::Unwrap<OptionTreeObject>( args.This() );

	String::Utf8Value tmp( args[0] );
	char *optionPath = StrDup( *tmp );
	newOption = GetOptionIndexExx( parent->db->odbc, parent->node, optionPath, NULL, NULL, NULL, FALSE, TRUE DBG_SRC );
	if( newOption ) {
		Local<Function> cons = Local<Function>::New( isolate, constructor );
		Local<Object> o;
		args.GetReturnValue().Set( o = cons->NewInstance( 0, NULL ) );

		OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( o );
		oto->db = parent->db;
		oto->node = newOption;
	}
	Release( optionPath );
	args.GetReturnValue().Set( Null( isolate ) );
}

struct enumArgs {
	Local<Function>cb;
	Isolate *isolate;
	SqlObject *db;
};

int CPROC invokeCallback( uintptr_t psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags ) {
	struct enumArgs *args = (struct enumArgs*)psv;
	Local<Value> argv[2];

	Local<Function> cons = Local<Function>::New( args->isolate, OptionTreeObject::constructor );
	Local<Object> o;
	o = cons->NewInstance( 0, NULL );

	OptionTreeObject *oto = OptionTreeObject::Unwrap<OptionTreeObject>( o );
	oto->db = args->db;
	oto->node = ID;

	argv[0] = o;
	argv[1] = String::NewFromUtf8( args->isolate, name );

	/*Local<Value> r = */args->cb->Call(Null(args->isolate), 2, argv );
	return 1;
}


void SqlObject::enumOptionNodes( const FunctionCallbackInfo<Value>& args ) {
	struct enumArgs callbackArgs;
	callbackArgs.isolate = args.GetIsolate();

	int argc = args.Length();
	if( argc < 1 ) {
		return;
	}
	
	Isolate* isolate = args.GetIsolate();
	SqlObject *sqlParent = ObjectWrap::Unwrap<SqlObject>( args.This() );

	if( !sqlParent->optionInitialized ) {
		SetOptionDatabaseOption( sqlParent->odbc );
		sqlParent->optionInitialized = TRUE;
	}

	Handle<Function> arg0 = Handle<Function>::Cast( args[0] );
	Local<Function> cb( arg0 );

	callbackArgs.db = sqlParent;
	callbackArgs.cb = Local<Function>::New( isolate, cb );
	callbackArgs.isolate = isolate;

	EnumOptionsEx( sqlParent->odbc, NULL, invokeCallback, (uintptr_t)&callbackArgs );
}

void OptionTreeObject::enumOptionNodes( const FunctionCallbackInfo<Value>& args ) {
	struct enumArgs callbackArgs;
	callbackArgs.isolate = args.GetIsolate();

	int argc = args.Length();
	if( argc < 1 ) {
		return;
	}

	
	Isolate* isolate = args.GetIsolate();
	OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( args.This() );
	Handle<Function> arg0 = Handle<Function>::Cast( args[0] );
	Local<Function> cb( arg0 );

	callbackArgs.db = oto->db;
	callbackArgs.cb = Local<Function>::New( isolate, cb );
	callbackArgs.isolate = isolate;

	EnumOptionsEx( oto->db->odbc, oto->node, invokeCallback, (uintptr_t)&callbackArgs );
}

void OptionTreeObject::readOptionNode( v8::Local<v8::String> field,
                              const PropertyCallbackInfo<v8::Value>& info ) {
	OptionTreeObject* oto = node::ObjectWrap::Unwrap<OptionTreeObject>( info.This() );
	char *buffer;
	size_t buflen;
	size_t res = GetOptionStringValueEx( oto->db->odbc, oto->node, &buffer, &buflen DBG_SRC );
	if( !buffer || res < 0 )
		return;
	info.GetReturnValue().Set( String::NewFromUtf8( info.GetIsolate(), buffer ) );
}

void OptionTreeObject::writeOptionNode( v8::Local<v8::String> field,
                              v8::Local<v8::Value> val,
                              const PropertyCallbackInfo<void>&info ) {
	String::Utf8Value tmp( val );
	OptionTreeObject* oto = node::ObjectWrap::Unwrap<OptionTreeObject>( info.Holder() );
	SetOptionStringValueEx( oto->db->odbc, oto->node, *tmp );
}


void SqlObject::option( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();
	char *sect;
	char *optname;
	char *defaultVal;

	if( argc > 0 ) {
		String::Utf8Value tmp( args[0] );
		defaultVal = StrDup( *tmp );
	}
	else
		defaultVal = "";

	if( argc > 1 ) {
		String::Utf8Value tmp( args[1] );
		sect = defaultVal;
		defaultVal = StrDup( *tmp );
	}
	else
		sect = NULL;

	if( argc > 2 ) {
		String::Utf8Value tmp( args[2] );
		optname = defaultVal;
		defaultVal = StrDup( *tmp );
	}
	else
		optname = NULL;

	TEXTCHAR readbuf[1024];

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	sql->fields = 0;

	if( !sql->optionInitialized ) {
		SetOptionDatabaseOption( sql->odbc );
      sql->optionInitialized = TRUE;
	}

	SACK_GetPrivateProfileStringExxx( sql->odbc
		, sect
		, optname
		, defaultVal
		, readbuf
		, 1024
		, NULL
		, TRUE
		DBG_SRC
		);

	Local<String> returnval = String::NewFromUtf8( isolate, readbuf );
	args.GetReturnValue().Set( returnval );

	Deallocate( char*, optname );
	Deallocate( char*, sect );
	Deallocate( char*, defaultVal );
}

//-----------------------------------------------------------

void SqlObject::setOption( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();
	char *sect;
	char *optname;
	char *defaultVal;

	if( argc > 0 ) {
		String::Utf8Value tmp( args[0] );
		defaultVal = StrDup( *tmp );
	}
	else
		defaultVal = "";

	if( argc > 1 ) {
		String::Utf8Value tmp( args[1] );
		sect = defaultVal;
		defaultVal = StrDup( *tmp );
	}
	else
		sect = NULL;

	if( argc > 2 ) {
		String::Utf8Value tmp( args[2] );
		optname = defaultVal;
		defaultVal = StrDup( *tmp );
	}
	else
		optname = NULL;

	TEXTCHAR readbuf[1024];

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	sql->fields = 0;

	SACK_WriteOptionString( sql->odbc
		, sect
		, optname
		, defaultVal
	);

	Local<String> returnval = String::NewFromUtf8( isolate, readbuf );
	args.GetReturnValue().Set( returnval );

	Deallocate( char*, optname );
	Deallocate( char*, sect );
	Deallocate( char*, defaultVal );
}

//-----------------------------------------------------------

void SqlObject::makeTable( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();
	char *tableCommand;

	if( argc > 0 ) {
		PTABLE table;
		String::Utf8Value tmp( args[0] );
		tableCommand = StrDup( *tmp );

		SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );

		table = GetFieldsInSQLEx( tableCommand, false DBG_SRC );
		if( CheckODBCTable( sql->odbc, table, CTO_MERGE ) )

			args.GetReturnValue().Set( True(isolate) );
		args.GetReturnValue().Set( False( isolate ) );

	}
	//args.GetReturnValue().Set( returnval );
	args.GetReturnValue().Set( False( isolate ) );
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

