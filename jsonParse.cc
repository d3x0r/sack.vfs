
#include "global.h"
#include <math.h>

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
	static void New6( const v8::FunctionCallbackInfo<Value>& args );
	static void write6( const v8::FunctionCallbackInfo<Value>& args );

	~parseObject();
};

Persistent<Function> parseObject::constructor;

void InitJSON( Isolate *isolate, Handle<Object> exports ){
	Local<Object> o = Object::New( isolate );
	NODE_SET_METHOD( o, "parse", parseJSON );
	NODE_SET_METHOD( o, "stringify", makeJSON );
	exports->Set( String::NewFromUtf8( isolate, "JSON" ), o );

	{
		Local<FunctionTemplate> parseTemplate;
		parseTemplate = FunctionTemplate::New( isolate, parseObject::New );
		parseTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.json6_parser" ) );
		parseTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "write", parseObject::write );

		parseObject::constructor.Reset( isolate, parseTemplate->GetFunction() );

		o->Set( String::NewFromUtf8( isolate, "begin" ),
			parseTemplate->GetFunction() );

	}

	Local<Object> o2 = Object::New( isolate );
	NODE_SET_METHOD( o2, "parse", parseJSON6 );
	NODE_SET_METHOD( o2, "stringify", makeJSON6 );
	exports->Set( String::NewFromUtf8( isolate, "JSON6" ), o2 );


	{
		Local<FunctionTemplate> parseTemplate;
		parseTemplate = FunctionTemplate::New( isolate, parseObject::New6 );
		parseTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.json6_parser" ) );
		parseTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "write", parseObject::write6 );

		parseObject::constructor.Reset( isolate, parseTemplate->GetFunction() );

		o2->Set( String::NewFromUtf8( isolate, "begin" ),
			parseTemplate->GetFunction() );

	}
}

parseObject::parseObject() {
	state = json_begin_parse();
}

parseObject::~parseObject() {
	json_parse_dispose_state( &state );
}

void parseObject::write( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	parseObject *parser = ObjectWrap::Unwrap<parseObject>( args.Holder() );
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Missing data parameter." ) ) );
		return;
	}

	String::Utf8Value data( args[0]->ToString() );
	int result;
	//lprintf( "add data..." );
	for( result = json_parse_add_data( parser->state, *data, data.length() );
		result > 0;
		//lprintf( "flush more..." ), 
		result = json_parse_add_data( parser->state, NULL, 0 )
		) {
		struct json_value_container * val;
		PDATALIST elements = json_parse_get_data( parser->state );
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
			json_dispose_message( &elements );
			return;
		}

		Local<Value> argv[1];
		if( !o.IsEmpty() )
			argv[0] = o;
		else argv[0] = v;

		Local<Function> cb = Local<Function>::New( isolate, parser->readCallback );
		//lprintf( "callback ... %p", myself );
		// using obj->jsThis  fails. here...
		{
			MaybeLocal<Value> result = cb->Call( isolate->GetCurrentContext()->Global(), 1, argv );
			if( result.IsEmpty() ) // if an exception occurred stop, and return it.
				return;
		}
		json_dispose_message( &elements );
		if( result < 2 )
			break;
	}
	if( result < 0 ) {
		PTEXT error = json_parse_get_error( parser->state );
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, GetText( error ) ) ) );
		LineRelease( error );
		json_parse_clear_state( parser->state );
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



void parseObject::write6(const v8::FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	parseObject *parser = ObjectWrap::Unwrap<parseObject>( args.Holder() );
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Missing data parameter." ) ) );
		return;
	}

	String::Utf8Value data( args[0]->ToString() );
	int result;
	//lprintf( "add data..." );
	for( result = json6_parse_add_data( parser->state, *data, data.length() );
		result > 0;
		//lprintf( "flush more..." ), 
		result = json6_parse_add_data( parser->state, NULL, 0 )
		) {
		struct json_value_container * val;
		PDATALIST elements = json_parse_get_data( parser->state );
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
			json_dispose_message( &elements );
			return;
		}

		Local<Value> argv[1];
		if( !o.IsEmpty() )
			argv[0] = o;
		else argv[0] = v;

		Local<Function> cb = Local<Function>::New( isolate, parser->readCallback );
		//lprintf( "callback ... %p", myself );
		// using obj->jsThis  fails. here...
		{
			MaybeLocal<Value> result = cb->Call( isolate->GetCurrentContext()->Global(), 1, argv );
			if( result.IsEmpty() ) // if an exception occurred stop, and return it.
				return;
		}
		json_dispose_message( &elements );
		if( result < 2 )
			break;
	}
	if( result < 0 ) {
		PTEXT error = json_parse_get_error( parser->state );
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, GetText( error ) ) ) );
		LineRelease( error );
		json_parse_clear_state( parser->state );
		return;
	}

}

void parseObject::New6( const v8::FunctionCallbackInfo<Value>& args ) {
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
	case VALUE_EMPTY:
		return Undefined(isolate);
	case VALUE_STRING:
		return String::NewFromUtf8( isolate, val->string );
	case VALUE_NUMBER:
		if( val->float_result )
			return Number::New( isolate, val->result_d );
		else {
			if( val->result_n < 0x7FFFFFFF && val->result_n > -0x7FFFFFFF )
				return Integer::New( isolate, (int32_t)val->result_n );
			else
				return Number::New( isolate, (double)val->result_n );
		}
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
		//lprintf( "value name is : %d %s", val->value_type, val->name ? val->name : "(NULL)" );
		switch( val->value_type ) {
		default:
			if( val->name )
				o->Set( String::NewFromUtf8( isolate, val->name )
					, makeValue( isolate, val ) );
			else {
				o->Set( index++, makeValue( isolate, val ) );
				if( val->value_type == VALUE_EMPTY )
					o->Delete( isolate->GetCurrentContext(), index - 1 );
			}
			break;
		case VALUE_ARRAY:
			if( val->name )
				o->Set( String::NewFromUtf8( isolate, val->name )
					, sub_o = Array::New( isolate ) );
			else {
				o->Set( index++, sub_o = Array::New( isolate ) );
			}
			buildObject( val->contains, sub_o, isolate );
			break;
		case VALUE_OBJECT:
			if( val->name ) {
				//JSObject::DefinePropertyOrElementIgnoreAttributes(json_object, key, value)
				//    .Check();
				/*
				lprintf( "namelen:%d", val->nameLen );
				o->DefineOwnProperty( isolate->GetCurrentContext()
					, (Local<Name>)String::NewFromUtf8( isolate, val->name, NewStringType::kNormal, -1 ).ToLocalChecked()
					, sub_o = Object::New( isolate )
					, PropertyAttribute::None );
				*/
				o->Set( //String::NewFromUtf8( isolate, val->name, NewStringType::kNormal, -1 ).ToLocalChecked()
						String::NewFromUtf8( isolate,val->name )
							, sub_o = Object::New( isolate ) );
			} 
			else {
				o->Set( index++, sub_o = Object::New( isolate ) );
			}

			buildObject( val->contains, sub_o, isolate );
			break;
		}
	}
}


Local<Value> ParseJSON(  Isolate *isolate, const char *utf8String, size_t len) {
	PDATALIST parsed = NULL;
	Local<Object> o;// = Object::New( isolate );
	Local<Value> v;// = Object::New( isolate );
	if( !json_parse_message( (char*)utf8String, len, &parsed ) )
	{
		//lprintf( "Failed to parse data..." );
		PTEXT error = json_parse_get_error( NULL );
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, GetText( error ) ) ) );
		LineRelease( error );
		return Undefined( isolate );
	}
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
	if( !json6_parse_message( (char*)utf8String, len, &parsed ) ) {
		//PTEXT error = json_parse_get_error( parser->state );
		//lprintf( "Failed to parse data..." );
		PTEXT error = json_parse_get_error( NULL );
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, GetText( error ) ) ) );
		LineRelease( error );
		return Undefined(isolate);
	}
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
