
#include "global.h"

#ifdef INCLUDE_GUI
void editOptions( const v8::FunctionCallbackInfo<Value>& args );
#endif

struct SqlObjectUserFunction {
	class SqlObject *sql;
	Persistent<Function> cb;
	Persistent<Function> cb2;
	Isolate *isolate;
	SqlObjectUserFunction() : cb(), cb2() {}
};

class SqlStmtObject : public node::ObjectWrap {
public:
	static v8::Persistent<v8::Function> constructor;
	class SqlObject *sql;
	PDATALIST values;
	SqlStmtObject() {
		values = NULL;
	}
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void Set( const v8::FunctionCallbackInfo<Value>& args );
};

class SqlObject : public node::ObjectWrap {
public:
	PODBC odbc;
	int optionInitialized;
	static v8::Persistent<v8::Function> constructor;
	//int columns;
	//CTEXTSTR *result;
	//size_t *resultLens;
	//CTEXTSTR *fields;
	v8::Persistent<v8::Function> onCorruption;
	Persistent<Object> _this;
	//Persistent<Object> volume;
public:
	PTHREAD thread;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLIST userFunctions;
	PLINKQUEUE messages;

	//static void Init( Local<Object> exports );
	SqlObject( const char *dsn );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void query( const v8::FunctionCallbackInfo<Value>& args );
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

	static void getLogging( const v8::FunctionCallbackInfo<Value>& args );
	static void setLogging( const v8::FunctionCallbackInfo<Value>& args );


	static void doWrap( SqlObject *sql, Local<Object> o );

	~SqlObject();
};


class OptionTreeObject : public node::ObjectWrap {
public:
	POPTION_TREE_NODE node;
	PODBC odbc;
	static v8::Persistent<v8::Function> constructor;

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



struct userMessage{
	int mode;
	struct sqlite3_context*onwhat;
	int argc;
	struct sqlite3_value**argv;
	int done;
	PTHREAD waiter;
};

Persistent<Function> SqlStmtObject::constructor;
Persistent<Function> SqlObject::constructor;

void createSqlObject( const char *name, Local<Object> into ) {
	class SqlObject* obj;
	obj = new SqlObject( name );
	SqlObject::doWrap( obj, into );

}

Local<Value> newSqlObject(Isolate *isolate, int argc, Local<Value> *argv ) {
	Local<Function> cons = Local<Function>::New( isolate, SqlObject::constructor );
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
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> sqlTemplate;
	// Prepare constructor template
	sqlTemplate = FunctionTemplate::New( isolate, SqlObject::New );
	sqlTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.Sqlite", v8::NewStringType::kNormal ).ToLocalChecked() );
	sqlTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap

	Local<FunctionTemplate> sqlStmtTemplate;
	// Prepare constructor template
	sqlStmtTemplate = FunctionTemplate::New( isolate, SqlStmtObject::New );
	sqlStmtTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.Sqlite.statement", v8::NewStringType::kNormal ).ToLocalChecked() );
	sqlStmtTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
	SqlStmtObject::constructor.Reset( isolate, sqlStmtTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "do", SqlObject::query );
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


	// read a portion of the tree (passed to a callback)
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "eo", SqlObject::enumOptionNodes );
	// get without create
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "fo", SqlObject::findOptionNode );
	// get the node.
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "go", SqlObject::getOptionNode );

	sqlTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "error", v8::NewStringType::kNormal ).ToLocalChecked()
			, FunctionTemplate::New( isolate, SqlObject::error )
			, Local<FunctionTemplate>() );
	sqlTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "log", v8::NewStringType::kNormal ).ToLocalChecked()
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
	SqlObject::constructor.Reset( isolate, sqlTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

	Local<Object> sqlfunc = sqlTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();

	SET_READONLY_METHOD(sqlfunc, "eo", SqlObject::enumOptionNodesInternal );
	SET_READONLY_METHOD(sqlfunc, "op", SqlObject::optionInternal );
	SET_READONLY_METHOD(sqlfunc, "so", SqlObject::setOptionInternal );
#ifdef INCLUDE_GUI
	SET_READONLY_METHOD(sqlfunc, "optionEditor", editOptions );
#endif

	SET( exports, "Sqlite", sqlfunc );
}

//-----------------------------------------------------------

void SqlObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		char *dsn;
		SqlObject* obj;
		if( args.Length() > 0 ) {
			String::Utf8Value arg( USE_ISOLATE( isolate ) args[0] );
			dsn = *arg;
			obj = new SqlObject( dsn );
		}
		else {
			obj = new SqlObject( ":memory:" );
		}
		obj->_this.Reset( isolate, args.This() );
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	} else {
		const int argc = 1;
		Local<Value> argv[1];
		if( args.Length() > 0 )
			argv[0] = args[0];
		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), args.Length(), argv ).ToLocalChecked() );
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
	CloseDatabase( sql->odbc );
}

void SqlObject::autoTransact( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	Local<Context> context = args.GetIsolate()->GetCurrentContext();

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	SetSQLAutoTransact( sql->odbc, args[0]->TOBOOL(args.GetIsolate()) );
}
//-----------------------------------------------------------
void SqlObject::transact( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	SQLBeginTransact( sql->odbc );
}
//-----------------------------------------------------------
void SqlObject::commit( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	SQLCommit( sql->odbc );
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
	char *out = EscapeSQLBinaryExx(sql->odbc, (*tmp), tmp.length(), &resultlen, FALSE DBG_SRC );
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

static void PushValue( Isolate *isolate, PDATALIST *pdlParams, Local<Value> arg, String::Utf8Value *name ) {
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
		val.string = (char*)myarr->GetContents().Data();
		val.stringLen = myarr->ByteLength();
		val.value_type = JSOX_VALUE_TYPED_ARRAY;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsUint8Array() ) {
		Local<Uint8Array> _myarr = arg.As<Uint8Array>();
		Local<ArrayBuffer> buffer = _myarr->Buffer();
		val.string = (char*)buffer->GetContents().Data();
		val.stringLen = buffer->ByteLength();
		val.value_type = JSOX_VALUE_TYPED_ARRAY;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsTypedArray() ) {
		Local<TypedArray> _myarr = arg.As<TypedArray>();
		Local<ArrayBuffer> buffer = _myarr->Buffer();
		val.string = (char*)buffer->GetContents().Data();
		val.stringLen = buffer->ByteLength();
		val.value_type = JSOX_VALUE_TYPED_ARRAY;
		AddDataItem( pdlParams, &val );
	}
	else {
		String::Utf8Value text( USE_ISOLATE( isolate ) arg->ToString(isolate->GetCurrentContext()).ToLocalChecked() );
		val.value_type = JSOX_VALUE_STRING;
		val.string = DupCStrLen( *text, val.stringLen = text.length() );
		//AddDataItem( pdlParams, &val );
	    
		lprintf( "Unsupported TYPE %s", *text );
	}

}

void SqlObject::query( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( args.Length() == 0 ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Required parameter, SQL query, is missing."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	String::Utf8Value sqlStmt( USE_ISOLATE( isolate ) args[0] );
	PTEXT statement;
	PDATALIST pdlParams = NULL;

	if( args.Length() == 1 ) {
		String::Utf8Value sqlStmt( USE_ISOLATE( isolate ) args[0] );
		statement = SegCreateFromCharLen( *sqlStmt, sqlStmt.length() );
	}


	if( args.Length() > 1 ) {
		int arg = 1;
		LOGICAL isFormatString;
		PVARTEXT pvtStmt = VarTextCreate();
		struct jsox_value_container val;
		memset( &val, 0, sizeof( val ) );
		if( StrChr( *sqlStmt, ':' )
			|| StrChr( *sqlStmt, '@' )
			|| StrChr( *sqlStmt, '$' ) ) {
			if( args[1]->IsObject() ) {
				arg = 2;
				pdlParams = CreateDataList( sizeof( struct jsox_value_container ) );
				Local<Object> params = Local<Object>::Cast( args[1] );
				Local<Array> paramNames = params->GetOwnPropertyNames(isolate->GetCurrentContext()).ToLocalChecked();
				for( uint32_t p = 0; p < paramNames->Length(); p++ ) {
					Local<Value> valName = GETN( paramNames, p );
					Local<Value> value = GETV( params, valName );
					String::Utf8Value name( USE_ISOLATE( isolate ) valName->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
					PushValue( isolate, &pdlParams, value, &name );
				}
			}
			else {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "Required parameter 2, Named Paramter Object, is missing." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
				return;
			}
			isFormatString = TRUE;
		}
		else if( StrChr( *sqlStmt, '?' ) ) {
			String::Utf8Value sqlStmt( USE_ISOLATE( isolate ) args[0] );
			statement = SegCreateFromCharLen( *sqlStmt, sqlStmt.length() );
			isFormatString = TRUE;
		} 
		else {
			arg = 0;
			isFormatString = FALSE;
		}

		if( !pdlParams )
			pdlParams = CreateDataList( sizeof( struct jsox_value_container ) );
		if( !isFormatString ) {
			for( ; arg < args.Length(); arg++ ) {
				if( args[arg]->IsString() ) {
					String::Utf8Value text( USE_ISOLATE(isolate) args[arg]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
					if( arg & 1 ) { // every odd parameter is inserted
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
					PushValue( isolate, &pdlParams, args[arg], NULL );
					VarTextAddCharacter( pvtStmt, '?' );
				}
			}
			statement = VarTextGet( pvtStmt );
			VarTextDestroy( &pvtStmt );
		}
		else {
			String::Utf8Value sqlStmt( USE_ISOLATE( isolate ) args[0] );
			statement = SegCreateFromCharLen( *sqlStmt, sqlStmt.length() );
			for( ; arg < args.Length(); arg++ ) {
				PushValue( isolate, &pdlParams, args[arg], NULL );
			}
		}
	}
	
	if( statement ) {
		//String::Utf8Value sqlStmt( USE_ISOLATE( isolate ) args[0] );

		SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
		PDATALIST pdlRecord = NULL;
		INDEX idx = 0;
		int items;
		struct jsox_value_container * jsval;

		if( !SQLRecordQuery_js( sql->odbc, GetText(statement), GetTextSize(statement), &pdlRecord, pdlParams DBG_SRC ) ) {
			const char *error;
			FetchSQLError( sql->odbc, &error );
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, error, v8::NewStringType::kNormal ).ToLocalChecked() ) );
			DeleteDataList( &pdlParams );
			return;
		}

		DATA_FORALL( pdlRecord, idx, struct jsox_value_container *, jsval ) {
			if( jsval->value_type == JSOX_VALUE_UNDEFINED ) break;
		}
		items = (int)idx;

		//&sql->columns, &sql->result, &sql->resultLens, &sql->fields
		if( pdlRecord )
		{
			int usedFields = 0;
			int maxDepth = 0;
			struct fieldTypes {
				const char *name;
				int used;
				int first;
				Local<Array> array;
			} *fields = NewArray( struct fieldTypes, items ) ;
			int usedTables = 0;
			struct tables {
				const char *table;
				const char *alias;
				Local<Object> container;
			}  *tables = NewArray( struct tables, items + 1);
			struct colMap {
				int depth;
				int col;
				const char *table;
				const char *alias;
				Local<Object> container;
				struct tables *t;
			}  *colMap = NewArray( struct colMap, items );
			tables[usedTables].table = NULL;
			tables[usedTables].alias = NULL;
			usedTables++;



			DATA_FORALL( pdlRecord, idx, struct jsox_value_container *, jsval ) {
				int m;
				if( jsval->value_type == JSOX_VALUE_UNDEFINED ) break;

				for( m = 0; m < usedFields; m++ ) {
					if( StrCaseCmp( fields[m].name, jsval->name ) == 0 ) {
						colMap[idx].col = m;
						colMap[idx].depth = fields[m].used;
						if( colMap[idx].depth > maxDepth )
							maxDepth = colMap[idx].depth+1;
						colMap[idx].alias = PSSQL_GetColumnTableAliasName( sql->odbc, (int)idx );
						int table;
						for( table = 0; table < usedTables; table++ ) {
							if( StrCmp( tables[table].alias, colMap[idx].alias ) == 0 ) {
								colMap[idx].t = tables + table;
								break;
							}
						}
						if( table == usedTables ) {
							tables[table].table = colMap[idx].table;
							tables[table].alias = colMap[idx].alias;
							colMap[idx].t = tables + table;
							usedTables++;
						}
						fields[m].used++;
						break;
					}
				}
				if( m == usedFields ) {
					colMap[idx].col = m;
					colMap[idx].depth = 0;
					colMap[idx].table = PSSQL_GetColumnTableName( sql->odbc, (int)idx );
					colMap[idx].alias = PSSQL_GetColumnTableAliasName( sql->odbc, (int)idx );
					if( colMap[idx].table && colMap[idx].alias ) {
						int table;
						for( table = 0; table < usedTables; table++ ) {
							if( StrCmp( tables[table].alias, colMap[idx].alias ) == 0 ) {
								colMap[idx].t = tables + table;
								break;
							}
						}
						if( table == usedTables ) {
							tables[table].table = colMap[idx].table;
							tables[table].alias = colMap[idx].alias;
							colMap[idx].t = tables + table;
							usedTables++;
						}
					} else
						colMap[idx].t = tables;
					fields[usedFields].first = (int)idx;
					fields[usedFields].name = jsval->name;// sql->fields[idx];
					fields[usedFields].used = 1;
					usedFields++;
				}
			}
			if( usedTables > 1 )
				for( int m = 0; m < usedFields; m++ ) {
					for( int t = 1; t < usedTables; t++ ) {
						if( StrCaseCmp( fields[m].name, tables[t].alias ) == 0 ) {
							PVARTEXT pvtSafe = VarTextCreate();
							vtprintf( pvtSafe, "%s : %s", TranslateText( "Column name overlaps table alias" ), tables[t].alias );
							isolate->ThrowException( Exception::Error(
								String::NewFromUtf8( isolate, GetText( VarTextPeek( pvtSafe ) ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
							VarTextDestroy( &pvtSafe );
							DeleteDataList( &pdlParams );
							return;
						}
					}
				}
			Local<Array> records = Array::New( isolate );
			Local<Object> record;
			if( pdlRecord ) {
				int row = 0;
				do {
					Local<Value> val;
					tables[0].container = record = Object::New( isolate );
					if( usedTables > 1 && maxDepth > 1 )
						for( int n = 1; n < usedTables; n++ ) {
							tables[n].container = Object::New( isolate );
							SET( record, tables[n].alias, tables[n].container );
						}
					else
						for( int n = 0; n < usedTables; n++ )
							tables[n].container = record;

					DATA_FORALL( pdlRecord, idx, struct jsox_value_container *, jsval ) {
						if( jsval->value_type == JSOX_VALUE_UNDEFINED ) break;

						Local<Object> container = colMap[idx].t->container;
						if( fields[colMap[idx].col].used > 1 ) {
							if( fields[colMap[idx].col].first == idx ) {
								if( !jsval->name )
									lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s", GetText( statement ) );
								else
									SET( record, jsval->name
									           , fields[colMap[idx].col].array = Array::New( isolate )
									           );
							}
						}

						switch( jsval->value_type ) {
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
									, new ScriptOrigin( String::NewFromUtf8( isolate, "DateFormatter"
										, NewStringType::kInternalized ).ToLocalChecked() ) ).ToLocalChecked();
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
							if( jsval->float_result ) {
								val = Number::New( isolate, jsval->result_d );
							}
							else {
								val = Number::New( isolate, (double)jsval->result_n );
							}
							break;
						case JSOX_VALUE_STRING:
							if( !jsval->string )
								val = Null( isolate );
							else
								val = localString( isolate, (char*)Hold(jsval->string), (int)jsval->stringLen );
							break;
						case JSOX_VALUE_TYPED_ARRAY:
							//lprintf( "Should result with a binary thing" );

							Local<ArrayBuffer> ab =
								ArrayBuffer::New( isolate, (char*)Hold( jsval->string ), jsval->stringLen );

							PARRAY_BUFFER_HOLDER holder = GetHolder();
							holder->o.Reset( isolate, ab );
							holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
							holder->buffer = jsval->string;
							jsval->string = NULL; // steal this buffer, don't let DB release it.

							val = ab;
							break;
						}

						if( fields[colMap[idx].col].used == 1 ){
							if( !jsval->name )
								lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s", GetText( statement ) );
							else
								SET( container, jsval->name, val );
						}
						else if( usedTables > 1 || ( fields[colMap[idx].col].used > 1 ) ) {
							if( fields[colMap[idx].col].used > 1 ) {
								if( !jsval->name )
									lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s", GetText( statement ) );
								else
									SET( colMap[idx].t->container, jsval->name, val );
								if( colMap[idx].alias )
									SET( fields[colMap[idx].col].array, colMap[idx].alias, val );
								SETN( fields[colMap[idx].col].array, colMap[idx].depth, val );
							}
						}
					}
					SETN( records, row++, record );
				} while( FetchSQLRecordJS( sql->odbc, &pdlRecord ) );
			}
			Deallocate( struct fieldTypes*, fields );
			Deallocate( struct tables*, tables );
			Deallocate( struct colMap*, colMap );

			SQLEndQuery( sql->odbc );
			args.GetReturnValue().Set( records );
		}
		else
		{
			SQLEndQuery( sql->odbc );
			args.GetReturnValue().Set( Array::New( isolate ) );
		}
		DeleteDataList( &pdlParams );
	}
}

//-----------------------------------------------------------

SqlObject::SqlObject( const char *dsn )
{
	messages = NULL;
	userFunctions = NULL;
	thread = NULL;
	memset( &async, 0, sizeof( async ) );
	odbc = ConnectToDatabase( dsn );
	SetSQLThreadProtect( odbc, FALSE );
	//SetSQLAutoClose( odbc, TRUE );
	optionInitialized = FALSE;
}

SqlObject::~SqlObject() {
	INDEX idx;
	struct SqlObjectUserFunction *data;
	LIST_FORALL( userFunctions, idx, struct SqlObjectUserFunction*, data ) {
		data->cb.Reset();
	}
	if( thread )
	{
		struct userMessage msg;
		msg.mode = 0;
		msg.onwhat = NULL;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &messages, &msg );
		uv_async_send( &async );

		while( !msg.done ) {
			WakeableSleep( SLEEP_FOREVER );
		}
	}
	CloseDatabase( odbc );
}

//-----------------------------------------------------------

Persistent<Function> OptionTreeObject::constructor;
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
		Local<Function> cons = Local<Function>::New(isolate, constructor);
		args.GetReturnValue().Set(cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked());
	}
}


void OptionTreeObject::Init(  ) {
	Isolate* isolate = Isolate::GetCurrent();

	Local<FunctionTemplate> optionTemplate;
	// Prepare constructor template
	optionTemplate = FunctionTemplate::New( isolate, New );
	optionTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.option.node", v8::NewStringType::kNormal ).ToLocalChecked() );
	optionTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( optionTemplate, "eo", enumOptionNodes );
	NODE_SET_PROTOTYPE_METHOD( optionTemplate, "fo", findOptionNode );
	NODE_SET_PROTOTYPE_METHOD( optionTemplate, "go", getOptionNode );
	Local<Template> proto = optionTemplate->InstanceTemplate();

	proto->SetNativeDataProperty( String::NewFromUtf8( isolate, "value", v8::NewStringType::kNormal ).ToLocalChecked()
			, readOptionNode
			, writeOptionNode );

	//NODE_SET_PROTOTYPE_METHOD( optionTemplate, "ro", readOptionNode );
	//NODE_SET_PROTOTYPE_METHOD( optionTemplate, "wo", writeOptionNode );

	constructor.Reset( isolate, optionTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
}


void SqlObject::getOptionNode( const v8::FunctionCallbackInfo<Value>& args ) {
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

	String::Utf8Value tmp( USE_ISOLATE( isolate ) args[0] );
	char *optionPath = StrDup( *tmp );

	Local<Function> cons = Local<Function>::New( isolate, OptionTreeObject::constructor );
	MaybeLocal<Object> o = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL );
	args.GetReturnValue().Set( o.ToLocalChecked() );

	OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( o.ToLocalChecked() );
	oto->odbc = sqlParent->odbc;
	//lprintf( "SO Get %p ", sqlParent->odbc );
	oto->node =  GetOptionIndexExx( sqlParent->odbc, NULL, optionPath, NULL, NULL, NULL, TRUE, TRUE DBG_SRC );
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

	Local<Function> cons = Local<Function>::New( isolate, constructor );
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
	if( !sqlParent->optionInitialized ) {
		SetOptionDatabaseOption( sqlParent->odbc );
		sqlParent->optionInitialized = TRUE;
	}
	POPTION_TREE_NODE newNode = GetOptionIndexExx( sqlParent->odbc, NULL, optionPath, NULL, NULL, NULL, FALSE, TRUE DBG_SRC );

	if( newNode ) {
		Local<Function> cons = Local<Function>::New( isolate, OptionTreeObject::constructor );
		Local<Object> o;
		args.GetReturnValue().Set( o = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );

		OptionTreeObject *oto = ObjectWrap::Unwrap<OptionTreeObject>( o );
		oto->odbc = sqlParent->odbc;
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
		Local<Function> cons = Local<Function>::New( isolate, constructor );
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

	Local<Function> cons = OptionTreeObject::constructor.Get( args->isolate );
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
		if( !sqlParent->optionInitialized ) {
			SetOptionDatabaseOption( sqlParent->odbc );
			sqlParent->optionInitialized = TRUE;
		}
		callbackArgs.odbc = sqlParent->odbc;
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
		// use_odbc = NULL;
	} else 
	{
		SqlObject *sql = SqlObject::Unwrap<SqlObject>( args.This() );

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

	Local<String> returnval = String::NewFromUtf8( isolate, readbuf, v8::NewStringType::kNormal ).ToLocalChecked();
	args.GetReturnValue().Set( returnval );

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
		use_odbc = sql->odbc;
	}
	if( ( sect && sect[0] == '/' ) ) {
			SACK_WritePrivateOptionStringEx( use_odbc
			, NULL
			, optname
			, defaultVal
			, sect, FALSE );
	} 
	else
		SACK_WriteOptionString( use_odbc
			, sect
			, optname
			, defaultVal
		);

	Local<String> returnval = String::NewFromUtf8( isolate, readbuf, v8::NewStringType::kNormal ).ToLocalChecked();
	args.GetReturnValue().Set( returnval );

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
		if( CheckODBCTable( sql->odbc, table, CTO_MERGE ) )
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

static void sqlUserAsyncMsg( uv_async_t* handle ) {
	SqlObject* myself = (SqlObject*)handle->data;
	struct userMessage *msg = (struct userMessage*)DequeLink( &myself->messages );
	if( msg->onwhat ) {
		if( msg->mode == 1 )
			callUserFunction( msg->onwhat, msg->argc, msg->argv );
		else if( msg->mode == 2 )
			callAggStep( msg->onwhat, msg->argc, msg->argv );
		else if( msg->mode == 3 ) {
			callAggFinal( msg->onwhat );
			uv_close( (uv_handle_t*)&myself->async, NULL );
		}
	} else {
		myself->thread = NULL;
		uv_close( (uv_handle_t*)&myself->async, NULL );		
	}	
	msg->done = 1;
	WakeThread( msg->waiter );
}

static void releaseBuffer( void *buffer ) {
	Deallocate( void*, buffer );
}

void callUserFunction( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv ) {
	struct SqlObjectUserFunction *userData = (struct SqlObjectUserFunction*)PSSQL_GetSqliteFunctionData( onwhat );
	if( userData->sql->thread != MakeThread() ) {
		struct userMessage msg;
		msg.mode = 1;
		msg.onwhat = onwhat;
		msg.argc = argc;
		msg.argv = argv;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &userData->sql->messages, &msg );
		uv_async_send( &userData->sql->async );

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
				Local<Object> arrayBuffer = ArrayBuffer::New( userData->isolate, _data, len );
				PARRAY_BUFFER_HOLDER holder = GetHolder();
				holder->o.Reset( userData->isolate, arrayBuffer );
				holder->o.SetWeak< ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
				holder->buffer = _data;
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
			buf = (uint8_t*)myarr->GetContents().Data();
			length = myarr->ByteLength();
		} else if( type == 2 ) {
			Local<Uint8Array> _myarr = str.As<Uint8Array>();
			Local<ArrayBuffer> buffer = _myarr->Buffer();
			buf = (uint8_t*)buffer->GetContents().Data();
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
	Release( userData );
}

void SqlObject::userFunction( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	int argc = args.Length();
	if( !sql->thread ) {
		sql->thread = MakeThread();
		uv_async_init( uv_default_loop(), &sql->async, sqlUserAsyncMsg );
		sql->async.data = sql;
	}

	if( argc > 0 ) {
		String::Utf8Value name( USE_ISOLATE( isolate ) args[0] );
		struct SqlObjectUserFunction *userData = new SqlObjectUserFunction();
		userData->isolate = isolate;
		userData->cb.Reset( isolate, Local<Function>::Cast( args[1] ) );
		userData->sql = sql;
		PSSQL_AddSqliteFunction( sql->odbc, *name, callUserFunction, destroyUserData, -1, userData );
	}
}

void SqlObject::userProcedure( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	int argc = args.Length();
	if( !sql->thread ) {
		sql->thread = MakeThread();
		uv_async_init( uv_default_loop(), &sql->async, sqlUserAsyncMsg );
		sql->async.data = sql;
	}

	if( argc > 0 ) {
		String::Utf8Value name( USE_ISOLATE( isolate ) args[0] );
		struct SqlObjectUserFunction *userData = new SqlObjectUserFunction();
		userData->isolate = isolate;
		userData->cb.Reset( isolate, Local<Function>::Cast( args[1] ) );
		userData->sql = sql;
		PSSQL_AddSqliteProcedure( sql->odbc, *name, callUserFunction, destroyUserData, -1, userData );
	}
}

void callAggStep( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv ) {
	struct SqlObjectUserFunction *userData = ( struct SqlObjectUserFunction* )PSSQL_GetSqliteFunctionData( onwhat );
	if( userData->sql->thread != MakeThread() ) {
		struct userMessage msg;
		msg.mode = 2;
		msg.onwhat = onwhat;
		msg.argc = argc;
		msg.argv = argv;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &userData->sql->messages, &msg );
		uv_async_send( &userData->sql->async );

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
				Local<Object> arrayBuffer = ArrayBuffer::New( userData->isolate, _data, len );
				PARRAY_BUFFER_HOLDER holder = GetHolder();
				holder->o.Reset( userData->isolate, arrayBuffer );
				holder->o.SetWeak< ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
				holder->buffer = _data;
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
	if( userData->sql->thread != MakeThread() ) {
		struct userMessage msg;
		msg.mode = 3;
		msg.onwhat = onwhat;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &userData->sql->messages, &msg );
		uv_async_send( &userData->sql->async );

		while( !msg.done ) {
			WakeableSleep( SLEEP_FOREVER );
		}

		return;
	} else {
		struct userMessage msg;
		msg.mode = 3;
		msg.onwhat = NULL;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &userData->sql->messages, &msg );
		uv_async_send( &userData->sql->async );
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
			buf = (uint8_t*)myarr->GetContents().Data();
			length = myarr->ByteLength();
		} else if( type == 2 ) {
			Local<Uint8Array> _myarr = str.As<Uint8Array>();
			Local<ArrayBuffer> buffer = _myarr->Buffer();
			buf = (uint8_t*)buffer->GetContents().Data();
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

void SqlObject::getLogging( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	args.GetReturnValue().Set( False( isolate ) );
}

void SqlObject::setLogging( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	if( args.Length() ) {
		bool b( args[0]->TOBOOL( isolate ) );
		if( b )
			SetSQLLoggingDisable( sql->odbc, FALSE );
		else
			SetSQLLoggingDisable( sql->odbc, TRUE );
	}
}


void SqlObject::error( const v8::FunctionCallbackInfo<Value>& args ) {
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	const char *error;
	FetchSQLError( sql->odbc, &error );
	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), error, NewStringType::kNormal, (int)strlen(error) ).ToLocalChecked() );

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
	SetSQLCorruptionHandler( sql->odbc, handleCorruption, (uintptr_t)sql );

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
		PSSQL_AddSqliteAggregate( sql->odbc, *name, callAggStep, callAggFinal, destroyUserData, -1, userData );

		if( !sql->thread ) {
			sql->thread = MakeThread();
			uv_async_init( uv_default_loop(), &sql->async, sqlUserAsyncMsg );
			sql->async.data = sql;
		}
	}
	else {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Aggregate requires 3 parameters (name,stepCallback(...args), finalCallback())"), v8::NewStringType::kNormal ).ToLocalChecked() ) );
	}
}

#ifdef INCLUDE_GUI
static uintptr_t RunEditor( PTHREAD thread ) {
	int (*EditOptions)( PODBC odbc, PSI_CONTROL parent, LOGICAL wait );
	extern void disableEventLoop( void );
	EditOptions = (int(*)(PODBC,PSI_CONTROL,LOGICAL))GetThreadParam( thread );
	EditOptions( NULL, NULL, TRUE );
	disableEventLoop();
	return 0;
}

void editOptions( const v8::FunctionCallbackInfo<Value>& args ){
	int (*EditOptions)( PODBC odbc, PSI_CONTROL parent, LOGICAL wait );
	extern void enableEventLoop( void );
#ifdef WIN32
	LoadFunction( "bag.psi.dll", NULL );
#else
	LoadFunction( "libbag.psi.so", NULL );
#endif
	EditOptions = (int(*)( PODBC, PSI_CONTROL,LOGICAL))LoadFunction( "EditOptions.plugin", "EditOptionsEx" );
	if( EditOptions ) {
		enableEventLoop();
		ThreadTo( RunEditor, (uintptr_t)EditOptions );
	} else
		lprintf( "Failed to load editor..." );
}
#endif
