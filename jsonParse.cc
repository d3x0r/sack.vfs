
#include "global.h"


static char const * const  JSON_ObjectElementTypeStrings[] =
{
   "JSON_Element_Integer_8",
   "JSON_Element_Integer_16",
   "JSON_Element_Integer_32",
   "JSON_Element_Integer_64",
   "JSON_Element_Unsigned_Integer_8",
   "JSON_Element_Unsigned_Integer_16",
   "JSON_Element_Unsigned_Integer_32",
   "JSON_Element_Unsigned_Integer_64",
   "JSON_Element_String",
   "JSON_Element_CharArray",
   "JSON_Element_Float",
   "JSON_Element_Double",
   "JSON_Element_Array",  // result will fill a PLIST
   "JSON_Element_Object",
   "JSON_Element_ObjectPointer",
   "JSON_Element_List",
   "JSON_Element_Text",  // ptext type
   "JSON_Element_PTRSZVAL",  
   "JSON_Element_PTRSZVAL_BLANK_0",
	"JSON_Element_UserRoutine",
	"JSON_Element_Raw_Object", // unparsed object remainder.  Includes bounding { } object indicator for re-parsing
   //JSON_Element_StaticText,  // text type; doesn't happen very often.
};


static char const * const json_value_type_strings_[] = {
	"VALUE_UNDEFINED"
	, "uninitialized"
	, "VALUE_NULL"
	, "VALUE_TRUE"
	, "VALUE_FALSE"
	, "VALUE_STRING"
	, "VALUE_NUMBER"
	, "VALUE_OBJECT"
	, "VALUE_ARRAY"
};

static char const *const *const json_value_type_strings = json_value_type_strings_+1;


static void makeJSON( const FunctionCallbackInfo<Value>& args );
static void parseJSON( const FunctionCallbackInfo<Value>& args );
	
void InitJSON( Isolate *isolate, Handle<Object> exports ){
	Local<Object> o = Object::New( isolate );
	NODE_SET_METHOD( o, "parse", parseJSON );
	NODE_SET_METHOD( o, "stringify", makeJSON );
	exports->Set( String::NewFromUtf8( isolate, "JSON" ), o );
}

static Local<Value> makeValue( Isolate *isolate, struct json_value_container *val ) {

	switch( val->value_type ) {
	case VALUE_UNDEFINED:
		return Undefined( isolate );
	default:
		lprintf( "Parser faulted; should never have a uninitilized value." );
		return Undefined( isolate );
	case VALUE_NULL:
		return Null( isolate );
	case VALUE_TRUE:
		return True( isolate );
	case VALUE_FALSE:
		return False( isolate );
	case VALUE_STRING:
		return String::NewFromUtf8( isolate, GetText( val->string ) );
	case VALUE_NUMBER:
		if( val->float_result )
			return Number::New( isolate, val->result_d );
		else
			return Integer::New( isolate, (int32_t)val->result_n );
	case VALUE_ARRAY:
		return Array::New( isolate );
	case VALUE_OBJECT:
		return Object::New( isolate );
	}

}

static void buildObject( PDATALIST msg_data, Local<Object> o, Isolate *isolate ) {

	struct json_value_container *val;
	Local<Object> sub_o;
	INDEX idx;
	DATA_FORALL( msg_data, idx, struct json_value_container*, val )
	{
	switch( val->result_value ) {
	default:
		if( val->name )
			o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
   				, makeValue( isolate, val ) );
		else
			o->Set( (uint32_t)idx, makeValue( isolate, val ) );
		break;
/*		
	case VALUE_UNDEFINED:
		// skip undefined values (even if literally 'undefined' input
		break;
	case uninitialized:
		lprintf( "Parser faulted; should never have a uninitilized value." );
		break;
	case VALUE_NULL:
			 );
			break;
	case VALUE_TRUE:
			o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, True( isolate )
			 );
			break;
	case VALUE_FALSE:
			o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, False( isolate )
			 );
			break;
	case VALUE_STRING:
			o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, String::NewFromUtf8( isolate, GetText( val->string ) )
			 );
			break;
	case VALUE_NUMBER:
			if( val->float_result )
				o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, Number::New( isolate, val->result_d )
					);
			else
				o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, Number::New( isolate, val->result_n )
					 );
			break;
*/			
	case VALUE_ARRAY:
			o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, sub_o = Array::New( isolate )
			 );
			buildObject( val->contains, sub_o, isolate );
			break;
	case VALUE_OBJECT:
			o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, sub_o = Object::New( isolate )
			 );
			buildObject( val->contains, sub_o, isolate );
			break;
		}
	}
}


static void internal_json_dispose_message( PDATALIST *msg_data )
{
	struct json_value_container *val;
	INDEX idx;
	DATA_FORALL( (*msg_data), idx, struct json_value_container*, val )
	{
		if( val->name ) LineRelease( val->name );
		if( val->string ) LineRelease( val->string );
		if( val->value_type == VALUE_OBJECT )
			internal_json_dispose_message( &val->contains );
	}
	// quick method
	DeleteDataList( msg_data );

}


Local<Value> ParseJSON(  Isolate *isolate, const char *utf8String, size_t len) {
	PDATALIST parsed = NULL;
	Local<Object> o;// = Object::New( isolate );
	Local<Value> v;// = Object::New( isolate );
	json_parse_message( utf8String, len, &parsed );
	if( parsed->Cnt > 1 ) {
		lprintf( "Multiple values would result, invalid parse." );
		return Undefined(isolate);
		// outside should always be a single value
	}

	struct json_value_container *val;
	val = (struct json_value_container *)GetDataItem( &parsed, 0 );
	if( val && val->contains ) {
		val = (struct json_value_container *)GetDataItem( &val->contains, 0 );
		if( val->name )
			o = Object::New(isolate);
		else
			o = Array::New(isolate);
		buildObject( parsed, o, isolate );
	}
	else if( val ) {
		v = makeValue( isolate, val );
		// this is illegal json
		// cannot result from a simple value?	
	}

	internal_json_dispose_message( &parsed );
	if( !o.IsEmpty() ) 
		return o;
	return v;
}

void parseJSON( const FunctionCallbackInfo<Value>& args )
{
	Isolate* isolate = Isolate::GetCurrent();
	const char *msg;
	String::Utf8Value tmp( args[0] );
	msg = *tmp;
	Local<Value> val = ParseJSON( isolate, msg, strlen( msg ) );
	args.GetReturnValue().Set( val );

}


void makeJSON( const FunctionCallbackInfo<Value>& args ) {
	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), "undefined :)" ) );
}
