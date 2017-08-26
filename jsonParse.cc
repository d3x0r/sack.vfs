
#include "global.h"
#include <math.h>

static void buildObject( PDATALIST msg_data, Local<Object> o, Isolate *isolate, struct reviver_data *revive );
static Local<Value> makeValue( Isolate *isolate, struct json_value_container *val, struct reviver_data *revive );


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
	static Persistent<Function> constructor6;
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
Persistent<Function> parseObject::constructor6;

void InitJSON( Isolate *isolate, Handle<Object> exports ){
	Local<Object> o = Object::New( isolate );
	SET_READONLY_METHOD( o, "parse", parseJSON );
	NODE_SET_METHOD( o, "stringify", makeJSON );
	SET_READONLY( exports, "JSON", o );

	{
		Local<FunctionTemplate> parseTemplate;
		parseTemplate = FunctionTemplate::New( isolate, parseObject::New );
		parseTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.json6_parser" ) );
		parseTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "write", parseObject::write );

		parseObject::constructor.Reset( isolate, parseTemplate->GetFunction() );

		SET_READONLY( o, "begin", parseTemplate->GetFunction() );
	}

	Local<Object> o2 = Object::New( isolate );
	SET_READONLY_METHOD( o2, "parse", parseJSON6 );
	NODE_SET_METHOD( o2, "stringify", makeJSON6 );
	SET_READONLY( exports, "JSON6", o2 );

	{
		Local<FunctionTemplate> parseTemplate;
		parseTemplate = FunctionTemplate::New( isolate, parseObject::New6 );
		parseTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.json6_parser" ) );
		parseTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "write", parseObject::write6 );

		parseObject::constructor6.Reset( isolate, parseTemplate->GetFunction() );

		SET_READONLY( o2, "begin", parseTemplate->GetFunction() );
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
	for( result = json6_parse_add_data( parser->state, *data, data.length() );
		result > 0;
		//lprintf( "flush more..." ), 
		result = json6_parse_add_data( parser->state, NULL, 0 )
		) {
		struct json_value_container * val;
		PDATALIST elements = json_parse_get_data( parser->state );
		Local<Object> o;
		Local<Value> v;// = Object::New( isolate );

		Local<Value> argv[1];
		val = (struct json_value_container *)GetDataItem( &elements, 0 );
		if( val ) {
			struct reviver_data r;
			r.revive = FALSE;
			argv[0] = convertMessageToJS( isolate, elements, &r );
			Local<Function> cb = Local<Function>::New( isolate, parser->readCallback );
			{
				MaybeLocal<Value> result = cb->Call( isolate->GetCurrentContext()->Global(), 1, argv );
				if( result.IsEmpty() ) // if an exception occurred stop, and return it.
					return;
			}
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
		val = (struct json_value_container *)GetDataItem( &elements, 0 );
		if( val ) {
			Local<Value> argv[1];
			struct reviver_data r;
			r.revive = FALSE;
			argv[0] = convertMessageToJS( isolate, elements, &r );
			Local<Function> cb = Local<Function>::New( isolate, parser->readCallback );
			{
				MaybeLocal<Value> result = cb->Call( isolate->GetCurrentContext()->Global(), 1, argv );
				if( result.IsEmpty() ) // if an exception occurred stop, and return it.
					return;
			}
			json_dispose_message( &elements );
		}
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

static Local<Value> makeValue( Isolate *isolate, struct json_value_container *val, struct reviver_data *revive ) {

	Local<Value> result;
	switch( val->value_type ) {
	case VALUE_UNDEFINED:
		result = Undefined( isolate );
		break;
	default:
		lprintf( "Parser faulted; should never have a uninitilized value." );
		result = Undefined( isolate );
		break;
	case VALUE_NULL:
		result = Null( isolate );
		break;
	case VALUE_TRUE:
		result = True( isolate );
		break;
	case VALUE_FALSE:
		result = False( isolate );
		break;
	case VALUE_EMPTY:
		result = Undefined(isolate);
		break;
	case VALUE_STRING:
		result = String::NewFromUtf8( isolate, val->string );
		break;
	case VALUE_NUMBER:
		if( val->float_result )
			result = Number::New( isolate, val->result_d );
		else {
			if( val->result_n < 0x7FFFFFFF && val->result_n > -0x7FFFFFFF )
				result = Integer::New( isolate, (int32_t)val->result_n );
			else
				result = Number::New( isolate, (double)val->result_n );
		}
		break;
	case VALUE_ARRAY:
		result = Array::New( isolate );
		break;
	case VALUE_OBJECT:
		result = Object::New( isolate );
		break;
	case VALUE_NEG_NAN:
		result = Number::New(isolate, -NAN);
		break;
	case VALUE_NAN:
		result = Number::New(isolate, NAN);
		break;
	case VALUE_NEG_INFINITY:
		result = Number::New(isolate, -INFINITY);
		break;
	case VALUE_INFINITY:
		result = Number::New(isolate, INFINITY);
		break;
	}
	if( revive->revive ) {
		Local<Value> args[2] = { revive->value, result };
		Local<Value> r = revive->reviver->Call( revive->_this, 2, args );
	}
	return result;
}

static void buildObject( PDATALIST msg_data, Local<Object> o, Isolate *isolate, struct reviver_data *revive ) {
	Local<Value> saveVal;
	Local<Value> thisKey;
	LOGICAL saveRevive = revive->revive;
	if( revive ) saveVal = (revive->value);
	struct json_value_container *val;
	Local<Object> sub_o;
	INDEX idx;
	int index = 0;
	DATA_FORALL( msg_data, idx, struct json_value_container*, val )
	{
		//lprintf( "value name is : %d %s", val->value_type, val->name ? val->name : "(NULL)" );
		switch( val->value_type ) {
		default:
			if( val->name ) {
				o->Set( revive->value = String::NewFromUtf8( isolate, val->name )
					, makeValue( isolate, val, revive ) );
			} else {
				if( val->value_type == VALUE_EMPTY )
					revive->revive = FALSE;
				if( revive->revive )
					revive->value = Integer::New( isolate, index );
				o->Set( index++, makeValue( isolate, val, revive ) );
				revive->revive = saveRevive;
				if( val->value_type == VALUE_EMPTY )
					o->Delete( isolate->GetCurrentContext(), index - 1 );
			}
			break;
		case VALUE_ARRAY:
			if( val->name ) {
				o->Set( thisKey = String::NewFromUtf8( isolate, val->name )
					, sub_o = Array::New( isolate ) );
			} 
			else {
				if( revive->revive )
					thisKey = Integer::New( isolate, index );
				o->Set( index++, sub_o = Array::New( isolate ) );
			}
			buildObject( val->contains, sub_o, isolate, revive );
			if( revive->revive ) {
				Local<Value> args[2] = { thisKey, sub_o };
				revive->reviver->Call( revive->_this, 2, args );
			}
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
						thisKey = String::NewFromUtf8( isolate,val->name )
							, sub_o = Object::New( isolate ) );
			} 
			else {
				if( revive->revive )
					thisKey = Integer::New( isolate, index );
				o->Set( index++, sub_o = Object::New( isolate ) );
			}

			buildObject( val->contains, sub_o, isolate, revive );
			if( revive->revive ) {
				Local<Value> args[2] = { thisKey, sub_o };
				revive->reviver->Call( revive->_this, 2, args );
			}
			break;
		}
	}
}

Local<Value> convertMessageToJS( Isolate *isolate, PDATALIST msg, struct reviver_data *revive ) {
	Local<Object> o;
	Local<Value> v;// = Object::New( isolate );

	struct json_value_container *val = (struct json_value_container *)GetDataItem( &msg, 0 );
	if( val && val->contains ) {
		if( val->value_type == VALUE_OBJECT )
			o = Object::New( isolate );
		else if( val->value_type == VALUE_ARRAY )
			o = Array::New( isolate );
		else
			lprintf( "Value has contents, but is not a container type?!" );
		buildObject( val->contains, o, isolate, revive );
		return o;
	} 
	if( val )
		return makeValue( isolate, val, revive );
}


Local<Value> ParseJSON(  Isolate *isolate, const char *utf8String, size_t len, struct reviver_data *revive ) {
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

	Local<Value> value = convertMessageToJS( isolate, parsed, revive );
	json_dispose_message( &parsed );
	return value;
}

void parseJSON( const v8::FunctionCallbackInfo<Value>& args )
{
	Isolate* isolate = Isolate::GetCurrent();
	const char *msg;
	String::Utf8Value tmp( args[0] );
	msg = *tmp;
	Handle<Function> reviver;
	LOGICAL revive = FALSE;
	msg = *tmp;
	struct reviver_data r;

	if( args.Length() > 1 ) {
		if( args[1]->IsFunction() ) {
			r._this = args.Holder();
			r.value = String::NewFromUtf8( isolate, "" );
			r.reviver = Handle<Function>::Cast( args[1] );
			r.revive = TRUE;
		}
		else {
			isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( isolate, TranslateText( "Reviver parameter is not a function." ) ) ) );

			return;
		}
	}
	else
		r.revive = FALSE;

	args.GetReturnValue().Set( ParseJSON( isolate, msg, strlen( msg ), &r ) );
}


void makeJSON( const v8::FunctionCallbackInfo<Value>& args ) {
	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), "undefined :) Stringify is not completed" ) );
}

Local<Value> ParseJSON6(  Isolate *isolate, const char *utf8String, size_t len, struct reviver_data *revive ) {
	PDATALIST parsed = NULL;
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
	Local<Value> value = convertMessageToJS( isolate, parsed, revive );
	json_dispose_message( &parsed );
	return value;
}

void parseJSON6( const v8::FunctionCallbackInfo<Value>& args )
{
	Isolate* isolate = Isolate::GetCurrent();
	if( args.Length() == 0 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, TranslateText( "Missing parameter, data to parse" ) ) ) );

		return;
	}
	const char *msg;
	String::Utf8Value tmp( args[0] );
	Handle<Function> reviver;
	msg = *tmp;
	struct reviver_data r;
	if( args.Length() > 1 ) {
		if( args[1]->IsFunction() ) {
			r._this = args.Holder();
			r.value = String::NewFromUtf8( isolate, "" );
			r.revive = TRUE;
			r.reviver = Handle<Function>::Cast( args[1] );
		}
		else {
			isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( isolate, TranslateText( "Reviver parameter is not a function." ) ) ) );
			return;
		}
	}
	else 
		r.revive = FALSE;

	args.GetReturnValue().Set( ParseJSON6( isolate, msg, strlen( msg ), &r ) );
}


void makeJSON6( const v8::FunctionCallbackInfo<Value>& args ) {
	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), "undefined :) Stringify is not completed" ) );
}
