
#include "global.h"


//-----------------------------------------------------------
//   SQL Object
//-----------------------------------------------------------

void SqlObject::Init( Handle<Object> exports ) {
	OptionTreeObject::Init(); // SqlObject attached this

	Isolate* isolate = Isolate::GetCurrent();

	Local<FunctionTemplate> sqlTemplate;
	// Prepare constructor template
	sqlTemplate = FunctionTemplate::New( isolate, New );
	sqlTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.Sqlite" ) );
	sqlTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "do", query );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "escape", escape );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "end", closeDb );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "transaction", transact );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "commit", commit );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "autoTransact", autoTransact );

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

	Local<Object> sqlfunc = sqlTemplate->GetFunction();

	NODE_SET_METHOD(sqlfunc, "op", optionInternal );
	NODE_SET_METHOD(sqlfunc, "so", setOptionInternal );

	exports->Set( String::NewFromUtf8( isolate, "Sqlite" ),
		sqlfunc );
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
			obj = new SqlObject( dsn );
		}
		else {
			obj = new SqlObject( dsn );
		}
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	} else {
		const int argc = 2;
		Local<Value> argv[argc] = { args[0], args.Holder() };
		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( Nan::NewInstance( cons, argc, argv ).ToLocalChecked() );
	}
}

void SqlObject::doWrap( SqlObject *sql, Local<Object> o ) {
	sql->Wrap( o );
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
void SqlObject::closeDb( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	CloseDatabase( sql->odbc );
}

void SqlObject::autoTransact( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	SetSQLAutoTransact( sql->odbc, args[0]->BooleanValue() );
}
//-----------------------------------------------------------
void SqlObject::transact( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	SQLBeginTransact( sql->odbc );
}
//-----------------------------------------------------------
void SqlObject::commit( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	SQLCommit( sql->odbc );
}
//-----------------------------------------------------------

void SqlObject::escape( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	if( args[0]->IsUndefined() ) return; // undefined is still undefined
	if( args[0]->IsNull() ) { 
		args.GetReturnValue().Set( args[0] ); // undefined is still undefined
		return;
	}
	String::Utf8Value tmp( args[0] );
	char *out = EscapeSQLString(sql->odbc, (*tmp) );
	args.GetReturnValue().Set( String::NewFromUtf8( isolate, out ) );
	Deallocate( char*, out );

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
		SQLEndQuery( sql->odbc );
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
SqlObject::SqlObject( const char *dsn )
{
   odbc = ConnectToDatabase( dsn );
   SetSQLThreadProtect( odbc, FALSE );
   //SetSQLAutoClose( odbc, TRUE );
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
		args.GetReturnValue().Set(Nan::NewInstance( cons, 0, NULL ).ToLocalChecked());
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
	MaybeLocal<Object> o = Nan::NewInstance( cons, 0, NULL );
	args.GetReturnValue().Set( o.ToLocalChecked() );

	OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( o.ToLocalChecked() );
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
	args.GetReturnValue().Set( o = Nan::NewInstance( cons, 0, NULL ).ToLocalChecked() );

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
		args.GetReturnValue().Set( o = Nan::NewInstance( cons, 0, NULL ).ToLocalChecked() );

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
		args.GetReturnValue().Set( o = Nan::NewInstance( cons, 0, NULL ).ToLocalChecked() );

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
	o = Nan::NewInstance( cons, 0, NULL ).ToLocalChecked();

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
	int res = (int)GetOptionStringValueEx( oto->db->odbc, oto->node, &buffer, &buflen DBG_SRC );
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


static void option_( const FunctionCallbackInfo<Value>& args, int internal ) {
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
		defaultVal = StrDup( "" );

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
	PODBC use_odbc = NULL;
	if( internal ) {
	} else 
	{
		SqlObject *sql = SqlObject::Unwrap<SqlObject>( args.This() );
		sql->fields = 0;

		if( !sql->optionInitialized ) {
			SetOptionDatabaseOption( sql->odbc );
			sql->optionInitialized = TRUE;
		}
		use_odbc = sql->odbc;
	}
	SACK_GetPrivateProfileStringExxx( use_odbc
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

void SqlObject::option( const FunctionCallbackInfo<Value>& args ) {
	option_( args, 0 );
}

void SqlObject::optionInternal( const FunctionCallbackInfo<Value>& args ) {
	option_( args, 1 );
}

//-----------------------------------------------------------

static void setOption( const FunctionCallbackInfo<Value>& args, int internal ) {
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
		defaultVal = StrDup( "" );

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

	PODBC use_odbc = NULL;
	if( internal ) {
	} else 
	{
		SqlObject *sql = SqlObject::Unwrap<SqlObject>( args.This() );
		sql->fields = 0;
		use_odbc = sql->odbc;
	}

	SACK_WriteOptionString( use_odbc
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

void SqlObject::setOption( const FunctionCallbackInfo<Value>& args ) {
	::setOption( args, 0 );
}
void SqlObject::setOptionInternal( const FunctionCallbackInfo<Value>& args ) {
	::setOption( args, 1 );
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

