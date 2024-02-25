
#include "global.h"

//#define DEBUG_EVENTS 
//#define EVENT_DEBUG_STDOUT

#ifdef EVENT_DEBUG_STDOUT
#  undef lprintf
#  define lprintf(f,...)  printf( "%d~" f "\n", (int)(GetThisThreadID() & 0x7FFFFFF),##__VA_ARGS__)
#endif


#ifdef INCLUDE_GUI
void editOptions( const v8::FunctionCallbackInfo<Value>& args );
#endif

static Local<Function> emptyFunction;

struct SqlObjectUserFunction {
	class SqlObject *sql;
	Persistent<Function> cb;
	Persistent<Function> cb2;
	Isolate *isolate;
	SqlObjectUserFunction() : cb(), cb2() {}
};

class SqlStmtObject : public node::ObjectWrap {
public:
//	static v8::Persistent<v8::Function> constructor;
	class SqlObject *sql;
	PDATALIST values;
	SqlStmtObject() {
		values = NULL;
	}
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void Set( const v8::FunctionCallbackInfo<Value>& args );
};

struct sql_object_state {
	PODBC odbc;
	int optionInitialized;
	PTHREAD thread;
	Isolate *isolate; // this is constant for the life of the connection
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLIST userFunctions;
	PLINKQUEUE messages;
	class SqlObject *sql;
};

//#define optionInitialized  state->optionInitialized
//#define userFunctions      state->userFunctions

class SqlObject : public node::ObjectWrap {
public:
	struct sql_object_state *state;

	/*
	PODBC odbc;
	int optionInitialized;
	PTHREAD thread;
	Isolate *isolate; // this is constant for the life of the connection
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLIST userFunctions;
	PLINKQUEUE messages;
	*/

	//static void Init( Local<Object> exports );
	SqlObject( const char *dsn, Isolate *isolate, Local<Object>, Local<Function> openCallback );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void query( const v8::FunctionCallbackInfo<Value>& args );
	static void promisedQuery( const v8::FunctionCallbackInfo<Value>& args );
	static void escape( const v8::FunctionCallbackInfo<Value>& args );
	static void unescape( const v8::FunctionCallbackInfo<Value>& args );
	static void option( const v8::FunctionCallbackInfo<Value>& args );
	static void setOption( const v8::FunctionCallbackInfo<Value>& args );
	static void optionInternal( const v8::FunctionCallbackInfo<Value>& args );
	static void setOptionInternal( const v8::FunctionCallbackInfo<Value>& args );
	static void makeTable( const v8::FunctionCallbackInfo<Value>& args );
	static void closeDb( const v8::FunctionCallbackInfo<Value>& args );
	static void commit( const v8::FunctionCallbackInfo<Value>& args );
	static void transact( const v8::FunctionCallbackInfo<Value>& args );
	static void autoTransact( const v8::FunctionCallbackInfo<Value>& args );
	static void userFunction( const v8::FunctionCallbackInfo<Value>& args );
	static void userProcedure( const v8::FunctionCallbackInfo<Value>& args );
	static void aggregateFunction( const v8::FunctionCallbackInfo<Value>& args );
	static void setOnCorruption( const v8::FunctionCallbackInfo<Value>& args );

	static void enumOptionNodes( const v8::FunctionCallbackInfo<Value>& args );
	static void enumOptionNodesInternal( const v8::FunctionCallbackInfo<Value>& args );
	static void findOptionNode( const v8::FunctionCallbackInfo<Value>& args );
	static void getOptionNode( const v8::FunctionCallbackInfo<Value>& args );
	static void error( const v8::FunctionCallbackInfo<Value>& args );
	static void getProvider( const v8::FunctionCallbackInfo<Value>& args );

	static void getLogging( const v8::FunctionCallbackInfo<Value>& args );
	static void setLogging( const v8::FunctionCallbackInfo<Value>& args );

	static void getRequire( const v8::FunctionCallbackInfo<Value>& args );
	static void setRequire( const v8::FunctionCallbackInfo<Value>& args );
	static void OnOpen( uintptr_t psv, PODBC odbc );
	Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	v8::Persistent<v8::Function> onCorruption;
	Persistent<Object> _this;

	static void doWrap( SqlObject *sql, Local<Object> o );

	~SqlObject();
};


class OptionTreeObject : public node::ObjectWrap {
public:
	POPTION_TREE_NODE node;
	PODBC odbc;
	//static v8::Persistent<v8::Function> constructor;

public:

	static void Init();
	OptionTreeObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void enumOptionNodes( const v8::FunctionCallbackInfo<Value>& args );
	static void findOptionNode( const v8::FunctionCallbackInfo<Value>& args );
	static void getOptionNode( const v8::FunctionCallbackInfo<Value>& args );
	static void writeOptionNode( v8::Local<v8::String> field,
		v8::Local<v8::Value> val,
		const PropertyCallbackInfo<void>&info );
	static void readOptionNode( v8::Local<v8::String> field,
		const PropertyCallbackInfo<v8::Value>& info );

	~OptionTreeObject();
};

enum UserMessageModes {
	OnOpen,  // open callback event
	OnClose, // close connection event
	Query, // async (promised) query
	OnDeallocate, // used to also be same as close
	OnSqliteFunction,
	OnSqliteAggStep,
	OnSqliteAggFinal,
};

struct query_thread_params {
	Isolate* isolate;
	SqlObject* sql;
	PTEXT statement;
	PDATALIST pdlParams;
	Local<Context> context;
	Persistent<Promise::Resolver> promise;
	PDATALIST pdlRecord;
	Local<Array> results;
	TEXTSTR error;
	//PTHREAD waiter;
};

struct userMessage{
	enum UserMessageModes mode;

	// really a union of this and all of the following
	struct query_thread_params* params;

	// state variables for a function/aggregate callback into JS from sqlite
	struct sqlite3_context*onwhat;
	int argc;
	struct sqlite3_value**argv;
	int done;
	PTHREAD waiter;
};

static void sqlUserAsyncMsgEx( uv_async_t* handle, LOGICAL internal );
static void sqlUserAsyncMsg( uv_async_t* handle );

/* This is used from external code... */
void createSqlObject( const char *name, Isolate *isolate, Local<Object> into ) {
	class SqlObject* obj;
	obj = new SqlObject( name, isolate, into, emptyFunction );
	//SqlObject::doWrap( obj, into );
}


Local<Value> newSqlObject(Isolate *isolate, int argc, Local<Value> *argv ) {
	class constructorSet *c = getConstructors( isolate );
	Local<Function> cons = Local<Function>::New( isolate, c->sqlConstructor );
	MaybeLocal<Object> mo = cons->NewInstance( isolate->GetCurrentContext(), argc, argv );
	if( !mo.IsEmpty() )
		return mo.ToLocalChecked();
	return Null( isolate );
}

//-----------------------------------------------------------
//   SQL Object
//-----------------------------------------------------------

void SqlObjectInit( Local<Object> exports ) {
	OptionTreeObject::Init(); // SqlObject attached this

	Isolate* isolate = Isolate::GetCurrent();
	class constructorSet *c = getConstructors( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> sqlTemplate;
	// Prepare constructor template
	sqlTemplate = FunctionTemplate::New( isolate, SqlObject::New );
	sqlTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.vfs.Sqlite" ) );
	sqlTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap

	Local<FunctionTemplate> sqlStmtTemplate;
	// Prepare constructor template
	sqlStmtTemplate = FunctionTemplate::New( isolate, SqlStmtObject::New );
	sqlStmtTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.vfs.Sqlite.statement" ) );
	sqlStmtTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
	c->sqlStmtConstructor.Reset( isolate, sqlStmtTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "do", SqlObject::query );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "run", SqlObject::promisedQuery );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "escape", SqlObject::escape );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "unescape", SqlObject::unescape );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "encode", SqlObject::escape );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "decode", SqlObject::unescape );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "end", SqlObject::closeDb );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "close", SqlObject::closeDb );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "transaction", SqlObject::transact );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "commit", SqlObject::commit );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "autoTransact", SqlObject::autoTransact );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "procedure", SqlObject::userProcedure );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "function", SqlObject::userFunction );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "aggregate", SqlObject::aggregateFunction );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "onCorruption", SqlObject::setOnCorruption );

	sqlTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "require" )
			, FunctionTemplate::New( isolate, SqlObject::getRequire )
			, FunctionTemplate::New( isolate, SqlObject::setRequire )
		);


	// read a portion of the tree (passed to a callback)
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "eo", SqlObject::enumOptionNodes );
	// get without create
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "fo", SqlObject::findOptionNode );
	// get the node.
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "go", SqlObject::getOptionNode );

	sqlTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "provider" )
		, FunctionTemplate::New( isolate, SqlObject::getProvider )
		, Local<FunctionTemplate>() );
	sqlTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "error" )
			, FunctionTemplate::New( isolate, SqlObject::error )
			, Local<FunctionTemplate>() );
	sqlTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "log" )
		, FunctionTemplate::New( isolate, SqlObject::getLogging )
		, FunctionTemplate::New( isolate, SqlObject::setLogging ) );

	// update the value from option node
	//NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "wo", writeOptionNode );
	// read value from the option node
	//NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "ro", readOptionNode );


	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "op", SqlObject::option );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "getOption", SqlObject::option );
	//NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "so", setOption );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "so", SqlObject::setOption );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "setOption", SqlObject::setOption );
	//NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "makeTable", makeTable );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "makeTable", SqlObject::makeTable );
	c->sqlConstructor.Reset( isolate, sqlTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

	Local<Object> sqlfunc = sqlTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();

	SET_READONLY_METHOD(sqlfunc, "eo", SqlObject::enumOptionNodesInternal );
	SET_READONLY_METHOD(sqlfunc, "op", SqlObject::optionInternal );
	SET_READONLY_METHOD(sqlfunc, "so", SqlObject::setOptionInternal );
#ifdef INCLUDE_GUI
	SET_READONLY_METHOD(sqlfunc, "optionEditor", editOptions );
#endif

	SET( exports, "Sqlite", sqlfunc );
	SET( exports, "ODBC", sqlfunc );
	SET( exports, "DB", sqlfunc );
}

//-----------------------------------------------------------

void SqlObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		char *dsn;
		SqlObject* obj;
		if( args.Length() > 0 ) {
			if( args[0]->IsString() ) {
				String::Utf8Value arg( USE_ISOLATE( isolate ) args[0] );
				Local<Function> callback = ( args.Length()>1 )?Local<Function>::Cast( args[1] ):emptyFunction;
				dsn = *arg;
				obj = new SqlObject( dsn, isolate, args.This(), callback );
			} else if( args[0]->IsObject() ){
				Local<Object> opts = Local<Object>::Cast( args[0] );
				lprintf( "option object method for opening a database connection is not complete!");
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "option object method for opening a database connection is not complete!" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
				return;
			} else {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "First argument to Sqlite() must be an object or string." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
				return;
			}
		}
		else {
			obj = new SqlObject( ":memory:", NULL, args.This(), emptyFunction );
		}
		args.GetReturnValue().Set( args.This() );
	} else {
		Local<Value> argv[2];
		if( args.Length() > 0 )
			argv[0] = args[0];
		if( args.Length() > 1 )
			argv[1] = args[1];
		class constructorSet *c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->sqlConstructor );
		MaybeLocal<Object> result = cons->NewInstance(isolate->GetCurrentContext(), args.Length(), argv);
		if( !result.IsEmpty() )
			args.GetReturnValue().Set( result.ToLocalChecked() );
		else
			isolate->ThrowException(Exception::Error(
				String::NewFromUtf8(isolate, TranslateText("Database Constructor callback failed"), v8::NewStringType::kNormal).ToLocalChecked()));
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
void SqlObject::closeDb( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	CloseDatabase( sql->state->odbc );
	if( sql->state->thread ) {
		static struct userMessage msg;
		msg.mode = OnClose;
		EnqueLink( &sql->state->messages, &msg );
#ifdef DEBUG_EVENTS
		lprintf( "uv_send closeDb %p", &sql->state->async );
#endif		
		uv_async_send( &sql->state->async );
		// cant' wait here.
	}

}

void SqlObject::autoTransact( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	Local<Context> context = args.GetIsolate()->GetCurrentContext();

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	SetSQLAutoTransact( sql->state->odbc, args[0]->TOBOOL(args.GetIsolate()) );
}
//-----------------------------------------------------------
void SqlObject::transact( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	SQLBeginTransact( sql->state->odbc );
}
//-----------------------------------------------------------
void SqlObject::commit( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	SQLCommit( sql->state->odbc );
}
//-----------------------------------------------------------

void SqlObject::escape( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	if( args[0]->IsUndefined() ) return; // undefined is still undefined
	if( args[0]->IsNull() ) { 
		args.GetReturnValue().Set( args[0] ); // undefined is still undefined
		return;
	}
	String::Utf8Value tmp( USE_ISOLATE( isolate ) args[0] );
	size_t resultlen;
	char *out = EscapeSQLBinaryExx(sql->state->odbc, (*tmp), tmp.length(), &resultlen, FALSE DBG_SRC );
	args.GetReturnValue().Set( String::NewFromUtf8( isolate, out, NewStringType::kNormal, (int)resultlen ).ToLocalChecked() );
	Deallocate( char*, out );

}
void SqlObject::unescape( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	if( args[0]->IsUndefined() ) return; // undefined is still undefined
	if( args[0]->IsNull() ) {
		args.GetReturnValue().Set( args[0] ); // undefined is still undefined
		return;
	}
	String::Utf8Value tmp( USE_ISOLATE( isolate ) args[0] );
	size_t outlen;
	char *out = RevertEscapeBinary( (*tmp), &outlen );
	args.GetReturnValue().Set( String::NewFromUtf8( isolate, out, NewStringType::kNormal, (int)outlen ).ToLocalChecked() );
	Deallocate( char*, out );

}
//-----------------------------------------------------------

void SqlStmtObject::Set( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlStmtObject *stmt = ObjectWrap::Unwrap<SqlStmtObject>( args.This() );
	if( args.Length() < 2 ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Required parameters (column, new value) are missing." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	int col = args[0]->ToInt32(isolate->GetCurrentContext()).ToLocalChecked()->Value();
	struct jsox_value_container val;
	memset( &val, 0, sizeof( val ) );
	int arg = 1;
	if( args[arg]->IsInt32() ) {
		val.value_type = JSOX_VALUE_NUMBER;
		val.result_n = args[arg]->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		val.float_result = 0;
		SetDataItem( &stmt->values, col, &val );
	} else if( args[arg]->IsNumber() ) {
		val.value_type = JSOX_VALUE_NUMBER;
		val.result_d = args[arg]->NumberValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
		val.float_result = 0;
		SetDataItem( &stmt->values, col, &val );
	} else if( args[arg]->IsArrayBuffer() ) {
	} else if( args[arg]->IsInt8Array() ) {
	} else if( args[arg]->IsTypedArray() ) {
	}
}

static LOGICAL PushValue( Isolate *isolate, PDATALIST *pdlParams, Local<Value> arg, String::Utf8Value *name, uint32_t p ) {
	struct jsox_value_container val;
	if( name ) {
		val.name = DupCStrLen( *name[0], val.nameLen = name[0].length() );
	}
	else {
		val.name = NULL;
		val.nameLen = 0;
	}
	if( arg->IsNull() ) {
		val.value_type = JSOX_VALUE_NULL;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsString() ) {
		String::Utf8Value text( USE_ISOLATE( isolate ) arg->ToString(isolate->GetCurrentContext()).ToLocalChecked() );
		val.value_type = JSOX_VALUE_STRING;
		val.string = DupCStrLen( *text, val.stringLen = text.length() );
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsInt32() ) {
		val.value_type = JSOX_VALUE_NUMBER;
		val.result_n = arg->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		val.float_result = 0;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsBoolean() ) {
		val.value_type = JSOX_VALUE_NUMBER;
		val.result_n = arg->TOBOOL( isolate );
		val.float_result = 0;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsNumber() ) {
		val.value_type = JSOX_VALUE_NUMBER;
		val.result_d = arg->NumberValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
		val.float_result = 1;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsArrayBuffer() ) {
		Local<ArrayBuffer> myarr = arg.As<ArrayBuffer>();
#if ( NODE_MAJOR_VERSION >= 14 )
		val.string = (char*)myarr->GetBackingStore()->Data();
#else
		val.string = (char*)myarr->GetContents().Data();
#endif
		val.stringLen = myarr->ByteLength();
		val.value_type = JSOX_VALUE_TYPED_ARRAY;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsUint8Array() ) {
		Local<Uint8Array> _myarr = arg.As<Uint8Array>();
		Local<ArrayBuffer> buffer = _myarr->Buffer();
#if ( NODE_MAJOR_VERSION >= 14 )
		val.string = (char*)buffer->GetBackingStore()->Data();
#else
		val.string = (char*)buffer->GetContents().Data();
#endif
		val.stringLen = buffer->ByteLength();
		val.value_type = JSOX_VALUE_TYPED_ARRAY;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsTypedArray() ) {
		Local<TypedArray> _myarr = arg.As<TypedArray>();
		Local<ArrayBuffer> buffer = _myarr->Buffer();
#if ( NODE_MAJOR_VERSION >= 14 )
		val.string = (char*)buffer->GetBackingStore()->Data();
#else
		val.string = (char*)buffer->GetContents().Data();
#endif
		val.stringLen = buffer->ByteLength();
		val.value_type = JSOX_VALUE_TYPED_ARRAY;
		AddDataItem( pdlParams, &val );
	}
	else {
		String::Utf8Value text( USE_ISOLATE( isolate ) arg->ToString(isolate->GetCurrentContext()).ToLocalChecked() );
		val.value_type = JSOX_VALUE_STRING;
		val.string = DupCStrLen( *text, val.stringLen = text.length() );
		//AddDataItem( pdlParams, &val );
	    
		lprintf( "Unsupported TYPE parameter %d %s", p+1, *text );
		return FALSE;
	}
	return TRUE;
}

static void buildQueryResult( struct query_thread_params* params ) {
	Isolate* isolate = params->isolate;
	Local<Context> context = isolate->GetCurrentContext();
	SqlObject* sql = params->sql;
	INDEX idx = 0;
	int items;
	struct jsox_value_container* jsval;
	PDATALIST pdlRecord = params->pdlRecord;
	DATA_FORALL( pdlRecord, idx, struct jsox_value_container*, jsval ) {
		if (jsval->value_type == JSOX_VALUE_UNDEFINED) break;
	}
	items = (int)idx;
	//&sql->columns, &sql->result, &sql->resultLens, &sql->fields
	if (pdlRecord)
	{
		int usedFields = 0;
		int maxDepth = 0;
		struct fieldTypes {
			const char* name;
			int used;
			int first;
			int hasArray;
			Local<Array> array;
		} *fields = NewArray( struct fieldTypes, items );
		int usedTables = 0;
		struct tables {
			//const char *table;
			const char* alias;
			Local<Object> container;
		}  *tables = NewArray( struct tables, items + 1 );
		struct colMap {
			int depth;
			int col;
			//const char *table;
			const char* alias;
			Local<Object> container;
			struct tables* t;
		}  *colMap = NewArray( struct colMap, items );
		//tables[usedTables].table = NULL;
		tables[usedTables].alias = NULL;
		usedTables++;
		//lprintf( "adding a table usage NULL" );

		DATA_FORALL( pdlRecord, idx, struct jsox_value_container*, jsval ) {
			int m;
			if (jsval->value_type == JSOX_VALUE_UNDEFINED) break;

			for (m = 0; m < usedFields; m++) {
				if (StrCaseCmp( fields[m].name, jsval->name ) == 0) {
					// this field duplicated a field already in the structure
					colMap[idx].col = m;
					colMap[idx].depth = fields[m].used;
					if (colMap[idx].depth > maxDepth)
						maxDepth = colMap[idx].depth + 1;
					colMap[idx].alias = StrDup( PSSQL_GetColumnTableAliasName( sql->state->odbc, (int)idx ) );
					//lprintf( "Alias:%s also in %s", jsval->name, colMap[idx].alias);
					int table;
					for (table = 0; table < usedTables; table++) {
						if (StrCmp( tables[table].alias, colMap[idx].alias ) == 0) {
							//lprintf( "Table already existed?");
							colMap[idx].t = tables + table;
							break;
						}
					}
					if (table == usedTables) {
						//tables[table].table = colMap[idx].table;
						tables[table].alias = colMap[idx].alias;
						colMap[idx].t = tables + table;
						usedTables++;
						//lprintf( "adding a table usage %s", colMap[idx].alias, colMap[idx].table);
					}
					fields[m].used++;
					break;
				}
			}

			if (m == usedFields) {
				colMap[idx].col = m;
				colMap[idx].depth = 0;
				//colMap[idx].table = StrDup( PSSQL_GetColumnTableName( sql->state->odbc, (int)idx ) );
				colMap[idx].alias = StrDup( PSSQL_GetColumnTableAliasName( sql->state->odbc, (int)idx ) );
				//lprintf( "Alias:%s in %s", jsval->name, colMap[idx].alias);
				if (colMap[idx].alias && colMap[idx].alias[0]) {
					int table;
					for (table = 0; table < usedTables; table++) {
						if (StrCmp( tables[table].alias, colMap[idx].alias ) == 0) {
							colMap[idx].t = tables + table;
							break;
						}
					}
					if (table == usedTables) {
						//tables[table].table = colMap[idx].table;
						tables[table].alias = colMap[idx].alias;
						colMap[idx].t = tables + table;
						usedTables++;
						//lprintf( "adding a table usage %s", colMap[idx].alias, colMap[idx].table);
					}
				}
				else
					colMap[idx].t = tables;
				fields[usedFields].first = (int)idx;
				fields[usedFields].name = jsval->name;// sql->fields[idx];
				fields[usedFields].used = 1;
				fields[usedFields].hasArray = FALSE;
				usedFields++;
			}
		}
		// NULL and 1 is just 1 table still...
		if (usedTables > 2)
			for (int m = 0; m < usedFields; m++) {
				for (int t = 1; t < usedTables; t++) {
					if (StrCaseCmp( fields[m].name, tables[t].alias ) == 0) {
						fields[m].used++;
					}
				}
			}
		Local<Array> records = Array::New( isolate );
		Local<Object> record;
		if (pdlRecord) {
			int row = 0;
			do {
				Local<Value> val;
				tables[0].container = record = Object::New( isolate );
				if (usedTables > 2 && maxDepth > 1)
					for (int n = 1; n < usedTables; n++) {
						tables[n].container = Object::New( isolate );
						SETVAR( record, tables[n].alias, tables[n].container );
					}
				else
					for (int n = 0; n < usedTables; n++)
						tables[n].container = record;

				DATA_FORALL( pdlRecord, idx, struct jsox_value_container*, jsval ) {
					if (jsval->value_type == JSOX_VALUE_UNDEFINED) break;

					Local<Object> container = colMap[idx].t->container;
					if (fields[colMap[idx].col].used > 1) {
						// add an array on the name for each result to be stored
						if (fields[colMap[idx].col].first == idx) {
							if (!jsval->name)
								lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s", GetText( params->statement ) );
							else {
								SETVAR( record, jsval->name
									, fields[colMap[idx].col].array = Array::New( isolate )
								);
								fields[colMap[idx].col].hasArray = TRUE;
							}
						}
					}

					switch (jsval->value_type) {
					default:
						lprintf( "Unhandled value result type:%d", jsval->value_type );
						break;
					case JSOX_VALUE_DATE:
					{
						Local<Script> script;
						char buf[64];
						snprintf( buf, 64, "new Date('%s')", jsval->string );
						script = Script::Compile( isolate->GetCurrentContext()
							, String::NewFromUtf8( isolate, buf, NewStringType::kNormal ).ToLocalChecked()
#if ( NODE_MAJOR_VERSION >= 16 )
							, new ScriptOrigin( isolate, String::NewFromUtf8( isolate, "DateFormatter"
						                                  , NewStringType::kInternalized ).ToLocalChecked() )
#else
								, new ScriptOrigin( String::NewFromUtf8( isolate, "DateFormatter"
						                                  , NewStringType::kInternalized ).ToLocalChecked() )
#endif
						                        ).ToLocalChecked();
						val = script->Run( isolate->GetCurrentContext() ).ToLocalChecked();
					}
					break;
					case JSOX_VALUE_TRUE:
						val = True( isolate );
						break;
					case JSOX_VALUE_FALSE:
						val = False( isolate );
						break;
					case JSOX_VALUE_NULL:
						val = Null( isolate );
						break;
					case JSOX_VALUE_NUMBER:
						if (jsval->float_result) {
							val = Number::New( isolate, jsval->result_d );
						}
						else {
							val = Number::New( isolate, (double)jsval->result_n );
						}
						break;
					case JSOX_VALUE_STRING:
						if (!jsval->string)
							val = Null( isolate );
						else
							val = localString( isolate, (char*)Hold( jsval->string ), (int)jsval->stringLen );
						break;
					case JSOX_VALUE_TYPED_ARRAY:
						//lprintf( "Should result with a binary thing" );

#if ( NODE_MAJOR_VERSION >= 14 )
						std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( Hold( jsval->string ), jsval->stringLen, releaseBufferBackingStore, NULL );
						Local<Object> ab = ArrayBuffer::New( isolate, bs );
						//Local<ArrayBuffer> ab =
						//	ArrayBuffer::New( isolate, (char*)Hold( jsval->string ), jsval->stringLen );

#else
						Local<ArrayBuffer> ab =
							ArrayBuffer::New( isolate, (char*)Hold( jsval->string ), jsval->stringLen );

						PARRAY_BUFFER_HOLDER holder = GetHolder();
						holder->o.Reset( isolate, ab );
						holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
						holder->buffer = jsval->string;
						jsval->string = NULL; // steal this buffer, don't let DB release it.
#endif

						val = ab;
						break;
					}
					if (fields[colMap[idx].col].used == 1) {
						if (!jsval->name)
							lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s", GetText( params->statement ) );
						else
							SETVAR( record, jsval->name, val );
					}
					else if (fields[colMap[idx].col].used > 1) {
						if (!jsval->name)
							lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s", GetText( params->statement ) );
						else
							SETVAR( colMap[idx].t->container, jsval->name, val );
						if (fields[colMap[idx].col].hasArray) {
							if (colMap[idx].alias)
								SETVAR( fields[colMap[idx].col].array, colMap[idx].alias, val );
							SETN( fields[colMap[idx].col].array, colMap[idx].depth, val );
						}
					}
				}
				SETN( records, row++, record );
			} while (FetchSQLRecordJS( sql->state->odbc, &pdlRecord ));
		}
		{
			int c;
			for (c = 0; c < items; c++) {
				if (colMap[c].alias) Deallocate( const char*, colMap[c].alias );
				//if( colMap[c].table ) Deallocate( char*, colMap[c].table );
			}
		}
		Deallocate( struct fieldTypes*, fields );
		Deallocate( struct tables*, tables );
		Deallocate( struct colMap*, colMap );

		//SQLEndQuery( sql->state->odbc );
		if (!params->promise.IsEmpty()) {
			Local<Promise::Resolver> res = params->promise.Get( isolate );
			res->Resolve( context, records );
			params->promise.Reset();
		}
		else {
			params->results = records;
		}
		//args.GetReturnValue().Set(records);
	}
	else
	{
		//SQLEndQuery( sql->state->odbc );
		if (!params->promise.IsEmpty()) {
			params->promise.Get( isolate )->Resolve( context, Array::New( isolate ) );
			params->promise.Reset();
		}
		else {
			params->results = Array::New( isolate );
		}
			//lprintf( "Probably an empty result...");
		//args.GetReturnValue().Set();
	}
	LineRelease( params->statement );
	ReleaseSQLResults( &params->pdlRecord );
	DeleteDataList( &params->pdlParams );
}

static void DoQuery( struct query_thread_params *params ) {
	Isolate* isolate = params->isolate;
	Local<Context> context = params->context;

	SqlObject* sql = params->sql;
	PTEXT statement = params->statement;
	//lprintf( "Doing Query:%s", GetText( params->statement ) );
	if (!SQLRecordQuery_js(sql->state->odbc, GetText(statement), GetTextSize(statement), &params->pdlRecord, params->pdlParams DBG_SRC)) {
		const char* error;
		ReleaseSQLResults( &params->pdlRecord );
		FetchSQLError(sql->state->odbc, &error);
		params->error = StrDup( error );
		if( params->promise.IsEmpty() ) {
			// not promised, can throw an exception now.
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, error, v8::NewStringType::kNormal ).ToLocalChecked() ) );

		}
	}

}

static uintptr_t queryThread( PTHREAD thread ) {
	struct query_thread_params* params = (struct query_thread_params*)GetThreadParam( thread );
	struct userMessage* msg = NewArray( struct userMessage, 1 );
	SetSQLThreadProtect( params->sql->state->odbc, TRUE );
	//lprintf( "ThreadTo Doing Query:%s", GetText( params->statement ) );
	DoQuery( params );
	//delete params;

	msg->mode = UserMessageModes::Query;
	// build result is called in uv thread, so all info is just passed....
	msg->params = params;
	msg->onwhat = NULL;
	msg->argc = 0;
	msg->argv = NULL;
	msg->done = 0;
	EnqueLink( &params->sql->state->messages, msg );
#ifdef DEBUG_EVENTS
	lprintf( "uv_send queryThread %p", &params->sql->state->async );
#endif	
	uv_async_send( &params->sql->state->async );
	return 0;
}

static void queryBuilder( const v8::FunctionCallbackInfo<Value>& args, SqlObject *sql, LOGICAL promised ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if (args.Length() == 0) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Required parameter, SQL query, is missing." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	String::Utf8Value sqlStmt( USE_ISOLATE( isolate ) args[0] );
	PTEXT statement = NULL;
	PDATALIST pdlParams = NULL;

	if (args.Length() == 1) {
		String::Utf8Value sqlStmt( USE_ISOLATE( isolate ) args[0] );
		statement = SegCreateFromCharLen( *sqlStmt, sqlStmt.length() );
	}


	if (args.Length() > 1) {
		int arg = 1;
		LOGICAL isFormatString;
		PVARTEXT pvtStmt = VarTextCreate();
		struct jsox_value_container val;
		memset( &val, 0, sizeof( val ) );
		if (StrChr( *sqlStmt, ':' )
			|| StrChr( *sqlStmt, '@' )
			|| StrChr( *sqlStmt, '$' )) {
			if (args[1]->IsObject()) {
				arg = 2;
				pdlParams = CreateDataList( sizeof( struct jsox_value_container ) );
				Local<Object> params = Local<Object>::Cast( args[1] );
				Local<Array> paramNames = params->GetOwnPropertyNames( isolate->GetCurrentContext() ).ToLocalChecked();
				for (uint32_t p = 0; p < paramNames->Length(); p++) {
					Local<Value> valName = GETN( paramNames, p );
					Local<Value> value = GETV( params, valName );
					String::Utf8Value name( USE_ISOLATE( isolate ) valName->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
					if (!PushValue( isolate, &pdlParams, value, &name, p )) {
						lprintf( "bad value in SQL:%s", *sqlStmt );
					}
				}
			}
			else {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "Required parameter 2, Named Paramter Object, is missing." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
				return;
			}
			isFormatString = TRUE;
		}
		else if (StrChr( *sqlStmt, '?' )) {
			String::Utf8Value sqlStmt( USE_ISOLATE( isolate ) args[0] );
			statement = SegCreateFromCharLen( *sqlStmt, sqlStmt.length() );
			isFormatString = TRUE;
		}
		else {
			arg = 0;
			isFormatString = FALSE;
		}

		if (!pdlParams)
			pdlParams = CreateDataList( sizeof( struct jsox_value_container ) );
		if (!isFormatString) {
			for (; arg < args.Length(); arg++) {
				if (args[arg]->IsString()) {
					String::Utf8Value text( USE_ISOLATE( isolate ) args[arg]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
					if (arg & 1) { // every odd parameter is inserted
						val.value_type = JSOX_VALUE_STRING;
						val.string = DupCStrLen( *text, text.length() );
						AddDataItem( &pdlParams, &val );
						VarTextAddCharacter( pvtStmt, '?' );
					}
					else {
						VarTextAddData( pvtStmt, *text, text.length() );
						continue;
					}
				}
				else {
					if (!PushValue( isolate, &pdlParams, args[arg], NULL, arg ))
						lprintf( "bad value in format parameter string:%s", *sqlStmt );
					VarTextAddCharacter( pvtStmt, '?' );
				}
			}
			statement = VarTextGet( pvtStmt );
			VarTextDestroy( &pvtStmt );
		}
		else {
			String::Utf8Value sqlStmt( USE_ISOLATE( isolate ) args[0] );
			statement = SegCreateFromCharLen( *sqlStmt, sqlStmt.length() );
			for (; arg < args.Length(); arg++) {
				if (!PushValue( isolate, &pdlParams, args[arg], NULL, 0 ))
					lprintf( "Bad value in sql statement:%s", *sqlStmt );
			}
		}
	}
	if (statement) {
		struct query_thread_params *params = new query_thread_params();
		params->isolate = isolate;
		params->context = context;
		params->sql = sql;
		params->statement = statement;
      params->pdlRecord = NULL;
		params->pdlParams = pdlParams;
		if (promised) {
#ifdef DEBUG_EVENTS
			lprintf( "Making promise?", promised, sql->state->thread );
#endif			
			if (!sql->state->thread) {
				sql->state->thread = MakeThread();
				//lprintf( "This should keep it open..." );
				class constructorSet* c = getConstructors( isolate );
				uv_async_init( c->loop, &sql->state->async, sqlUserAsyncMsg );
				sql->state->async.data = sql->state;
			}
			//lprintf( " making promise to return..." );
			Local<Promise::Resolver> pr = Promise::Resolver::New( context ).ToLocalChecked();
			params->promise.Reset( isolate, pr );
			ThreadTo( queryThread, (uintptr_t)params );
			args.GetReturnValue().Set( pr->GetPromise() );
			//lprintf("Should return now?");
		}
		else  // not promised, is not run on a thread, cleanup should happen NOW.
		{
			//lprintf( "Non Promised query... %s", GetText( params->statement ) );
			DoQuery( params ); // might throw instead of having a record.
			if( !params->error ) {
				buildQueryResult( params );
				args.GetReturnValue().Set( params->results );
			} else {
				// buildQueryResult releases resources that are used...
				if( params->error ) ReleaseEx( params->error DBG_SRC );
				else args.GetReturnValue().Set( Array::New( isolate ) );
				LineRelease( params->statement );
				if( params->pdlParams )
					DeleteDataList( &params->pdlParams );
			}
			delete params;
		}
	}
}


void SqlObject::promisedQuery( const v8::FunctionCallbackInfo<Value>& args ) {
	SqlObject* sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	queryBuilder( args, sql, TRUE );
}

void SqlObject::query( const v8::FunctionCallbackInfo<Value>& args ) {
	SqlObject* sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	queryBuilder( args, sql, FALSE );
}

//-----------------------------------------------------------

void SqlObject::OnOpen( uintptr_t psv, PODBC odbc ){
	SqlObject *this_ = (SqlObject*)psv;
	if( this_->openCallback.IsEmpty() ) return;
	PTHREAD thread = MakeThread();
	struct userMessage msg;
	this_->state->odbc = odbc;
	msg.mode = UserMessageModes::OnOpen;
	msg.onwhat = NULL;
	msg.done = 0;
	
	if( thread != this_->state->thread ) {
		msg.waiter = thread;
		EnqueLink( &this_->state->messages, &msg );
#ifdef DEBUG_EVENTS
		lprintf( "uv_send onOpen %p", &this_->state->async );
#endif		
		uv_async_send( &this_->state->async );
		while( !msg.done ) WakeableSleep( 1000 );
	} else {
		msg.waiter = NULL;
		EnqueLink( &this_->state->messages, &msg );
		// might be multiple things queued... (probably not)
		while( !msg.done ) {
#ifdef DEBUG_EVENTS			
			lprintf( "OnOpen Called async handler directly %p", &this_->state->async );
#endif			
			sqlUserAsyncMsgEx( &this_->state->async, TRUE );
		}
	}
}
//-----------------------------------------------------------

static void WeakReferenceReleased( const v8::WeakCallbackInfo<void> &info ){
	//Persistent< Value > object
	void *parameter = info.GetParameter();
	//lprintf( "Sql object garbage collected (close UV)");
	SqlObject *sql = (SqlObject*)parameter;
	if( sql->state->async.data ){
		// only do this if we started an async callback on it.
		struct userMessage msg;
		msg.mode = UserMessageModes::OnDeallocate;
		msg.onwhat = NULL;
		msg.done = 0;
		msg.waiter = NULL;
		EnqueLink( &sql->state->messages, &msg );
		while( !msg.done ) {
#ifdef DEBUG_EVENTS			
			lprintf( "weakRef Called async handler directly %p", &sql->state->async );
#endif			
			sqlUserAsyncMsgEx( &sql->state->async, TRUE );
		}
	}
	sql->_this.Reset();
}

//-----------------------------------------------------------

SqlObject::SqlObject( const char *dsn, Isolate *isolate, Local<Object>jsThis, Local<Function> _openCallback )
{
	state = NewArray( struct sql_object_state, 1 );
	state->sql = this;
	memset( &state->async, 0, sizeof( state->async ) );
	state->messages = NULL;
	state->userFunctions = NULL;
	state->thread = NULL;
	state->optionInitialized = FALSE;

	state->isolate = isolate;
	this->_this.Reset( isolate, jsThis );
	this->_this.SetWeak( (void*)this, WeakReferenceReleased, WeakCallbackType::kParameter );
	this->Wrap( jsThis );

	if( !_openCallback.IsEmpty() ) {
#ifdef DEBUG_EVENTS
		lprintf( "Open Callback inits handle.... %p", &this->state->async );
#endif		
		state->thread = MakeThread();
		class constructorSet *c = getConstructors( isolate );
		this->state->async.data = this->state;
		uv_async_init( c->loop, &this->state->async, sqlUserAsyncMsg );
		this->openCallback.Reset( isolate, _openCallback );
	}
	state->odbc = ConnectToDatabaseLoginCallback( dsn, NULL, NULL, FALSE, SqlObject::OnOpen, (uintptr_t)this DBG_SRC );
	SetSQLThreadProtect( state->odbc, FALSE );
	//SetSQLAutoClose( odbc, TRUE );
}

SqlObject::~SqlObject() {
	INDEX idx;
	struct SqlObjectUserFunction *data;
#ifdef DEBUG_EVENTS
	lprintf( "Deleting Object" );
#endif	
	LIST_FORALL( state->userFunctions, idx, struct SqlObjectUserFunction*, data ) {
		data->cb.Reset();
	}
	// state->thread is probably already cleared 1) weskRef closes the handle, and dispatches close immediately
	// end() or close() sends a close event to UV, which deletes the queue, and clears this too.
	if( state->thread )
	{
		struct userMessage *msg = NewArray( struct userMessage, 1 );
		msg->mode = UserMessageModes::OnDeallocate;
		msg->onwhat = NULL;
		msg->done = 0;
		msg->waiter = NULL;
		EnqueLink( &state->messages, msg );
#ifdef DEBUG_EVENTS
		lprintf( "uv_send sqlObject Destroy %p", &state->async );
#endif
		uv_async_send( &state->async );
	}
	CloseDatabase( state->odbc );
	ReleaseEx( state DBG_SRC );
}

//-----------------------------------------------------------

OptionTreeObject::OptionTreeObject()  {
}

OptionTreeObject::~OptionTreeObject() {
}

void OptionTreeObject::New(const v8::FunctionCallbackInfo<Value>& args) {
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
		class constructorSet *c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New(isolate, c->otoConstructor);
		args.GetReturnValue().Set(cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked());
	}
}


void OptionTreeObject::Init(  ) {
	Isolate* isolate = Isolate::GetCurrent();

	Local<FunctionTemplate> optionTemplate;
	// Prepare constructor template
	optionTemplate = FunctionTemplate::New( isolate, New );
	optionTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.vfs.option.node" ) );
	optionTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( optionTemplate, "eo", enumOptionNodes );
	NODE_SET_PROTOTYPE_METHOD( optionTemplate, "fo", findOptionNode );
	NODE_SET_PROTOTYPE_METHOD( optionTemplate, "go", getOptionNode );
	Local<Template> proto = optionTemplate->InstanceTemplate();

	proto->SetNativeDataProperty( String::NewFromUtf8Literal( isolate, "value" )
			, readOptionNode
			, writeOptionNode );

	//NODE_SET_PROTOTYPE_METHOD( optionTemplate, "ro", readOptionNode );
	//NODE_SET_PROTOTYPE_METHOD( optionTemplate, "wo", writeOptionNode );

	class constructorSet *c = getConstructors( isolate );
	c->otoConstructor.Reset( isolate, optionTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
}


void SqlObject::getOptionNode( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();

	if( argc < 1 ) {
		return;
	}

	SqlObject *sqlParent = ObjectWrap::Unwrap<SqlObject>( args.This() );
	if( !sqlParent->state->optionInitialized ) {
		SetOptionDatabaseOption( sqlParent->state->odbc );
		sqlParent->state->optionInitialized = TRUE;
	}

	String::Utf8Value tmp( USE_ISOLATE( isolate ) args[0] );
	char *optionPath = StrDup( *tmp );

	class constructorSet *c = getConstructors( isolate );
	Local<Function> cons = Local<Function>::New( isolate, c->otoConstructor );
	MaybeLocal<Object> o = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL );
	args.GetReturnValue().Set( o.ToLocalChecked() );

	OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( o.ToLocalChecked() );
	oto->odbc = sqlParent->state->odbc;
	//lprintf( "SO Get %p ", sqlParent->odbc );
	oto->node =  GetOptionIndexExx( sqlParent->state->odbc, NULL, optionPath, NULL, NULL, NULL, TRUE, TRUE DBG_SRC );
}


void OptionTreeObject::getOptionNode( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();

	if( argc < 1 ) {
		return;
	}

	OptionTreeObject *parent = ObjectWrap::Unwrap<OptionTreeObject>( args.Holder() );

	String::Utf8Value tmp( USE_ISOLATE( isolate ) args[0] );
	char *optionPath = StrDup( *tmp );

	class constructorSet *c = getConstructors( isolate );
	Local<Function> cons = Local<Function>::New( isolate, c->otoConstructor );
	Local<Object> o;
	//lprintf( "objecttreeobject constructor..." );
	args.GetReturnValue().Set( o = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );

	OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( o );
	oto->odbc = parent->odbc;
	//lprintf( "OTO Get %p  %p", parent->db->odbc, parent->node );
	oto->node =  GetOptionIndexExx( parent->odbc, parent->node, optionPath, NULL, NULL, NULL, TRUE, TRUE DBG_SRC );
	//lprintf( "result node: %s %p %p", optionPath, oto->node, o );
	Release( optionPath );
}

void SqlObject::findOptionNode( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();

	if( argc < 1 ) {
		return;
	}
	String::Utf8Value tmp( USE_ISOLATE( isolate ) args[0] );
	char *optionPath = StrDup( *tmp );
	SqlObject *sqlParent = ObjectWrap::Unwrap<SqlObject>( args.This() );
	if( !sqlParent->state->optionInitialized ) {
		SetOptionDatabaseOption( sqlParent->state->odbc );
		sqlParent->state->optionInitialized = TRUE;
	}
	POPTION_TREE_NODE newNode = GetOptionIndexExx( sqlParent->state->odbc, NULL, optionPath, NULL, NULL, NULL, FALSE, TRUE DBG_SRC );

	if( newNode ) {
		class constructorSet *c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->otoConstructor );
		Local<Object> o;
		args.GetReturnValue().Set( o = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );

		OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( o );
		oto->odbc = sqlParent->state->odbc;
		oto->node = newNode;
	}
	Release( optionPath );
	args.GetReturnValue().Set( Null(isolate) );
}


void OptionTreeObject::findOptionNode( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();

	if( argc < 1 ) {
		return;
	}

	POPTION_TREE_NODE newOption;
	OptionTreeObject *parent = ObjectWrap::Unwrap<OptionTreeObject>( args.This() );

	String::Utf8Value tmp( USE_ISOLATE( isolate ) args[0] );
	char *optionPath = StrDup( *tmp );
	newOption = GetOptionIndexExx( parent->odbc, parent->node, optionPath, NULL, NULL, NULL, FALSE, TRUE DBG_SRC );
	if( newOption ) {
		class constructorSet *c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->otoConstructor );
		Local<Object> o;
		args.GetReturnValue().Set( o = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );

		OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( o );
		oto->odbc = parent->odbc;
		oto->node = newOption;
	}
	Release( optionPath );
	args.GetReturnValue().Set( Null( isolate ) );
}

struct enumArgs {
	Local<Function>cb;
	Isolate *isolate;
	PODBC odbc;
};

int CPROC invokeCallback( uintptr_t psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags ) {
	struct enumArgs *args = (struct enumArgs*)psv;
	Local<Value> argv[2];

	class constructorSet *c = getConstructors( args->isolate );
	Local<Function> cons = c->otoConstructor.Get( args->isolate );
	Local<Object> o;
	o = cons->NewInstance( args->isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();

	OptionTreeObject *oto = OptionTreeObject::Unwrap<OptionTreeObject>( o );
	oto->odbc = args->odbc;
	oto->node = ID;

	argv[0] = o;
	argv[1] = String::NewFromUtf8( args->isolate, name, v8::NewStringType::kNormal ).ToLocalChecked();

	MaybeLocal<Value> r = args->cb->Call(args->isolate->GetCurrentContext(), Null(args->isolate), 2, argv );
	if( r.IsEmpty() )
		return 0;
	return 1;
}


static void enumOptionNodes( const v8::FunctionCallbackInfo<Value>& args, SqlObject *sqlParent ) {
	struct enumArgs callbackArgs;
	callbackArgs.isolate = args.GetIsolate();

	int argc = args.Length();
	if( argc < 1 ) {
		return;
	}
	
	Isolate* isolate = args.GetIsolate();
	LOGICAL dropODBC;
	if( sqlParent ) {
		if( !sqlParent->state->optionInitialized ) {
			SetOptionDatabaseOption( sqlParent->state->odbc );
			sqlParent->state->optionInitialized = TRUE;
		}
		callbackArgs.odbc = sqlParent->state->odbc;
		dropODBC = FALSE;
	}
	else {
		callbackArgs.odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
		dropODBC = TRUE;
	}
	Local<Function> arg0 = Local<Function>::Cast( args[0] );
	Local<Function> cb( arg0 );

	callbackArgs.cb = Local<Function>::New( isolate, cb );
	callbackArgs.isolate = isolate;

	EnumOptionsEx( callbackArgs.odbc, NULL, invokeCallback, (uintptr_t)&callbackArgs );
	if( dropODBC )
		DropOptionODBC( callbackArgs.odbc );
}

void SqlObject::enumOptionNodes( const v8::FunctionCallbackInfo<Value>& args ) {
	SqlObject *sqlParent = ObjectWrap::Unwrap<SqlObject>( args.This() );
	::enumOptionNodes( args, sqlParent );
}
void SqlObject::enumOptionNodesInternal( const v8::FunctionCallbackInfo<Value>& args ) {
	::enumOptionNodes( args, NULL );
}

void OptionTreeObject::enumOptionNodes( const v8::FunctionCallbackInfo<Value>& args ) {
	struct enumArgs callbackArgs;
	callbackArgs.isolate = args.GetIsolate();

	int argc = args.Length();
	if( argc < 1 ) {
		return;
	}

	Isolate* isolate = args.GetIsolate();
	OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( args.This() );
	Local<Function> arg0 = Local<Function>::Cast( args[0] );
	Local<Function> cb( arg0 );

	callbackArgs.odbc = oto->odbc;
	callbackArgs.cb = Local<Function>::New( isolate, cb );
	callbackArgs.isolate = isolate;

	EnumOptionsEx( oto->odbc, oto->node, invokeCallback, (uintptr_t)&callbackArgs );
}

void OptionTreeObject::readOptionNode( v8::Local<v8::String> field,
                              const PropertyCallbackInfo<v8::Value>& info ) {
	OptionTreeObject* oto = node::ObjectWrap::Unwrap<OptionTreeObject>( info.This() );
	char *buffer;
	size_t buflen;
	int res = (int)GetOptionStringValueEx( oto->odbc, oto->node, &buffer, &buflen DBG_SRC );
	if( !buffer || res < 0 )
		return;
	info.GetReturnValue().Set( String::NewFromUtf8( info.GetIsolate(), buffer, v8::NewStringType::kNormal ).ToLocalChecked() );
}

void OptionTreeObject::writeOptionNode( v8::Local<v8::String> field,
                              v8::Local<v8::Value> val,
                              const PropertyCallbackInfo<void>&info ) {
	String::Utf8Value tmp( USE_ISOLATE( info.GetIsolate() ) val );
	OptionTreeObject* oto = node::ObjectWrap::Unwrap<OptionTreeObject>( info.Holder() );
	SetOptionStringValueEx( oto->odbc, oto->node, *tmp );
}


static void option_( const v8::FunctionCallbackInfo<Value>& args, int internal ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();
	char *sect;
	char *optname;
	char *defaultVal;
	char *filename;

	if( argc > 0 ) {
		String::Utf8Value tmp( USE_ISOLATE( isolate ) args[0] );
		defaultVal = StrDup( *tmp );
	}
	else
		defaultVal = StrDup( "" );

	if( argc > 1 ) {
		String::Utf8Value tmp( USE_ISOLATE( isolate ) args[1] );
		sect = defaultVal;
		defaultVal = StrDup( *tmp );
	}
	else
		sect = NULL;

	if( argc > 2 ) {
		String::Utf8Value tmp( USE_ISOLATE( isolate ) args[2] );
		optname = defaultVal;
		defaultVal = StrDup( *tmp );
	}
	else {
		if ((sect && sect[0] == '/')) {
			optname = NULL;
		}
		else {
			optname = sect;
			sect = NULL;
		}
	}

	if( argc > 3 ) {
		String::Utf8Value tmp( USE_ISOLATE( isolate ) args[3] );
		filename = StrDup(*tmp);
	}
	else {
		filename = NULL;
	}

	TEXTCHAR readbuf[1024];
	PODBC use_odbc = NULL;
	if( internal ) {
		// use_odbc = NULL;
	} else 
	{
		SqlObject *sql = SqlObject::Unwrap<SqlObject>( args.This() );

		if( !sql->state->optionInitialized ) {
			SetOptionDatabaseOption( sql->state->odbc );
			sql->state->optionInitialized = TRUE;
		}
		use_odbc = sql->state->odbc;
	}
	SACK_GetPrivateProfileStringExxx( use_odbc
		, sect
		, optname
		, defaultVal
		, readbuf
		, 1024
		, filename
		, TRUE
		DBG_SRC
		);

	Local<String> returnval = String::NewFromUtf8( isolate, readbuf, v8::NewStringType::kNormal ).ToLocalChecked();
	args.GetReturnValue().Set( returnval );

	Deallocate( char*, filename );
	Deallocate( char*, optname );
	Deallocate( char*, sect );
	Deallocate( char*, defaultVal );
}

void SqlObject::option( const v8::FunctionCallbackInfo<Value>& args ) {
	option_( args, 0 );
}

void SqlObject::optionInternal( const v8::FunctionCallbackInfo<Value>& args ) {
	option_( args, 1 );
}

//-----------------------------------------------------------
namespace sack {
	namespace sql {
		namespace options {
			SQLGETOPTION_PROC( LOGICAL, SACK_WritePrivateOptionStringEx )(PODBC odbc, CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile, LOGICAL flush);
		}
	}
}

static void setOption( const v8::FunctionCallbackInfo<Value>& args, int internal ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();
	char *sect;
	char *optname;
	char *defaultVal;

	if( argc > 0 ) {
		String::Utf8Value tmp( USE_ISOLATE( isolate ) args[0] );
		defaultVal = StrDup( *tmp );
	}
	else
		defaultVal = StrDup( "" );

	if( argc > 1 ) {
		String::Utf8Value tmp( USE_ISOLATE( isolate ) args[1] );
		sect = defaultVal;
		defaultVal = StrDup( *tmp );
	}
	else
		sect = NULL;

	if( argc > 2 ) {
		String::Utf8Value tmp( USE_ISOLATE( isolate ) args[2] );
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
		use_odbc = sql->state->odbc;
	}
	if( ( sect && sect[0] == '/' ) ) {
		SACK_GetPrivateProfileStringExxx(use_odbc
			, NULL
			, optname
			, defaultVal
			, readbuf
			, 1024
			, sect
			, TRUE
			DBG_SRC
		);

		if (strcmp(readbuf, defaultVal)) {
			SACK_WritePrivateOptionStringEx(use_odbc
				, NULL
				, optname
				, defaultVal
				, sect, FALSE);
		}
	}
	else {
		SACK_GetPrivateProfileStringExxx(use_odbc
			, sect
			, optname
			, defaultVal
			, readbuf
			, 1024
			, NULL
			, TRUE
			DBG_SRC
		);
		if (strcmp(readbuf, defaultVal)) {
			SACK_WriteOptionString(use_odbc
				, sect
				, optname
				, defaultVal
			);
		}
	}
	Deallocate( char*, optname );
	Deallocate( char*, sect );
	Deallocate( char*, defaultVal );
}

void SqlObject::setOption( const v8::FunctionCallbackInfo<Value>& args ) {
	::setOption( args, 0 );
}
void SqlObject::setOptionInternal( const v8::FunctionCallbackInfo<Value>& args ) {
	::setOption( args, 1 );
}

//-----------------------------------------------------------

void SqlObject::makeTable( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	int argc = args.Length();
	char *tableCommand;

	if( argc > 0 ) {
		PTABLE table;
		String::Utf8Value tmp( USE_ISOLATE( isolate ) args[0] );
		tableCommand = StrDup( *tmp );

		SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );

		table = GetFieldsInSQLEx( tableCommand, false DBG_SRC );
		if( !table ) 
			args.GetReturnValue().Set( False( isolate ) );
		else if( CheckODBCTable( sql->state->odbc, table, CTO_MERGE ) )
			args.GetReturnValue().Set( True(isolate) );
		else
			args.GetReturnValue().Set( False( isolate ) );

	}
	else
		args.GetReturnValue().Set( False( isolate ) );
}

static void callUserFunction( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv );
static void callAggStep( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv );
static void callAggFinal( struct sqlite3_context*onwhat );

static void uv_closed_sql( uv_handle_t* handle ) {
	struct sql_object_state* myself = (struct sql_object_state*)handle->data;
#ifdef DEBUG_EVENTS
	lprintf( "Closed uv_handle(6?)");
#endif
	ReleaseEx( myself DBG_SRC );
}

static void sqlUserAsyncMsgEx( uv_async_t* handle, LOGICAL internal ) {
	LOGICAL closing = FALSE;
	struct sql_object_state* myself = (struct sql_object_state*)handle->data;
	Isolate *isolate = myself->isolate;
	HandleScope scope( isolate );
	struct userMessage *msg;
	while( msg  = (struct userMessage*)DequeLink( &myself->messages ) ) {
		if (msg->mode == UserMessageModes::Query) {
			Local<Context> context = isolate->GetCurrentContext();
			if( msg->params->error ) {
				struct query_thread_params * const params = msg->params;
				Local<Promise::Resolver> res = params->promise.Get( isolate );
				res->Reject( context, String::NewFromUtf8(isolate, params->error).ToLocalChecked() );
				params->promise.Reset();

				ReleaseEx( params->error DBG_SRC );
				LineRelease( params->statement );			
				DeleteDataList( &params->pdlParams );
				ReleaseSQLResults( &params->pdlRecord );
			} else {
				// probably results in a Resolve();
				buildQueryResult( msg->params ); // this is in charge of releasing any data... 
			}
			// this just triggers node's idle callback so the resolved/rejected promise can be dispatched
			//lprintf( "Releasing message..." );
			Release( msg );
			msg = NULL;
		} else if (msg->mode == UserMessageModes::OnOpen) {
			Local<Function> cb = myself->sql->openCallback.Get( isolate );
			Local<Value> args[1] = {myself->sql->_this.Get( isolate )};
			MaybeLocal<Value> result = cb->Call( isolate->GetCurrentContext(), args[0], 1, args );
		}
		else if( msg->onwhat ) {
			struct SqlObjectUserFunction* userData = ( struct SqlObjectUserFunction* )PSSQL_GetSqliteFunctionData( msg->onwhat );
			Isolate* isolate = userData->isolate;
			if( msg->mode == UserMessageModes::OnSqliteFunction )
				callUserFunction( msg->onwhat, msg->argc, msg->argv );
			else if( msg->mode == UserMessageModes::OnSqliteAggStep )
				callAggStep( msg->onwhat, msg->argc, msg->argv );
			else if( msg->mode == UserMessageModes::OnSqliteAggFinal ) {
				callAggFinal( msg->onwhat );
				myself->thread = NULL;
				Hold( myself );
#ifdef DEBUG_EVENTS
				lprintf( "Sack uv_close5");
#endif
				uv_close( (uv_handle_t*)&myself->async, uv_closed_sql );
			}
		} else {
			closing = TRUE;
			myself->thread = NULL;
			Hold( myself );
#ifdef DEBUG_EVENTS
			lprintf( "Sack uv_close6 %p", &myself->async );
#endif
			uv_close( (uv_handle_t*)&myself->async, uv_closed_sql );
		}	
		if( msg ) {
			msg->done = 1;
			if (msg->waiter)
				WakeThread( msg->waiter );
		}
		if( msg->mode == UserMessageModes::OnDeallocate ) {
			ReleaseEx( msg DBG_SRC );
		}
	}
	if( !internal && !closing )
	{
#ifdef DEBUG_EVENTS
		lprintf( "Should be calling node's idle proc..." );
#endif
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
		//lprintf( "called proc?" );
	}
}

static void sqlUserAsyncMsg( uv_async_t* handle ) {
	sqlUserAsyncMsgEx( handle, FALSE );
}

static void releaseBuffer( void *buffer ) {
	Deallocate( void*, buffer );
}

void callUserFunction( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv ) {
	struct SqlObjectUserFunction *userData = (struct SqlObjectUserFunction*)PSSQL_GetSqliteFunctionData( onwhat );
	if( userData->sql->state->thread != MakeThread() ) {
		struct userMessage msg;
		msg.mode = UserMessageModes::OnSqliteFunction;
		msg.onwhat = onwhat;
		msg.argc = argc;
		msg.argv = argv;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &userData->sql->state->messages, &msg );
#ifdef DEBUG_EVENTS
		lprintf( "uv_send call user function %p", &userData->sql->state->async );
#endif		
		uv_async_send( &userData->sql->state->async );

		while( !msg.done ) {
			WakeableSleep( SLEEP_FOREVER );
		}
		return;
	}

	Local<Value> *args;
	if( argc > 0 ) {
		int n;
		char *text;
		int textLen;
		args = new Local<Value>[argc];
		for( n = 0; n < argc; n++ ) {
			int type = PSSQL_GetSqliteValueType( argv[n] );
			switch( type ) {
			case 1:
			{
				int64_t val;
				PSSQL_GetSqliteValueInt64( argv[n], &val );
				if( val & 0xFFFFFFFF00000000ULL )
					args[n] = Number::New( userData->isolate, (double)val );
				else
					args[n] = Integer::New( userData->isolate, (int32_t)val );
				break;
			}
			case 2:
			{
				double val;
				PSSQL_GetSqliteValueDouble( argv[n], &val );
				args[n] = Number::New( userData->isolate, val );
				break;
			}
			case 4:
			{
				const char *data;
				char *_data;
				int len;
				PSSQL_GetSqliteValueBlob( argv[n], &data, &len );
				_data = NewArray( char, len );
				memcpy( _data, data, len );

#if ( NODE_MAJOR_VERSION >= 14 )
				std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( _data, len, releaseBufferBackingStore, NULL );
				Local<Object> arrayBuffer = ArrayBuffer::New( userData->isolate, bs );
#else
				Local<Object> arrayBuffer = ArrayBuffer::New( userData->isolate, _data, len );
				PARRAY_BUFFER_HOLDER holder = GetHolder();
				holder->o.Reset( userData->isolate, arrayBuffer );
				holder->o.SetWeak< ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
				holder->buffer = _data;
#endif
				break;
			}
			case 5:
				args[n] = Null( userData->isolate );
				break;
			case 3:
			default:
				PSSQL_GetSqliteValueText( argv[n], (const char**)&text, &textLen );
				args[n] = String::NewFromUtf8( userData->isolate, text, NewStringType::kNormal, textLen ).ToLocalChecked();
			}
		}
	} else {
		args = NULL;
	}
	Local<Function> cb = Local<Function>::New( userData->isolate, userData->cb );
	Local<Value> str = cb->Call( userData->isolate->GetCurrentContext(), userData->sql->handle(), argc, args ).ToLocalChecked();
	String::Utf8Value result( USE_ISOLATE( userData->isolate ) str->ToString( userData->isolate->GetCurrentContext() ).ToLocalChecked() );
	int type;
	if( ( ( type = 1 ), str->IsArrayBuffer() ) || ( ( type = 2 ), str->IsUint8Array() ) ) {
		uint8_t *buf = NULL;
		size_t length;
		if( type == 1 ) {
			Local<ArrayBuffer> myarr = str.As<ArrayBuffer>();
#if ( NODE_MAJOR_VERSION >= 14 )
			buf = (uint8_t*)myarr->GetBackingStore()->Data();
#else
			buf = (uint8_t*)myarr->GetContents().Data();
#endif
			length = myarr->ByteLength();
		} else if( type == 2 ) {
			Local<Uint8Array> _myarr = str.As<Uint8Array>();
			Local<ArrayBuffer> buffer = _myarr->Buffer();
#if ( NODE_MAJOR_VERSION >= 14 )
			buf = (uint8_t*)buffer->GetBackingStore()->Data();
#else
			buf = (uint8_t*)buffer->GetContents().Data();
#endif
			length = buffer->ByteLength();
		}
		if( buf )
			PSSQL_ResultSqliteBlob( onwhat, (const char *)buf, (int)length, NULL );
	} else if( str->IsNumber() ) {
		if( str->IsInt32() )
			PSSQL_ResultSqliteInt( onwhat, (int)str->IntegerValue( userData->isolate->GetCurrentContext() ).FromMaybe(0) );
		else
			PSSQL_ResultSqliteDouble( onwhat, str->NumberValue( userData->isolate->GetCurrentContext() ).FromMaybe( 0 ) );
	} else if( str->IsString() )
		PSSQL_ResultSqliteText( onwhat, DupCStrLen( *result, result.length() ), result.length(), releaseBuffer );
	else
		lprintf( "unhandled result type (object? array? function?)" );
	if( argc > 0 ) {
		delete[] args;
	}
}


void SqlStmtObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
}

static void destroyUserData( void *vpUserData ) {
	struct SqlObjectUserFunction *userData = (struct SqlObjectUserFunction *)vpUserData;
	userData->cb.Reset();
	userData->cb2.Reset();
	delete userData;
}

void SqlObject::userFunction( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	int argc = args.Length();
	if( !sql->state->thread ) {
		sql->state->thread = MakeThread();
		class constructorSet *c = getConstructors( isolate );
		uv_async_init( c->loop, &sql->state->async, sqlUserAsyncMsg );
		sql->state->async.data = sql->state;
	}

	if( argc > 0 ) {
		String::Utf8Value name( USE_ISOLATE( isolate ) args[0] );
		struct SqlObjectUserFunction *userData = new SqlObjectUserFunction();
		userData->isolate = isolate;
		userData->cb.Reset( isolate, Local<Function>::Cast( args[1] ) );
		userData->sql = sql;
		PSSQL_AddSqliteFunction( sql->state->odbc, *name, callUserFunction, destroyUserData, -1, userData );
	}
}

void SqlObject::userProcedure( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	int argc = args.Length();
	if( !sql->state->thread ) {
		sql->state->thread = MakeThread();
		class constructorSet *c = getConstructors( isolate );
		uv_async_init( c->loop, &sql->state->async, sqlUserAsyncMsg );
		sql->state->async.data = sql->state;
	}

	if( argc > 0 ) {
		String::Utf8Value name( USE_ISOLATE( isolate ) args[0] );
		struct SqlObjectUserFunction *userData = new SqlObjectUserFunction();
		userData->isolate = isolate;
		userData->cb.Reset( isolate, Local<Function>::Cast( args[1] ) );
		userData->sql = sql;
		PSSQL_AddSqliteProcedure( sql->state->odbc, *name, callUserFunction, destroyUserData, -1, userData );
	}
}

void callAggStep( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv ) {
	struct SqlObjectUserFunction *userData = ( struct SqlObjectUserFunction* )PSSQL_GetSqliteFunctionData( onwhat );
	if( userData->sql->state->thread != MakeThread() ) {
		struct userMessage msg;
		msg.mode = UserMessageModes::OnSqliteAggStep;
		msg.onwhat = onwhat;
		msg.argc = argc;
		msg.argv = argv;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &userData->sql->state->messages, &msg );
#ifdef DEBUG_EVENTS
		lprintf( "uv_send AggStep %p", &userData->sql->state->async );
#endif		
		uv_async_send( &userData->sql->state->async );

		while( !msg.done ) {
			WakeableSleep( SLEEP_FOREVER );
		}
		return;
	}

	Local<Value> *args;
	if( argc > 0 ) {
		int n;
		char *text;
		int textLen;
		args = new Local<Value>[argc];
		for( n = 0; n < argc; n++ ) {
			int type = PSSQL_GetSqliteValueType( argv[n] );
			switch( type ) {
			case 1:
			{
				int64_t val;
				PSSQL_GetSqliteValueInt64( argv[n], &val );
				if( val & 0xFFFFFFFF00000000ULL )
					args[n] = Number::New( userData->isolate, (double)val );
				else
					args[n] = Integer::New( userData->isolate, (int32_t)val );
				break;
			}
			case 2:
			{
				double val;
				PSSQL_GetSqliteValueDouble( argv[n], &val );
				args[n] = Number::New( userData->isolate, val );
				break;
			}
			case 4:
			{
				const char *data;
				char *_data;
				int len;
				PSSQL_GetSqliteValueBlob( argv[n], &data, &len );
				_data = NewArray( char, len );
				memcpy( _data, data, len );
#if ( NODE_MAJOR_VERSION >= 14 )
				std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( _data, len, releaseBufferBackingStore, NULL );
				Local<Object> arrayBuffer = ArrayBuffer::New( userData->isolate, bs );
#else
				Local<Object> arrayBuffer = ArrayBuffer::New( userData->isolate, _data, len );
				PARRAY_BUFFER_HOLDER holder = GetHolder();
				holder->o.Reset( userData->isolate, arrayBuffer );
				holder->o.SetWeak< ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
				holder->buffer = _data;
#endif
				break;
			}
			case 5:
				args[n] = Null( userData->isolate );
				break;
			case 3:
			default:
				PSSQL_GetSqliteValueText( argv[n], (const char**)&text, &textLen );
				args[n] = String::NewFromUtf8( userData->isolate, text, NewStringType::kNormal, textLen ).ToLocalChecked();
			}
		}
	} else {
		args = NULL;
	}
	Local<Function> cb = Local<Function>::New( userData->isolate, userData->cb );
	cb->Call( userData->isolate->GetCurrentContext(), userData->sql->handle(), argc, args );
	if( argc > 0 ) {
		delete[] args;
	}
}

void callAggFinal( struct sqlite3_context*onwhat ) {
	struct SqlObjectUserFunction *userData = ( struct SqlObjectUserFunction* )PSSQL_GetSqliteFunctionData( onwhat );
	if( userData->sql->state->thread != MakeThread() ) {
		struct userMessage msg;
		msg.mode = UserMessageModes::OnSqliteAggFinal;
		msg.onwhat = onwhat;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &userData->sql->state->messages, &msg );
#ifdef DEBUG_EVENTS
		lprintf( "uv_send aggFinal %p", &userData->sql->state->async );
#endif		
		uv_async_send( &userData->sql->state->async );

		while( !msg.done ) {
			WakeableSleep( SLEEP_FOREVER );
		}

		return;
	} else {
		struct userMessage msg;
		msg.mode = OnSqliteAggFinal;
		msg.onwhat = NULL;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &userData->sql->state->messages, &msg );
#ifdef DEBUG_EVENTS
		lprintf( "uv_send aggFinal2 %p", &userData->sql->state->async );
#endif		
		uv_async_send( &userData->sql->state->async );
	}

	Local<Function> cb2 = Local<Function>::New( userData->isolate, userData->cb2 );
	Local<Value> str;
	MaybeLocal<Value> mv = cb2->Call( userData->isolate->GetCurrentContext(), userData->sql->handle(), 0, NULL );
	str = mv.ToLocalChecked();
	int type;
	if( ( ( type = 1 ), str->IsArrayBuffer() ) || ( ( type = 2 ), str->IsUint8Array() ) ) {
		uint8_t *buf = NULL;
		size_t length;
		if( type == 1 ) {
			Local<ArrayBuffer> myarr = str.As<ArrayBuffer>();
#if ( NODE_MAJOR_VERSION >= 14 )
			buf = (uint8_t*)myarr->GetBackingStore()->Data();
#else
			buf = (uint8_t*)myarr->GetContents().Data();
#endif
			length = myarr->ByteLength();
		} else if( type == 2 ) {
			Local<Uint8Array> _myarr = str.As<Uint8Array>();
			Local<ArrayBuffer> buffer = _myarr->Buffer();
#if ( NODE_MAJOR_VERSION >= 14 )
			buf = (uint8_t*)buffer->GetBackingStore()->Data();
#else
			buf = (uint8_t*)buffer->GetContents().Data();
#endif
			length = buffer->ByteLength();
		}
		if( buf )
			PSSQL_ResultSqliteBlob( onwhat, (const char *)buf, (int)length, releaseBuffer );
	} else if( str->IsNumber() ) {
		if( str->IsInt32() )
			PSSQL_ResultSqliteInt( onwhat, (int)str->IntegerValue( userData->isolate->GetCurrentContext() ).FromMaybe( 0 ) );
		else
			PSSQL_ResultSqliteDouble( onwhat, str->NumberValue( userData->isolate->GetCurrentContext() ).FromMaybe(0) );
	} else if( str->IsString() ) {
		String::Utf8Value result( USE_ISOLATE( userData->isolate) str->ToString( userData->isolate->GetCurrentContext() ).ToLocalChecked() );
		PSSQL_ResultSqliteText( onwhat, DupCStrLen( *result, result.length() ), result.length(), releaseBuffer );
	}
	else
		lprintf( "unhandled result type (object? array? function?)" );
}

void SqlObject::getRequire( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	args.GetReturnValue().Set( False( isolate ) );
}

void SqlObject::setRequire( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	SetConnectionRequired( sql->state->odbc, args[0]->TOBOOL( isolate ) );
}

void SqlObject::getLogging( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	args.GetReturnValue().Set( GetConnectionRequired( sql->state->odbc )?True(isolate):False( isolate ) );
}

void SqlObject::setLogging( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	if( args.Length() ) {
		bool b( args[0]->TOBOOL( isolate ) );
		if( b )
			SetSQLLoggingDisable( sql->state->odbc, FALSE );
		else
			SetSQLLoggingDisable( sql->state->odbc, TRUE );
	}
}


void SqlObject::error( const v8::FunctionCallbackInfo<Value>& args ) {
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	const char *error;
	FetchSQLError( sql->state->odbc, &error );
	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), error, NewStringType::kNormal, (int)strlen(error) ).ToLocalChecked() );

}

void SqlObject::getProvider( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject* sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	int provider = GetDatabaseProvider( sql->state->odbc );
	args.GetReturnValue().Set( Number::New( isolate, provider ) );

}

static void handleCorruption( uintptr_t psv, PODBC odbc ) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	SqlObject *sql = (SqlObject*)psv;
	Local<Function> cb = Local<Function>::New( isolate, sql->onCorruption.Get( isolate ) );
	cb->Call( isolate->GetCurrentContext(), sql->_this.Get( isolate ), 0, 0 );
}


void SqlObject::setOnCorruption( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	int argc = args.Length();
	sql->onCorruption.Reset( isolate, Local<Function>::Cast( args[0] ) );
	SetSQLCorruptionHandler( sql->state->odbc, handleCorruption, (uintptr_t)sql );

}

void SqlObject::aggregateFunction( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	int argc = args.Length();

	if( argc > 2 ) {
		String::Utf8Value name( USE_ISOLATE( isolate ) args[0] );
		struct SqlObjectUserFunction *userData = new SqlObjectUserFunction();
		userData->isolate = isolate;
		userData->cb.Reset( isolate, Local<Function>::Cast( args[1] ) );
		userData->cb2.Reset( isolate, Local<Function>::Cast( args[2] ) );
		userData->sql = sql;
		PSSQL_AddSqliteAggregate( sql->state->odbc, *name, callAggStep, callAggFinal, destroyUserData, -1, userData );

		if( !sql->state->thread ) {
			sql->state->thread = MakeThread();
			class constructorSet *c = getConstructors( isolate );
			uv_async_init( c->loop, &sql->state->async, sqlUserAsyncMsg );
			sql->state->async.data = sql->state;
		}
	}
	else {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Aggregate requires 3 parameters (name,stepCallback(...args), finalCallback())"), v8::NewStringType::kNormal ).ToLocalChecked() ) );
	}
}

#ifdef INCLUDE_GUI
struct threadParam {
	int( *editor )( PODBC, PSI_CONTROL, LOGICAL );
	class constructorSet* c;
};
static uintptr_t RunEditor( PTHREAD thread ) {
	struct threadParam* tp = ( struct threadParam* )GetThreadParam( thread );
	int (*EditOptions)( PODBC odbc, PSI_CONTROL parent, LOGICAL wait );
	extern void disableEventLoop( class constructorSet *c );
	EditOptions = tp->editor;
	EditOptions( NULL, NULL, TRUE );
	disableEventLoop( tp->c );
	Release( tp );
	return 0;
}

void editOptions( const v8::FunctionCallbackInfo<Value>& args ){
	struct threadParam* tp = new(struct threadParam );
	//int (*EditOptions)( PODBC odbc, PSI_CONTROL parent, LOGICAL wait );
	extern void enableEventLoop( class constructorSet *c );
#ifdef WIN32
	LoadFunction( "bag.psi.dll", NULL );
#else
	LoadFunction( "libbag.psi.so", NULL );
#endif
	tp->c = getConstructors( args.GetIsolate() );
	tp->editor = (int(*)( PODBC, PSI_CONTROL,LOGICAL))LoadFunction( "EditOptions.plugin", "EditOptionsEx" );
	if( tp->editor ) {
		//enableEventLoop( getConstructors( args.GetIsolate() ) );
		ThreadTo( RunEditor, (uintptr_t)tp );
	} else
		lprintf( "Failed to load editor..." );
}
#endif
