
#include "global.h"
#include <math.h>

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
static void buildObject( PDATALIST msg_data, Local<Object> o, Isolate *isolate );
static Local<Value> makeValue( Isolate *isolate, struct json_value_container *val );


static void makeJSON( const v8::FunctionCallbackInfo<Value>& args );
static void parseJSON( const v8::FunctionCallbackInfo<Value>& args );
static void makeJSON6( const v8::FunctionCallbackInfo<Value>& args );
static void parseJSON6( const v8::FunctionCallbackInfo<Value>& args );
static void beginJSON6( const v8::FunctionCallbackInfo<Value>& args );
static void writeJSON6( const v8::FunctionCallbackInfo<Value>& args );
static void endJSON6( const v8::FunctionCallbackInfo<Value>& args );


class parseObject : public node::ObjectWrap {
	struct json_parse_state *state;
public:
	static Persistent<Function> constructor;
	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //

public:

	static void Init( Handle<Object> exports );
	parseObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void write( const v8::FunctionCallbackInfo<Value>& args );


	~parseObject();
};

Persistent<Function> parseObject::constructor;

void InitJSON( Isolate *isolate, Handle<Object> exports ){
	Local<Object> o = Object::New( isolate );
	NODE_SET_METHOD( o, "parse", parseJSON );
	NODE_SET_METHOD( o, "stringify", makeJSON );
	exports->Set( String::NewFromUtf8( isolate, "JSON" ), o );

	Local<Object> o2 = Object::New( isolate );
	NODE_SET_METHOD( o2, "parse", parseJSON6 );
	NODE_SET_METHOD( o2, "stringify", makeJSON6 );
	exports->Set( String::NewFromUtf8( isolate, "JSON6" ), o2 );

	{
		Local<FunctionTemplate> parseTemplate;
		parseTemplate = FunctionTemplate::New( isolate, parseObject::New );
		parseTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.json6_parser" ) );
		parseTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "write", parseObject::write );

		parseObject::constructor.Reset( isolate, parseTemplate->GetFunction() );

		o2->Set( String::NewFromUtf8( isolate, "begin" ),
			parseTemplate->GetFunction() );

	}
}

parseObject::parseObject() {
	state = json6_begin_parse();
}

parseObject::~parseObject() {
	json6_parse_dispose_state( &state );
}

void parseObject::write(const v8::FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	parseObject *parser = ObjectWrap::Unwrap<parseObject>( args.Holder() );
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Missing data parameter." ) ) );
		return;
	}

	String::Utf8Value data( args[0]->ToString() );
	int result;
	for( result = json6_parse_add_data( parser->state, *data, data.length() );
		result > 0;
		result = json6_parse_add_data( parser->state, NULL, 0 )
		) {
		struct json_value_container * val;
		PDATALIST elements = json6_parse_get_data( parser->state );
		Local<Object> o;
		Local<Value> v;// = Object::New( isolate );

		val = (struct json_value_container *)GetDataItem( &elements, 0 );
		if( val && val->contains ) {
			if( val->value_type == VALUE_OBJECT )
				o = Object::New( isolate );
			else if( val->value_type == VALUE_ARRAY )
				o = Array::New( isolate );
			else
				lprintf( "Value has contents, but is not a container type?!" );
			buildObject( val->contains, o, isolate );
		}
		else if( val ) {
			//lprintf( "was just a single, simple value type..." );
			v = makeValue( isolate, val );
			// this is illegal json
			// cannot result from a simple value?	
		}
		else {
			json6_dispose_message( &elements );
			return;
		}

		Local<Value> argv[1];
		if( !o.IsEmpty() )
			argv[0] = o;
		else argv[0] = v;

		Local<Function> cb = Local<Function>::New( isolate, parser->readCallback );
		//lprintf( "callback ... %p", myself );
		// using obj->jsThis  fails. here...
		cb->Call( isolate->GetCurrentContext()->Global(), 1, argv );

		json6_dispose_message( &elements );
	}
	if( result < 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Invalid JSON passed" ) ) );
		json6_parse_clear_state( parser->state );
		return;
	}

}

void parseObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must specify port name to open." ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		parseObject* obj = new parseObject();
		Handle<Function> arg0 = Handle<Function>::Cast( args[0] );
		Persistent<Function> cb( isolate, arg0 );
		obj->readCallback = cb;

		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
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
		return String::NewFromUtf8( isolate, val->string );
	case VALUE_NUMBER:
		if( val->float_result )
			return Number::New( isolate, val->result_d );
		else
			return Integer::New( isolate, (int32_t)val->result_n );
	case VALUE_ARRAY:
		return Array::New( isolate );
	case VALUE_OBJECT:
		return Object::New( isolate );
	case VALUE_NEG_NAN:
		return Number::New(isolate, -NAN);
	case VALUE_NAN:
		return Number::New(isolate, NAN);
	case VALUE_NEG_INFINITY:
		return Number::New(isolate, -INFINITY);
	case VALUE_INFINITY:
		return Number::New(isolate, INFINITY);
	
	}

}

static void buildObject( PDATALIST msg_data, Local<Object> o, Isolate *isolate ) {

	struct json_value_container *val;
	Local<Object> sub_o;
	INDEX idx;
	int index = 0;
	DATA_FORALL( msg_data, idx, struct json_value_container*, val )
	{
	switch( val->value_type ) {
	default:
		if( val->name )
			o->Set( String::NewFromUtf8( isolate, val->name )
				, makeValue( isolate, val ) );
		else
			o->Set( (uint32_t)idx, makeValue( isolate, val ) );
		break;
	case VALUE_ARRAY:
			o->Set( String::NewFromUtf8( isolate, val->name )
						, sub_o = Array::New( isolate )
			 );
			buildObject( val->contains, sub_o, isolate );
			break;
	case VALUE_OBJECT:
			if( val->name )
				o->Set( String::NewFromUtf8( isolate,val->name )
							, sub_o = Object::New( isolate )
				 );
			else
				o->Set( index++, sub_o = Object::New( isolate )
				 );

			buildObject( val->contains, sub_o, isolate );
			break;
		}
	}
}


Local<Value> ParseJSON(  Isolate *isolate, const char *utf8String, size_t len) {
	PDATALIST parsed = NULL;
	Local<Object> o;// = Object::New( isolate );
	Local<Value> v;// = Object::New( isolate );
	json_parse_message( (char*)utf8String, len, &parsed );
	if( parsed->Cnt > 1 ) {
		lprintf( "Multiple values would result, invalid parse." );
		return Undefined(isolate);
		// outside should always be a single value
	}

	struct json_value_container *val;
	val = (struct json_value_container *)GetDataItem( &parsed, 0 );
	if( val && val->contains ) {
		if( val->value_type == VALUE_OBJECT )
			o = Object::New( isolate );
		else if( val->value_type == VALUE_ARRAY )
			o = Array::New( isolate );
		else
			lprintf( "Value has contents, but is not a container type?!" );
		buildObject( val->contains, o, isolate );
	}
	else if( val ) {
		//lprintf( "was just a single, simple value type..." );
		v = makeValue( isolate, val );
		// this is illegal json
		// cannot result from a simple value?	
	}

	json_dispose_message( &parsed );
	if( !o.IsEmpty() ) 
		return o;
	return v;
}

void parseJSON( const v8::FunctionCallbackInfo<Value>& args )
{
	Isolate* isolate = Isolate::GetCurrent();
	const char *msg;
	String::Utf8Value tmp( args[0] );
	msg = *tmp;
	Local<Value> val = ParseJSON( isolate, msg, strlen( msg ) );
	args.GetReturnValue().Set( val );

}


void makeJSON( const v8::FunctionCallbackInfo<Value>& args ) {
	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), "undefined :) Stringify is not completed" ) );
}

Local<Value> ParseJSON6(  Isolate *isolate, const char *utf8String, size_t len) {
	PDATALIST parsed = NULL;
	Local<Object> o;// = Object::New( isolate );
	Local<Value> v;// = Object::New( isolate );
	json6_parse_message( (char*)utf8String, len, &parsed );
	if( parsed && parsed->Cnt > 1 ) {
		lprintf( "Multiple values would result, invalid parse." );
		return Undefined(isolate);
		// outside should always be a single value
	}

	struct json_value_container *val;
	val = (struct json_value_container *)GetDataItem( &parsed, 0 );
	if( val && val->contains ) {
		if( val->value_type == VALUE_OBJECT )
			o = Object::New( isolate );
		else if( val->value_type == VALUE_ARRAY )
			o = Array::New( isolate );
		else
			lprintf( "Value has contents, but is not a container type?!" );
		buildObject( val->contains, o, isolate );
	}
	else if( val ) {
		//lprintf( "was just a single, simple value type..." );
		v = makeValue( isolate, val );
		// this is illegal json
		// cannot result from a simple value?	
	}

	json6_dispose_message( &parsed );
	if( !o.IsEmpty() ) 
		return o;
	return v;
}

void parseJSON6( const v8::FunctionCallbackInfo<Value>& args )
{
	Isolate* isolate = Isolate::GetCurrent();
	const char *msg;
	String::Utf8Value tmp( args[0] );
	msg = *tmp;
	Local<Value> val = ParseJSON6( isolate, msg, strlen( msg ) );
	args.GetReturnValue().Set( val );

}


void makeJSON6( const v8::FunctionCallbackInfo<Value>& args ) {
	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), "undefined :) Stringify is not completed" ) );
}
