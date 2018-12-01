
#include "global.h"

#ifdef INCLUDE_GUI
void editOptions( const v8::FunctionCallbackInfo<Value>& args );
#endif

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

	Local<FunctionTemplate> sqlStmtTemplate;
	// Prepare constructor template
	sqlStmtTemplate = FunctionTemplate::New( isolate, SqlStmtObject::New );
	sqlStmtTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.Sqlite.statement" ) );
	sqlStmtTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
	SqlStmtObject::constructor.Reset( isolate, sqlStmtTemplate->GetFunction() );

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "do", query );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "escape", escape );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "unescape", unescape );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "encode", escape );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "decode", unescape );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "end", closeDb );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "close", closeDb );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "transaction", transact );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "commit", commit );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "autoTransact", autoTransact );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "procedure", userProcedure );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "function", userFunction );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "aggregate", aggregateFunction );
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "onCorruption", setOnCorruption );


	// read a portion of the tree (passed to a callback)
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "eo", enumOptionNodes );
	// get without create
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "fo", findOptionNode );
	// get the node.
	NODE_SET_PROTOTYPE_METHOD( sqlTemplate, "go", getOptionNode );

	sqlTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "error" )
			, FunctionTemplate::New( isolate, SqlObject::error )
			, Local<FunctionTemplate>() );
	sqlTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "log" )
		, FunctionTemplate::New( isolate, SqlObject::getLogging )
		, FunctionTemplate::New( isolate, SqlObject::setLogging ) );

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

	SET_READONLY_METHOD(sqlfunc, "eo", enumOptionNodesInternal );
	SET_READONLY_METHOD(sqlfunc, "op", optionInternal );
	SET_READONLY_METHOD(sqlfunc, "so", setOptionInternal );
#ifdef INCLUDE_GUI
	SET_READONLY_METHOD(sqlfunc, "optionEditor", editOptions );
#endif

	exports->Set( String::NewFromUtf8( isolate, "Sqlite" ),
		sqlfunc );
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

	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	SetSQLAutoTransact( sql->odbc, args[0]->BooleanValue() );
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
			String::NewFromUtf8( isolate, TranslateText( "Required parameters (column, new value) are missing." ) ) ) );
		return;
	}
	int col = args[0]->ToInt32(isolate)->Value();
	struct json_value_container val;
	memset( &val, 0, sizeof( val ) );
	int arg = 1;
	if( args[arg]->IsInt32() ) {
		val.value_type = VALUE_NUMBER;
		val.result_n = args[arg]->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		val.float_result = 0;
		SetDataItem( &stmt->values, col, &val );
	} else if( args[arg]->IsNumber() ) {
		val.value_type = VALUE_NUMBER;
		val.result_d = args[arg]->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		val.float_result = 0;
		SetDataItem( &stmt->values, col, &val );
	} else if( args[arg]->IsArrayBuffer() ) {
	} else if( args[arg]->IsInt8Array() ) {
	} else if( args[arg]->IsTypedArray() ) {
	}
}

static void PushValue( Isolate *isolate, PDATALIST *pdlParams, Local<Value> arg, String::Utf8Value *name ) {
	struct json_value_container val;
	if( name ) {
		val.name = DupCStrLen( *name[0], val.nameLen = name[0].length() );
	}
	else {
		val.name = NULL;
		val.nameLen = 0;
	}
	if( arg->IsString() ) {
		String::Utf8Value text( arg->ToString() );
		val.value_type = VALUE_STRING;
		val.string = DupCStrLen( *text, val.stringLen = text.length() );
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsInt32() ) {
		val.value_type = VALUE_NUMBER;
		val.result_n = arg->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		val.float_result = 0;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsBoolean() ) {
		val.value_type = VALUE_NUMBER;
		val.result_n = arg->BooleanValue( isolate->GetCurrentContext() ).FromMaybe(false);
		val.float_result = 0;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsNumber() ) {
		val.value_type = VALUE_NUMBER;
		val.result_d = arg->Int32Value( isolate->GetCurrentContext() ).FromMaybe( 0 );
		val.float_result = 1;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsArrayBuffer() ) {
		Local<ArrayBuffer> myarr = arg.As<ArrayBuffer>();
		val.string = (char*)myarr->GetContents().Data();
		val.stringLen = myarr->ByteLength();
		val.value_type = VALUE_TYPED_ARRAY;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsUint8Array() ) {
		Local<Uint8Array> _myarr = arg.As<Uint8Array>();
		Local<ArrayBuffer> buffer = _myarr->Buffer();
		val.string = (char*)buffer->GetContents().Data();
		val.stringLen = buffer->ByteLength();
		val.value_type = VALUE_TYPED_ARRAY;
		AddDataItem( pdlParams, &val );
	}
	else if( arg->IsTypedArray() ) {
		Local<TypedArray> _myarr = arg.As<TypedArray>();
		Local<ArrayBuffer> buffer = _myarr->Buffer();
		val.string = (char*)buffer->GetContents().Data();
		val.stringLen = buffer->ByteLength();
		val.value_type = VALUE_TYPED_ARRAY;
		AddDataItem( pdlParams, &val );
	}
	else {
		lprintf( "Unsupported TYPE" );
	}

}

void SqlObject::query( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() == 0 ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Required parameter, SQL query, is missing.") ) ) );
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
		struct json_value_container val;
		memset( &val, 0, sizeof( val ) );
		if( StrChr( *sqlStmt, ':' )
			|| StrChr( *sqlStmt, '@' )
			|| StrChr( *sqlStmt, '$' ) ) {
			if( args[1]->IsObject() ) {
				arg = 2;
				pdlParams = CreateDataList( sizeof( struct json_value_container ) );
				Local<Object> params = Local<Object>::Cast( args[1] );
				Local<Array> paramNames = params->GetOwnPropertyNames();
				for( int p = 0; p < paramNames->Length(); p++ ) {
					Local<Value> valName = paramNames->Get( p );
					Local<Value> value = params->Get( valName );
					String::Utf8Value name( valName->ToString() );
					PushValue( isolate, &pdlParams, value, &name );
				}
			}
			else {
				isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText( "Required parameter 2, Named Paramter Object, is missing." ) ) ) );
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
			pdlParams = CreateDataList( sizeof( struct json_value_container ) );
		if( !isFormatString ) {
			for( ; arg < args.Length(); arg++ ) {
				if( args[arg]->IsString() ) {
					String::Utf8Value text( args[arg]->ToString() );
					if( arg & 1 ) { // every odd parameter is inserted
						val.value_type = VALUE_STRING;
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
		sql->fields = 0;
		if( !SQLRecordQuery_v4( sql->odbc, GetText(statement), GetTextSize(statement), &sql->columns, &sql->result, &sql->resultLens, &sql->fields, pdlParams DBG_SRC ) ) {
			const char *error;
			FetchSQLError( sql->odbc, &error );
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, error ) ) );
			DeleteDataList( &pdlParams );
			return;
		}
		if( sql->columns )
		{
			int usedFields = 0;
			int maxDepth = 0;
			struct fieldTypes {
				const char *name;
				int used;
				int first;
				Local<Array> array;
			} *fields = NewArray( struct fieldTypes, sql->columns ) ;
			int usedTables = 0;
			struct tables {
				const char *table;
				const char *alias;
				Local<Object> container;
			}  *tables = NewArray( struct tables, sql->columns + 1);
			struct colMap {
				int depth;
				int col;
				const char *table;
				const char *alias;
				Local<Object> container;
				struct tables *t;
			}  *colMap = NewArray( struct colMap, sql->columns );
			tables[usedTables].table = NULL;
			tables[usedTables].alias = NULL;
			usedTables++;

			for( int n = 0; n < sql->columns; n++ ) {
				int m;
				for( m = 0; m < usedFields; m++ ) {
					if( StrCaseCmp( fields[m].name, sql->fields[n] ) == 0 ) {
						colMap[n].col = m;
						colMap[n].depth = fields[m].used;
						if( colMap[n].depth > maxDepth )
							maxDepth = colMap[n].depth+1;
						colMap[n].alias = PSSQL_GetColumnTableAliasName( sql->odbc, n );
						int table;
						for( table = 0; table < usedTables; table++ ) {
							if( StrCmp( tables[table].alias, colMap[n].alias ) == 0 ) {
								colMap[n].t = tables + table;
								break;
							}
						}
						if( table == usedTables ) {
							tables[table].table = colMap[n].table;
							tables[table].alias = colMap[n].alias;
							colMap[n].t = tables + table;
							usedTables++;
						}
						fields[m].used++;
						break;
					}
				}
				if( m == usedFields ) {
					colMap[n].col = m;
					colMap[n].depth = 0;
					colMap[n].table = PSSQL_GetColumnTableName( sql->odbc, n );
					colMap[n].alias = PSSQL_GetColumnTableAliasName( sql->odbc, n );
					if( colMap[n].table && colMap[n].alias ) {
						int table;
						for( table = 0; table < usedTables; table++ ) {
							if( StrCmp( tables[table].alias, colMap[n].alias ) == 0 ) {
								colMap[n].t = tables + table;
								break;
							}
						}
						if( table == usedTables ) {
							tables[table].table = colMap[n].table;
							tables[table].alias = colMap[n].alias;
							colMap[n].t = tables + table;
							usedTables++;
						}
					} else
						colMap[n].t = tables;
					fields[usedFields].first = n;
					fields[usedFields].name = sql->fields[n];
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
								String::NewFromUtf8( isolate, GetText( VarTextPeek( pvtSafe ) ) ) ) );
							VarTextDestroy( &pvtSafe );
							DeleteDataList( &pdlParams );
							return;
						}
					}
				}
			Local<Array> records = Array::New( isolate );
			Local<Object> record;
			if( sql->result ) {
				int row = 0;
				do {
					Local<Value> val;
					tables[0].container = record = Object::New( isolate );
					if( usedTables > 1 && maxDepth > 1 )
						for( int n = 1; n < usedTables; n++ ) {
							tables[n].container = Object::New( isolate );
							record->Set( String::NewFromUtf8( isolate, tables[n].alias ), tables[n].container );
						}
					else
						for( int n = 0; n < usedTables; n++ )
							tables[n].container = record;
					for( int n = 0; n < sql->columns; n++ ) {
						Local<Object> container = colMap[n].t->container;
						if( fields[colMap[n].col].used > 1 ) {
							if( fields[colMap[n].col].first == n ) {
								record->Set( String::NewFromUtf8( isolate, sql->fields[n] )
										  , fields[colMap[n].col].array = Array::New( isolate )
										  );
							}
						}

						if( sql->result[n] ) {
							double f;
							int64_t i;
							int type = IsTextAnyNumber( sql->result[n], &f, &i );
							if( fields[colMap[n].col].used > 1 ) {

							}
							if( type == 2 )
								val = Number::New( isolate, f );
							else if( type == 1 )
								val = Number::New( isolate, (double)i );
							else
								val = String::NewFromUtf8( isolate, sql->result[n], NewStringType::kNormal, (int)sql->resultLens[n] ).ToLocalChecked();
						}
						else
							val = Null(isolate);

						if( fields[colMap[n].col].used == 1 )
							container->Set( String::NewFromUtf8( isolate, sql->fields[n] ), val );
						else if( usedTables > 1 || ( fields[colMap[n].col].used > 1 ) ) {
							if( fields[colMap[n].col].used > 1 ) {
								colMap[n].t->container->Set( String::NewFromUtf8( isolate, sql->fields[n] ), val );
								if( colMap[n].alias )
									fields[colMap[n].col].array->Set( String::NewFromUtf8( isolate, colMap[n].alias ), val );
								fields[colMap[n].col].array->Set( colMap[n].depth, val );
							}
						}

					}
					records->Set( row++, record );
				} while( FetchSQLRecord( sql->odbc, &sql->result ) );
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
			args.GetReturnValue().Set( True( isolate ) );
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
	argv[1] = String::NewFromUtf8( args->isolate, name );

	MaybeLocal<Value> r = args->cb->Call(Null(args->isolate), 2, argv );
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
	Handle<Function> arg0 = Handle<Function>::Cast( args[0] );
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
	Handle<Function> arg0 = Handle<Function>::Cast( args[0] );
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
	info.GetReturnValue().Set( String::NewFromUtf8( info.GetIsolate(), buffer ) );
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
		sql->fields = 0;
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

	Local<String> returnval = String::NewFromUtf8( isolate, readbuf );
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
	Local<Value> str = cb->Call( userData->sql->handle(), argc, args );
	String::Utf8Value result( USE_ISOLATE( userData->isolate ) str->ToString() );
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
			PSSQL_ResultSqliteInt( onwhat, (int)str->IntegerValue() );
		else
			PSSQL_ResultSqliteDouble( onwhat, str->NumberValue() );
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
		struct SqlObjectUserFunction *userData = NewArray( struct SqlObjectUserFunction, 1 );
		memset( userData, 0, sizeof( userData[0] ) );
		userData->isolate = isolate;
		userData->cb.Reset( isolate, Handle<Function>::Cast( args[1] ) );
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
		struct SqlObjectUserFunction *userData = NewArray( struct SqlObjectUserFunction, 1 );
		memset( userData, 0, sizeof( userData[0] ) );
		userData->isolate = isolate;
		userData->cb.Reset( isolate, Handle<Function>::Cast( args[1] ) );
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
	cb->Call( userData->sql->handle(), argc, args );
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
			PSSQL_ResultSqliteInt( onwhat, (int)str->IntegerValue() );
		else
			PSSQL_ResultSqliteDouble( onwhat, str->NumberValue() );
	} else if( str->IsString() ) {
		String::Utf8Value result( USE_ISOLATE( userData->isolate) str->ToString() );
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
		Local<Boolean> b( args[0]->ToBoolean() );
		if( b->Value () )
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
	cb->Call( sql->_this.Get( isolate ), 0, 0 );
}


void SqlObject::setOnCorruption( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	int argc = args.Length();
	sql->onCorruption.Reset( isolate, Handle<Function>::Cast( args[0] ) );
	SetSQLCorruptionHandler( sql->odbc, handleCorruption, (uintptr_t)sql );

}

void SqlObject::aggregateFunction( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	SqlObject *sql = ObjectWrap::Unwrap<SqlObject>( args.This() );
	int argc = args.Length();

	if( argc > 2 ) {
		String::Utf8Value name( USE_ISOLATE( isolate ) args[0] );
		struct SqlObjectUserFunction *userData = NewArray( struct SqlObjectUserFunction, 1 );
		memset( userData, 0, sizeof( userData[0] ) );
		userData->isolate = isolate;
		userData->cb.Reset( isolate, Handle<Function>::Cast( args[1] ) );
		userData->cb2.Reset( isolate, Handle<Function>::Cast( args[2] ) );
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
			String::NewFromUtf8( isolate, TranslateText( "Aggregate requires 3 parameters (name,stepCallback(...args), finalCallback())") ) ) );
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
	EditOptions = (int(*)( PODBC, PSI_CONTROL,LOGICAL))LoadFunction( "EditOptions.plugin", "EditOptionsEx" );
	if( EditOptions ) {
		enableEventLoop();
		ThreadTo( RunEditor, (uintptr_t)EditOptions );
	} else
		lprintf( "Failed to load editor..." );
}
#endif
