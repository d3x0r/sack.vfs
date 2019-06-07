
#include "global.h"

struct currentState {
	Persistent<Function> done;
	Persistent<Function> unhandled;
	Local<Value> state;
};

struct CurrentRule {
	Persistent<Function> handler;
	class ConfigObject *config;
	CurrentRule() : handler() {}
};

class ConfigObject : public node::ObjectWrap {
public:
	Isolate *isolate;
	static v8::Persistent<v8::Function> constructor;
	PCONFIG_HANDLER pch;
	PLIST handlers;
	uintptr_t lastResult;
	//PCONFIG_HANDLER pchCurrent;
	ConfigObject();
	~ConfigObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void Add( const v8::FunctionCallbackInfo<Value>& args );
	static void Go( const v8::FunctionCallbackInfo<Value>& args );
	static void Write( const v8::FunctionCallbackInfo<Value>& args );
	static void On( const v8::FunctionCallbackInfo<Value>& args );
	static void Begin( const v8::FunctionCallbackInfo<Value>& args );
	static void End( const v8::FunctionCallbackInfo<Value>& args );
	static void addFilter( const v8::FunctionCallbackInfo<Value>& args );
	static void clearDefaultFilters( const v8::FunctionCallbackInfo<Value>& args ) {
		ConfigObject *config = ObjectWrap::Unwrap<ConfigObject>( args.This() );
		ClearDefaultFilters( config->pch );
	}

};

Persistent<Function> ConfigObject::constructor;

static uintptr_t CPROC releaseArg( uintptr_t lastArg ) {
	return 0;
}

ConfigObject::ConfigObject() {
	pch = CreateConfigurationHandler();
	SetConfigurationEndProc( pch, releaseArg );
}

ConfigObject::~ConfigObject() {
	Local<Object> deleteMe = ( (Value*)lastResult )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
	DestroyConfigurationEvaluator( pch );
}

void ConfigObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		ConfigObject* obj;
		obj = new ConfigObject();
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	} else {
		//const int argc = 1;
		//Local<Value> argv[1];
		//if( args.Length() > 0 )
		//	argv[0] = args[0];
		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );
	}
}

static uintptr_t CPROC handler( uintptr_t psv, uintptr_t psvRule, arg_list args ) {
	struct CurrentRule *rule = (struct CurrentRule *)psvRule;
	ConfigObject *config = rule->config;
	int argc = PARAM_COUNT(args);
	Local<Value> *argv = new Local<Value>[argc];
	int n;
	for( n = 0; n < argc; n++ ) {
		switch( my_va_next_arg_type( args ) ) {
		case CONFIG_ARG_STRING:
			{
				char *arg;
				arg = my_va_arg( args, char * );
				argv[n] = String::NewFromUtf8( config->isolate, arg, v8::NewStringType::kNormal ).ToLocalChecked();
			}
			break;
	case CONFIG_ARG_INT64:
			{
				uint64_t arg;
				arg = my_va_arg( args, uint64_t );
				argv[n] = Integer::New( config->isolate, (int32_t)arg );
			}
			break;
	case CONFIG_ARG_FLOAT:
			{
				double arg;
				arg = my_va_arg( args, double );
				argv[n] = Number::New( config->isolate, arg );
			}
			break;
	case CONFIG_ARG_DATA:
			{
				void * arg;
				arg = my_va_arg( args, void* );
				break;
	case CONFIG_ARG_DATA_SIZE:
			{
				size_t arglen;
				arglen = my_va_arg( args, size_t );
				lprintf( "Create new typedarray...." );
			}
			}
			break;
	case CONFIG_ARG_LOGICAL:
			{
				LOGICAL arg;
				arg = my_va_arg( args, LOGICAL );
				if( arg )
					argv[n] = True(config->isolate);
				else
					argv[n] = False( config->isolate);
			}
			break;
	case CONFIG_ARG_FRACTION:
			{
				PFRACTION arg;
				arg = my_va_arg( args, PFRACTION );
			}
			break;
		case CONFIG_ARG_COLOR:
			{
				CDATA arg;
				arg = my_va_arg( args, CDATA );
			}
			break;

		}
	}
	Local<Function> cb = rule->handler.Get( config->isolate );
	Local<Value> result = cb->Call( config->isolate->GetCurrentContext(), ( (Value*)psv )->ToObject( config->isolate->GetCurrentContext() ).ToLocalChecked(), argc, argv ).ToLocalChecked();
	config->lastResult = (uintptr_t)*result;
	result.Clear();
	return config->lastResult;
}

void ConfigObject::Add( const v8::FunctionCallbackInfo<Value>& args ) {
	ConfigObject *config = ObjectWrap::Unwrap<ConfigObject>( args.This() );
	struct CurrentRule *rule = new CurrentRule();
	rule->handler.Reset( args.GetIsolate(), Local<Function>::Cast( args[1] ) );
	AddLink( &config->handlers, rule );
	String::Utf8Value format( USE_ISOLATE( args.GetIsolate() ) args[0] );
	AddConfigurationExx( config->pch, *format, handler, (uintptr_t)rule DBG_SRC );
}

void ConfigObject::On( const v8::FunctionCallbackInfo<Value>& args ) {
	if( args.Length() > 1 ) {
		String::Utf8Value event( USE_ISOLATE( args.GetIsolate() ) args[0] );
		if( StrCmp( *event, "done" ) == 0 ) {

		}
		if( StrCmp( *event, "unhandled" ) == 0 ) {

		}
	}
}

void ConfigObject::Begin( const v8::FunctionCallbackInfo<Value>& args ) {

}

void ConfigObject::End( const v8::FunctionCallbackInfo<Value>& args ) {

}

static PTEXT CPROC dispatchFilter( POINTER *tmp, PTEXT data ) {
	return NULL;
}

void ConfigObject::addFilter( const v8::FunctionCallbackInfo<Value>& args ) {
	ConfigObject *config = ObjectWrap::Unwrap<ConfigObject>( args.This() );
	AddConfigurationFilter( config->pch, dispatchFilter );
}

static void configExpand( const v8::FunctionCallbackInfo<Value>& args ) {
	if( args.Length() > 0 ) {

	}
}
static void configStrip( const v8::FunctionCallbackInfo<Value>& args ) {
	if( args.Length() > 0 ) {

	}
}
static void configEncode( const v8::FunctionCallbackInfo<Value>& args ) {
	if( args.Length() > 0 ) {

	}
}
static void configDecode( const v8::FunctionCallbackInfo<Value>& args ) {
	if( args.Length() > 0 ) {

	}
}
static void configColor( const v8::FunctionCallbackInfo<Value>& args ) {
	if( args.Length() > 0 ) {

	}
}

void ConfigObject::Go( const v8::FunctionCallbackInfo<Value>& args ) {
	ConfigObject *config = ObjectWrap::Unwrap<ConfigObject>( args.This() );
	String::Utf8Value filename( USE_ISOLATE( args.GetIsolate() ) args[0] );
	ProcessConfigurationFile( config->pch, *filename, (uintptr_t)(*Null(args.GetIsolate() )) );
}

void ConfigObject::Write( const v8::FunctionCallbackInfo<Value>& args ) {
	ConfigObject *config = ObjectWrap::Unwrap<ConfigObject>( args.This() );
	String::Utf8Value input( USE_ISOLATE( args.GetIsolate() ) args[0] );
	config->lastResult = ProcessConfigurationInput( config->pch, *input, input.length(), config->lastResult );
}


void ConfigScriptInit( Local<Object> exports ) {
	Isolate* isolate = Isolate::GetCurrent();
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> configTemplate;

	configTemplate = FunctionTemplate::New( isolate, ConfigObject::New );
	configTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.Config", v8::NewStringType::kNormal ).ToLocalChecked() );
	configTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap

	NODE_SET_PROTOTYPE_METHOD( configTemplate, "add", ConfigObject::Add );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "go", ConfigObject::Go );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "write", ConfigObject::Write );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "addFilter", ConfigObject::addFilter );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "clearFilters", ConfigObject::clearDefaultFilters );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "on", ConfigObject::On );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "begin", ConfigObject::Begin );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "end", ConfigObject::End );

	Local<Object> configfunc = configTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();

	SET_READONLY_METHOD(configfunc, "expand", configExpand );
	SET_READONLY_METHOD(configfunc, "strip", configStrip );
	SET_READONLY_METHOD(configfunc, "color", configColor );
	SET_READONLY_METHOD(configfunc, "encode", configEncode );
	SET_READONLY_METHOD(configfunc, "decode", configDecode );
	SET( exports, "Config", configfunc );

}

