
#include "global.h"

struct currentState {
	Persistent<Function> done;
	Persistent<Function> unhandled;
	Local<Value> state;
};

struct CurrentRule {
	Persistent<Function> handler;
	class ConfigObject *config;
	CurrentRule(){}
};

class ConfigObject : public node::ObjectWrap {
public:
	Isolate *isolate;
	//static v8::Persistent<v8::Function> constructor;
	PCONFIG_HANDLER pch;
	PLIST handlers = NULL;
	uintptr_t lastResult;
	Persistent<Value> lastValue;
	Persistent<Function> unhandled;
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

//Persistent<Function> ConfigObject::constructor;

static uintptr_t CPROC releaseArg( uintptr_t lastArg ) {
	return 0;
}

ConfigObject::ConfigObject() {
	pch = CreateConfigurationHandler();
	lastResult = (uintptr_t)this;
	//unhandled.Reset();
	SetConfigurationEndProc( pch, releaseArg );
}

ConfigObject::~ConfigObject() {
	lastValue.Reset();
	//Local<Object> deleteMe = ( (Value*)lastResult )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked();
	DestroyConfigurationEvaluator( pch );
}

static uintptr_t HandleUnhandled( uintptr_t psv, CTEXTSTR line ) {
	ConfigObject* config = (ConfigObject*)psv;
	if (!config->unhandled.IsEmpty()) {
		Local<Function> handler = config->unhandled.Get( config->isolate );
		if (line) {
			Local<Value> argv[1] = { String::NewFromUtf8( config->isolate, line, v8::NewStringType::kNormal ).ToLocalChecked() };
			handler->Call( config->isolate->GetCurrentContext(), config->lastValue.Get( config->isolate ), 1, argv );
		}else
			handler->Call( config->isolate->GetCurrentContext(), config->lastValue.Get( config->isolate ), 0, NULL );
	}
	return 0;
}

void ConfigObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		ConfigObject* obj;
		obj = new ConfigObject();
		obj->isolate = isolate;
		obj->lastValue.Reset( isolate, args.This() );
		obj->Wrap( args.This() );
		SetConfigurationUnhandled( obj->pch, HandleUnhandled );
		args.GetReturnValue().Set( args.This() );
	} else {
		//const int argc = 1;
		//Local<Value> argv[1];
		//if( args.Length() > 0 )
		//	argv[0] = args[0];
		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->ConfigObject_constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );
	}
}

static uintptr_t CPROC handler( uintptr_t psv, uintptr_t psvRule, arg_list args ) {
	struct CurrentRule *rule = (struct CurrentRule *)psvRule;
	ConfigObject *config = rule->config;
	int argc = PARAM_COUNT(args);
	Local<Value> *argv = new Local<Value>[argc];
	int lessn = 0;
	int n;
	for( n = 0; n < argc; n++ ) {
		switch( my_va_next_arg_type( args ) ) {
		case CONFIG_ARG_STRING:
			{
				char *arg;
				arg = my_va_arg( args, char * );
				argv[n - lessn] = String::NewFromUtf8( config->isolate, arg, v8::NewStringType::kNormal ).ToLocalChecked();
			}
			break;
		case CONFIG_ARG_INT64:
			{
				uint64_t arg;
				arg = my_va_arg( args, uint64_t );
				argv[n - lessn] = Integer::New( config->isolate, (int32_t)arg );
			}
			break;
		case CONFIG_ARG_FLOAT:
			{
				double arg;
				arg = my_va_arg( args, double );
				argv[n - lessn] = Number::New( config->isolate, arg );
			}
			break;
		case CONFIG_ARG_DATA:
			{
				void * arg = my_va_arg( args, void* );
				lessn++;
				break;
		case CONFIG_ARG_DATA_SIZE:
				{
					size_t arglen;
					arglen = my_va_arg( args, size_t );
					Local<ArrayBuffer> ab;
#if ( NODE_MAJOR_VERSION >= 14 )
					std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( (POINTER)arg, arglen, releaseBufferBackingStore, NULL );
					ab = ArrayBuffer::New( config->isolate, bs );
#else
					ab = ArrayBuffer::New( isolate, (void*)arg, arglen );
#endif
					argv[n- lessn] = ab;
				}
			}
			break;
		case CONFIG_ARG_LOGICAL:
			{
				LOGICAL arg;
				arg = my_va_arg( args, LOGICAL );
				if( arg )
					argv[n - lessn] = True(config->isolate);
				else
					argv[n - lessn] = False( config->isolate);
			}
			break;
		case CONFIG_ARG_FRACTION:
			{
				PFRACTION arg = my_va_arg( args, PFRACTION );
				lprintf( "need conversion to JS type for Fraction: %d/%d", arg->numerator, arg->denominator );
				lessn--;
			}
			break;
		case CONFIG_ARG_COLOR:
			{
				CDATA arg = my_va_arg( args, CDATA );
				lprintf( "need conversion to JS type for Color: %08x", arg );
				lessn--;
			}
			break;
		}
	}
	Local<Function> cb = rule->handler.Get( config->isolate );
	Local<Value> that = config->lastValue.Get( config->isolate );
	//Local<Value> ((Value*)psv)->ToObject( config->isolate->GetCurrentContext() ).ToLocalChecked()
	MaybeLocal<Value> ml_result = cb->Call( config->isolate->GetCurrentContext(), that, n-lessn, argv );
	if (!ml_result.IsEmpty()) {
		Local<Value> result = ml_result.ToLocalChecked();
		config->lastValue.Reset( config->isolate, result );
		//config->lastResult = (uintptr_t)*result;
		//result.Clear();
	}
	return (uintptr_t)config;
}

void ConfigObject::Add( const v8::FunctionCallbackInfo<Value>& args ) {
	ConfigObject *config = ObjectWrap::Unwrap<ConfigObject>( args.This() );
	struct CurrentRule *rule = new CurrentRule();
	rule->config = config;
	rule->handler.Reset( args.GetIsolate(), Local<Function>::Cast( args[1] ) );
	AddLink( &config->handlers, rule );
	String::Utf8Value format( USE_ISOLATE( args.GetIsolate() ) args[0] );
	AddConfigurationExx( config->pch, *format, handler, (uintptr_t)rule DBG_SRC );
}

void ConfigObject::On( const v8::FunctionCallbackInfo<Value>& args ) {
	if( args.Length() > 1 ) {
		ConfigObject* config = ObjectWrap::Unwrap<ConfigObject>( args.This() );
		String::Utf8Value event( USE_ISOLATE( args.GetIsolate() ) args[0] );
		if( StrCmp( *event, "done" ) == 0 ) {

		}
		if( StrCmp( *event, "unhandled" ) == 0 ) {
			config->unhandled.Reset( config->isolate, Local<Function>::Cast( args[1] )) ;
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
	ProcessConfigurationFile( config->pch, *filename, (uintptr_t)(config) );
}

void ConfigObject::Write( const v8::FunctionCallbackInfo<Value>& args ) {
	ConfigObject *config = ObjectWrap::Unwrap<ConfigObject>( args.This() );
	String::Utf8Value input( USE_ISOLATE( args.GetIsolate() ) args[0] );
	config->lastResult = ProcessConfigurationInput( config->pch, *input, input.length(), config->lastResult );
}


void ConfigScriptInit( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> configTemplate;

	configTemplate = FunctionTemplate::New( isolate, ConfigObject::New );
	configTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.Config" ) );
	configTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap

	NODE_SET_PROTOTYPE_METHOD( configTemplate, "add", ConfigObject::Add );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "go", ConfigObject::Go );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "write", ConfigObject::Write );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "addFilter", ConfigObject::addFilter );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "clearFilters", ConfigObject::clearDefaultFilters );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "on", ConfigObject::On );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "begin", ConfigObject::Begin );
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "end", ConfigObject::End );

	Local<Function> configfunc = configTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
	class constructorSet* c = getConstructors( isolate );
	c->ConfigObject_constructor.Reset( isolate, configfunc );


	SET_READONLY_METHOD(configfunc, "expand", configExpand );
	SET_READONLY_METHOD(configfunc, "strip", configStrip );
	SET_READONLY_METHOD(configfunc, "color", configColor );
	SET_READONLY_METHOD(configfunc, "encode", configEncode );
	SET_READONLY_METHOD(configfunc, "decode", configDecode );
	SET( exports, "Config", configfunc );

}

