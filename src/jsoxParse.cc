
#include "global.h"
#include <math.h>

//#define DEBUG_INPUT
//#define DEBUG_REVIVAL_CALLBACKS
//#define DEBUG_REFERENCE_FOLLOW
//#define DEBUG_SET_FIELDCB

static void buildObject( PDATALIST msg_data, Local<Object> o, struct reviver_data *revive );
static Local<Value> makeValue( struct jsox_value_container *val, struct reviver_data *revive );

static struct timings {
	uint64_t start;
	uint64_t deltas[10];
}timings;

static void makeJSOX( const v8::FunctionCallbackInfo<Value>& args );
static void escapeJSOX( const v8::FunctionCallbackInfo<Value>& args );
static void parseJSOX( const v8::FunctionCallbackInfo<Value>& args );
static void setFromPrototypeMap( const v8::FunctionCallbackInfo<Value>& args );
static void showTimings( const v8::FunctionCallbackInfo<Value>& args );


class JSOXRefObject : public node::ObjectWrap {
	
public:

	JSOXRefObject() {}
	~JSOXRefObject() {}

	static void New( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		Local<Context> context = isolate->GetCurrentContext();
		int argc = args.Length();
		if( argc == 0 ) {
			isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Must callback to read into." ) ) );
			return;
		}

		if( args.IsConstructCall() ) {
			// Invoked as constructor: `new MyObject(...)`
			JSOXRefObject* obj = new JSOXRefObject();

			if( args.Length() > 1 && !args[1]->IsUndefined() ) {
				SET_READONLY( args.This(), "o", args[0] );
				SET_READONLY( args.This(), "f", args[1] );
			}
			obj->Wrap( args.This() );
			args.GetReturnValue().Set( args.This() );
		}
		else {
			// Invoked as plain function `MyObject(...)`, turn into construct call.
			int argc = args.Length();
			Local<Value>* argv = new Local<Value>[argc];
			for( int n = 0; n < argc; n++ )
				argv[n] = args[n];
			class constructorSet* c = getConstructors( isolate );
			Local<Function> cons = Local<Function>::New( isolate, c->jsoxRefConstructor );
			args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
			delete[] argv;
		}

	}

};
//Persistent<Function> JSOXObject::constructor;

void InitJSOX( Isolate *isolate, Local<Object> exports ){
	Local<Context> context = isolate->GetCurrentContext();

	Local<Object> o2 = Object::New( isolate );
	SET_READONLY_METHOD( o2, "parse", parseJSOX );
	//NODE_SET_METHOD( o2, "stringify", makeJSOX );
	SET_READONLY_METHOD( o2, "setFromPrototypeMap", setFromPrototypeMap );
	SET_READONLY_METHOD( o2, "escape", escapeJSOX );
	//SET_READONLY_METHOD( o2, "timing", showTimings );
	SET_READONLY( exports, "JSOX", o2 );

	{
		Local<FunctionTemplate> parseTemplate;
		parseTemplate = FunctionTemplate::New( isolate, JSOXObject::New );
		parseTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.core.jsox_parser" ) );
		parseTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "write", JSOXObject::write );
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "parse", JSOXObject::parse );
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "reset", JSOXObject::reset );
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "currentRef", JSOXObject::getCurrentRef );
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "setFromPrototypeMap", JSOXObject::setFromPrototypeMap );
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "setPromiseFromPrototypeMap", JSOXObject::setPromiseFromPrototypeMap );

		class constructorSet *c = getConstructors( isolate );
		c->jsoxConstructor.Reset( isolate, parseTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

		if( c->dateCons.IsEmpty() ) {
			Local<Date> tmp = Local<Date>::Cast( Date::New( context, 0 ).ToLocalChecked() );
			Local<Function> cons = Local<Function>::Cast(
				tmp->Get( context, String::NewFromUtf8Literal( isolate, "constructor" ) ).ToLocalChecked()
			);
			c->dateCons.Reset( isolate, cons );
		}

		//SET_READONLY( o2, "begin", parseTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
		SET( o2, "begin", parseTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

		Local<FunctionTemplate> refTemplate;
		refTemplate = FunctionTemplate::New( isolate, JSOXRefObject::New );
		refTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.core.jsox_object_ref" ) );
		refTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap

		SET( o2, "Ref", refTemplate->GetFunction( isolate->GetCurrentContext() ).ToLocalChecked() );

		c->jsoxRefConstructor.Reset( isolate, refTemplate->GetFunction( isolate->GetCurrentContext() ).ToLocalChecked() );
	}

}


JSOXObject::JSOXObject() {
	state = jsox_begin_parse();
	prototypes = NULL;
}

JSOXObject::~JSOXObject() {
	jsox_parse_dispose_state( &state );
	this->reviver.Reset();
	this->readCallback.Reset();
}

#define logTick(n) do { uint64_t tick = GetCPUTick(); if( n >= 0 ) timings.deltas[n] += tick-timings.start; timings.start = tick; } while(0)

void ReportException( v8::Isolate* isolate, v8::TryCatch* handler ) {
}

const char* ToCString( const v8::String::Utf8Value& value ) {
	return *value ? *value : "<string conversion failed>";
}

void JSOXObject::reset( const v8::FunctionCallbackInfo<Value>& args ) {
	JSOXObject* parser = ObjectWrap::Unwrap<JSOXObject>( args.Holder() );
	jsox_parse_clear_state( parser->state );
}

void JSOXObject::getCurrentRef( const v8::FunctionCallbackInfo<Value>& args ) {
	JSOXObject* parser = ObjectWrap::Unwrap<JSOXObject>( args.Holder() );
	Isolate* const isolate = args.GetIsolate();
	// currentReviver;
	//parser->reviver
	//jsox_parse_clear_state( parser->state );
	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->jsoxRefConstructor );
		Local<Value> args_[2] = { parser->currentReviver->refObject, parser->currentReviver->fieldName };
		MaybeLocal<Object> newRef = cons->NewInstance( isolate->GetCurrentContext(), 2, args_ );
		if( !newRef.IsEmpty() )
			args.GetReturnValue().Set( newRef.ToLocalChecked() );
		else
			lprintf("Constructor threw exception");
	}

}

void JSOXObject::write( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	JSOXObject *parser = ObjectWrap::Unwrap<JSOXObject>( args.Holder() );
	int argc = args.Length();

	String::Utf8Value *data_;
	if( argc > 0 ) data_ = new String::Utf8Value( isolate, args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ) ;
	else data_ = NULL;
	int result;
	//Local<Function> cb = Local<Function>::New( isolate, parser->readCallback );
	Local<Context> context = isolate->GetCurrentContext();
	Local<Value> global = context->Global();
#ifdef DEBUG_INPUT
	lprintf( "Parse:%.*s", data_[0].length(), *data_[0] );
#endif
	for( result = jsox_parse_add_data( parser->state, (argc>0)?*data_[0]:NULL, (argc>0)?data_[0].length():0 );
		result > 0;
		result = jsox_parse_add_data( parser->state, NULL, 0 )
		) {
		struct jsox_value_container * val;
		PDATALIST elements = jsox_parse_get_data( parser->state );
		Local<Object> o;
		Local<Value> v;// = Object::New( isolate );

		Local<Value> argv[1];
		val = (struct jsox_value_container *)GetDataItem( &elements, 0 );
		if( val ) {
			struct reviver_data r;
			r.failed = FALSE;
			if( !parser->reviver.IsEmpty() ) {
				r.revive = TRUE;
				r.reviver = parser->reviver.Get( isolate );
				r._this = args.Holder();
			}
			else {
				r.revive = FALSE;
			}
			r.isolate = isolate;
			r.reviveStack = NULL;
			r.context = r.isolate->GetCurrentContext();
			r.parser = parser;
			parser->currentReviver = &r;
			argv[0] = convertMessageToJS2( elements, &r );
			parser->currentReviver = NULL;
			if( r.revive ) {
				Local<Value> args[2] = { String::NewFromUtf8Literal( r.isolate, "" ) , argv[0] };
				MaybeLocal<Value> res = r.reviver->Call( r.context, r._this, 2, args );
				if( !res.IsEmpty() )
					argv[0] = res.ToLocalChecked();

			}
			{
				Local<Function> cb = Local<Function>::New( isolate, parser->readCallback );
				MaybeLocal<Value> cbResult = cb->Call( context, global, 1, argv );
				if( cbResult.IsEmpty() ) {
					lprintf( "Callback failed." );
					delete data_;
					return;
				}
			}
		}
		jsox_dispose_message( &elements );
		if( result < 2 )
			break;
	}
	delete data_;
	if( result < 0 ) {
		PTEXT error = jsox_parse_get_error( parser->state );
		if( error ) {
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, GetText( error ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
			LineRelease( error );
		} else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "No Error Text" STRSYM(__LINE__), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		jsox_parse_clear_state( parser->state );
		return;
	}

}

void JSOXObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Must callback to read into." ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		JSOXObject* obj = new JSOXObject();
		if( args[0]->IsFunction() ) {
			obj->readCallback.Reset( isolate, Local<Function>::Cast( args[0] ) );
		}
		if( args.Length() > 1 && !args[1]->IsUndefined() ) {
			Local<Function> arg1 = Local<Function>::Cast( args[1] );
			obj->reviver.Reset( isolate, arg1 );
		}
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		class constructorSet *c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->jsoxConstructor );
		MaybeLocal<Object> resObj = cons->NewInstance( isolate->GetCurrentContext(), argc, argv );
		if(!resObj.IsEmpty() )
			args.GetReturnValue().Set( resObj.ToLocalChecked() );
		else
			lprintf("Constructor threw exception");
		delete[] argv;
	}

}


Local<Object> getObject( struct reviver_data* revive, struct jsox_value_container* val ) {
	Local<Object> sub_o;
	revive->fieldCb.Clear();
	if( val->className ) {
		Local<Function> cb;

		MaybeLocal<Value> mprotoDef;
		Local<Object> protoDef;
		Local<String> className = String::NewFromUtf8( revive->isolate, val->className, v8::NewStringType::kNormal, (int)val->classNameLen ).ToLocalChecked();
		/*
		if( revive->parser && !revive->parser->promiseFromPrototypeMap.IsEmpty() )
			mprotoDef = revive->parser->promiseFromPrototypeMap.Get( revive->isolate )->Get( revive->context, className );
		if( !mprotoDef->IsEmpty() ) {
			protoDef = mprotoDef->ToLocalChecked();

			struct PromiseWrapper *pw = makePromise( revive->context, revive->isolate );
			Local<Value> args[] = { pw->resolve.Get( revive->isolate ), pw->reject.Get( revive->isolate) };
			sub_o = cb->Call( revive->context, sub_o, 2, args ).ToLocalChecked().As<Object>();

			Local<Value> p = protoDef->Get( revive->context, String::NewFromUtf8( revive->isolate, "protoDef", v8::NewStringType::kNormal, (int)val->classNameLen ).ToLocalChecked() )->ToLocalChecked();
			Local<Value> f = protoDef->Get( revive->context, String::NewFromUtf8( revive->isolate, "cb", v8::NewStringType::kNormal, (int)val->classNameLen ).ToLocalChecked() )->ToLocalChecked();
			cb = f.As<Function>();
			Local<Object> po = p.As<Object>();
			sub_o->SetPrototype( revive->context, po );
		}
		*/
#ifdef DEBUG_REVIVAL_CALLBACKS
		lprintf( "lookup up classname:%.*s", val->classNameLen, val->className );
#endif
		if( mprotoDef.IsEmpty() && revive->parser && !revive->parser->fromPrototypeMap.IsEmpty() ) {
			mprotoDef = revive->parser->fromPrototypeMap.Get( revive->isolate )->Get( revive->context, className );
			if( !mprotoDef.IsEmpty() && !mprotoDef.ToLocalChecked()->IsUndefined() ) {
				if( !mprotoDef.ToLocalChecked()->IsObject() ) {
					String::Utf8Value data( revive->isolate, mprotoDef.ToLocalChecked()->ToString( revive->isolate->GetCurrentContext() ).ToLocalChecked() );
					lprintf( "Expected prototype definition object... failed. %s", *data );
				}
				else {
					protoDef = mprotoDef.ToLocalChecked().As<Object>();
					Local<Value> p = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "protoCon" ) ).ToLocalChecked();
					Local<Value> f = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "cb" ) ).ToLocalChecked();
					if( !f.IsEmpty() && f->IsFunction() ) {
#ifdef DEBUG_SET_FIELDCB
						lprintf( "Setting fieldCB" );
#endif
						revive->fieldCb = f.As<Function>();
					}
					if( p->IsFunction() ) {
#ifdef DEBUG_REVIVAL_CALLBACKS
						lprintf( "New instance in get Object(2)" );
#endif
						MaybeLocal<Object> mo = p.As<Function>()->NewInstance( revive->context, 0, NULL );
						if( !mo.IsEmpty() ) {
							sub_o = mo.ToLocalChecked();
							//lprintf( "Return constructed object...");
						}
						else {
//#ifdef DEBUG_REVIVAL_CALLBACKS
							lprintf( "Constructor threw exception" );
//#endif
							revive->failed = TRUE;
						}
						//else lprintf( "constructor might have failed.");
					}
				}
			}
		}
		if( mprotoDef.IsEmpty() || ( mprotoDef.ToLocalChecked()->IsUndefined() ) ) {
			class constructorSet* c = getConstructors( revive->isolate );
			mprotoDef = c->fromPrototypeMap.Get( revive->isolate )->Get( revive->context, className );
			if( !mprotoDef.IsEmpty() && !mprotoDef.ToLocalChecked()->IsUndefined() ) {
				if( !mprotoDef.ToLocalChecked()->IsObject() ) {
					lprintf( "Expected prototype definition object... failed." );
				}
				else {
					protoDef = mprotoDef.ToLocalChecked().As<Object>();
					Local<Value> p = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "protoCon" ) ).ToLocalChecked();
					Local<Value> f = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "cb") ).ToLocalChecked();
					if( f->IsFunction() ) {
						revive->fieldCb = f.As<Function>();
					}
					if( p->IsFunction() ) {
						//lprintf( "New instance in object..." );
						MaybeLocal<Object> mo = p.As<Function>()->NewInstance( revive->context, 0, NULL );
						if (!mo.IsEmpty())
							sub_o = mo.ToLocalChecked();
						else {
//#ifdef DEBUG_REVIVAL_CALLBACKS
							lprintf( "Making a new instance threw an exception." );
//#endif
							revive->failed = TRUE;
						}
					}
				}
			}
		}

		if( sub_o.IsEmpty() ) {
			INDEX index;
			struct prototypeHolder* holder;
			// named, but there was no constructor, maybe just tagged input
			// and all of those share the same (empty) prototype....
			LIST_FORALL( revive->parser->prototypes, index, struct prototypeHolder*, holder ) {
				if( memcmp( holder->className, val->className, val->classNameLen ) == 0 ) {
					break;
				}
			}
			if( !holder ) {
				holder = NewPlus( struct prototypeHolder, 0 );
				holder->cls = new Persistent<Value>();
				holder->cls[0].Reset( revive->isolate, Object::New( revive->isolate ) );
				holder->className = DupCStrLen( val->className, val->classNameLen );
				AddLink( &revive->parser->prototypes, holder );
			}
			sub_o = Object::New( revive->isolate );
			sub_o->SetPrototype( revive->context, holder->cls[0].Get( revive->isolate ) );
		}
	}
	else {
		sub_o = Object::New( revive->isolate );
	}

	return sub_o;
}

static Local<Value> getArray( struct reviver_data* revive, struct jsox_value_container* val ) {
	Local<Value> sub_o;
#ifdef DEBUG_SET_FIELDCB
	lprintf( "Clear Field CB Here... what about where we came from?" );
#endif
	revive->fieldCb.Clear();
	if( val->className ) {
		MaybeLocal<Value> mprotoDef;
		Local<Object> protoDef;
		//lprintf( "value has a classname: %.*s", val->classNameLen, val->className );
		Local<String> className = String::NewFromUtf8( revive->isolate, val->className, v8::NewStringType::kNormal, (int)val->classNameLen ).ToLocalChecked();

		if( mprotoDef.IsEmpty() && revive->parser && !revive->parser->fromPrototypeMap.IsEmpty() ) {
			mprotoDef = revive->parser->fromPrototypeMap.Get( revive->isolate )->Get( revive->context, className );
			//lprintf( "protodef1?");
			if( !mprotoDef.IsEmpty() && !mprotoDef.ToLocalChecked()->IsUndefined() ) {
				if( !mprotoDef.ToLocalChecked()->IsObject() ) {
					String::Utf8Value data( revive->isolate, mprotoDef.ToLocalChecked()->ToString( revive->isolate->GetCurrentContext() ).ToLocalChecked() );
					lprintf( "Expected prototype definition object... failed. %s", *data );
				}
				else {
					protoDef = mprotoDef.ToLocalChecked().As<Object>();
					Local<Value> p = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "protoCon" ) ).ToLocalChecked();
					Local<Value> f = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "cb" ) ).ToLocalChecked();
					if( f->IsFunction() ) {
#ifdef DEBUG_SET_FIELDCB
						lprintf( "Set protocon callback as field callback" );
#endif
						revive->fieldCb = f.As<Function>();
					}
					else
						revive->fieldCb.Clear();
					if( p->IsFunction() ) {
						//lprintf( "Create a new instance of the thing...null, null");
						MaybeLocal<Object> mo = p.As<Function>()->NewInstance( revive->context, 0, NULL );
						if( !mo.IsEmpty() )
							sub_o = mo.ToLocalChecked();
						else
							lprintf("Constructor threw exception");
					}
				}
			}
		}
		if( mprotoDef.IsEmpty() || ( mprotoDef.ToLocalChecked()->IsUndefined() ) ) {
			class constructorSet* c = getConstructors( revive->isolate );
			mprotoDef = c->fromPrototypeMap.Get( revive->isolate )->Get( revive->context, className );
			if( !mprotoDef.IsEmpty() && !mprotoDef.ToLocalChecked()->IsUndefined() ) {
				if( !mprotoDef.ToLocalChecked()->IsObject() ) {
					lprintf( "Expected prototype definition object... failed." );
				}
				else {
					protoDef = mprotoDef.ToLocalChecked().As<Object>();
					Local<Value> p = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "protoCon" ) ).ToLocalChecked();
					Local<Value> f = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "cb" ) ).ToLocalChecked();
					if( f->IsFunction() ) {
#ifdef DEBUG_SET_FIELDCB
						lprintf( "Set protocon callback as field callback" );
#endif
						revive->fieldCb = f.As<Function>();
					}
					else
						revive->fieldCb.Clear();
					if( p->IsFunction() ) {
						MaybeLocal<Object> mo = p.As<Function>()->NewInstance( revive->context, 0, NULL );
						if( !mo.IsEmpty() )
							sub_o = mo.ToLocalChecked();
						else
							lprintf( "Constructor threw exception" );
					}
				}
			}
		}
	}
	else {
		revive->fieldCb.Clear();
	}
	if( sub_o.IsEmpty() ) {
		sub_o = Array::New( revive->isolate );
	}
	return sub_o;
}


#if ( defined( _DEBUG ) && defined( DEBUG_REFERENCE_FOLLOW ) )
#  define LogObject(o)  LogObjectEx(revive->isolate, o DBG_SRC )
void LogObjectEx( Isolate *isolate, Local<Value> o DBG_PASS ) {
	if( o->IsUndefined() )
		_lprintf( DBG_RELAY )( "Is undefined %p", o );
	else if( o->IsNull() )
		_lprintf( DBG_RELAY )( "Is NULL %p", o );
	else if( o->IsArray() )
		_lprintf( DBG_RELAY )( "Is Array %p", o );
	else if( o->IsObject() )
		_lprintf(DBG_RELAY)( "Is Object %p", o );
	else if( o->IsNumber() )
		_lprintf( DBG_RELAY )( "Is Number(unexpected) %p", o );
	else if( o->IsBoolean() )
		_lprintf( DBG_RELAY )( "Is Bool(unexpected) %p", o );
	else if( o->IsString() ){
		String::Utf8Value tmp( USE_ISOLATE( isolate ) o );
		_lprintf( DBG_RELAY )( "Is String(unexpected) %p %s", o, *tmp );
	}else {
		String::Utf8Value tmp( USE_ISOLATE( isolate ) o );
		_lprintf( DBG_RELAY )( "Unhandeld type.  Value:%s", *tmp );
	}
}
#else 
#  define LogObject(o) 
#endif
#define MODE NewStringType::kNormal
//#define MODE NewStringType::kInternalized

static inline Local<Value> makeValue( struct jsox_value_container *val, struct reviver_data *revive ) {
	Local<Function> fieldCb; // save what the fieldCb was... getArray clears it.
	Local<Value> result;
	Local<Script> script;
	Local<Object> sub_o;
	class constructorSet *c = getConstructors( revive->isolate );
	//lprintf( "Saving the callback... did it get cleared?" );
	//lprintf("handling:%d  %.*s %.*s", val->value_type
	//		, val->nameLen, val->name? val->name:"<NONAME>"
	//		, val->stringLen, val->string?val->string:"<NOSTRING>");
	switch( val->value_type ) {
	case JSOX_VALUE_UNDEFINED:
		result = Undefined( revive->isolate );
		break;
	default:
		if( val->value_type >= JSOX_VALUE_TYPED_ARRAY && val->value_type <= JSOX_VALUE_TYPED_ARRAY_MAX ) {
			Local<ArrayBuffer> ab;
			//lprintf( "Typed array makeValue...%d", val->value_type - JSOX_VALUE_TYPED_ARRAY );
			if( val->value_type < JSOX_VALUE_TYPED_ARRAY_MAX ) {
#if ( NODE_MAJOR_VERSION >= 14 )
				std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( revive->isolate, val->stringLen );
				memcpy( bs->Data(), val->string, val->stringLen );
				ab = ArrayBuffer::New( revive->isolate, bs );
#else
				ab = ArrayBuffer::New( revive->isolate, val->string, val->stringLen, ArrayBufferCreationMode::kExternalized );
#endif
			}
			switch( val->value_type - JSOX_VALUE_TYPED_ARRAY ) {
			case 0:
				result = ab;
				break;
			case 1: // "u8"
				result = Uint8Array::New( ab, 0, val->stringLen );
				break;
			case 2:// "cu8"
				result = Uint8ClampedArray::New( ab, 0, val->stringLen );
				break;
			case 3:// "s8"
				result = Int8Array::New( ab, 0, val->stringLen );
				break;
			case 4:// "u16"
				result = Uint16Array::New( ab, 0, val->stringLen/2 );
				break;
			case 5:// "s16"
				result = Int16Array::New( ab, 0, val->stringLen / 2 );
				break;
			case 6:// "u32"
				result = Uint32Array::New( ab, 0, val->stringLen/4 );
				break;
			case 7:// "s32"
				result = Int32Array::New( ab, 0, val->stringLen / 4 );
				break;
			//case 8:// "u64"
			//	result = Uint64Array::New( ab, 0, val->stringLen/ 8 );
			//	break;
			//case 9:// "s64"
			//	result = Int64Array::New( ab, 0, val->stringLen/ 8 );
			//	break;
			case 10:// "f32"
				result = Float32Array::New( ab, 0, val->stringLen / 4 );
				break;
			case 11:// "f64"
				result = Float64Array::New( ab, 0, val->stringLen/ 8 );
				break;
			case 12:// "ref"
				//lprintf( "THIS should have a container? %p", val->contains );
#ifdef DEBUG_REFERENCE_FOLLOW
				lprintf( "Resolving a ref....%d %d", val->contains->Cnt, revive->reviveStack->Top );
#endif
				{
					struct jsox_value_container *pathVal;
					INDEX idx;
					LOGICAL off_stack = FALSE; // the root object is technically on-stack... 
					Local<Object> refObj = revive->rootObject;
					//lprintf( "Ref Object is the root object...");
					DATA_FORALL( val->contains, idx, struct jsox_value_container *, pathVal ) {
						if( revive->failed ) return Undefined( revive->isolate );
#ifdef DEBUG_REFERENCE_FOLLOW
						lprintf( "get reference:%d %s %d %s %u", idx, off_stack?"OFF":"ON", pathVal->value_type, pathVal->string, (uint32_t)pathVal->result_n );
#endif
						LogObject( refObj );
						
						if( pathVal->value_type == JSOX_VALUE_NUMBER ) {
							MaybeLocal<Value> mbArrayMember = refObj->Get( revive->context, (uint32_t)pathVal->result_n );
							if( !mbArrayMember.IsEmpty() ) {
								Local<Value> arraymember = mbArrayMember.ToLocalChecked();
#ifdef DEBUG_REFERENCE_FOLLOW
								MaybeLocal<String> mbString = arraymember->ToString(revive->context);
								if( !mbString.IsEmpty() ) {
									String::Utf8Value tmp( USE_ISOLATE( revive->isolate ) mbString.ToLocalChecked() );
									//if( StrCmp( *tmp, "[object Object]" ) == 0 )DebugBreak();
									lprintf( "Array member is : %s", *tmp );
								}
#endif
								if( arraymember->IsUndefined() ) {
									//MaybeLocal<Object> maybeRefObj = arraymember->ToObject( revive->isolate->GetCurrentContext() );
									{

										struct reviveStackMember* member = (struct reviveStackMember*)PeekLinkEx( &revive->reviveStack, revive->reviveStack->Top - idx - 1 );
										if( !member ) {
											lprintf( "Stack is %d at %d get %d", revive->reviveStack->Top, idx, revive->reviveStack->Top - idx - 1 );
										}
										if( member && member->index == (uint32_t)pathVal->result_n ) {
											LogObject( member->object );
											if( member->object->IsObject() ) {
												refObj = member->object.As<Object>();
												off_stack = FALSE;
												//lprintf( "Saving replacement, maybe we can re-apply a fixup?" );
												struct reviveMemberReplacement rep;
												rep.object = revive->refObject;
												rep.fieldName = revive->fieldName;
												AddDataItem( &member->pdlSubsts, &rep );
											} else
												lprintf( "Unexpected value from member..." );

										} else {
											lprintf( "Referenced array member is not an object!. " );
										}
									} 
									//else
									//	refObj = maybeRefObj.ToLocalChecked();
								} else {
									LogObject( arraymember );
									off_stack = TRUE;
									refObj = arraymember.As<Object>();
								}
							}
						}
						else if( pathVal->value_type == JSOX_VALUE_STRING ) {
							Local<Value> val_temp;
							if( refObj->IsMap() ) {
								//Local<Map> map = refObj.As<Map>();
								Local<Function> mapGetter = refObj->Get( revive->context, localStringExternal( revive->isolate, "get" ) ).ToLocalChecked().As<Function>();
								Local<Value> args[] = { localStringExternal( revive->isolate, pathVal->string, (int)pathVal->stringLen ) };
								val_temp = mapGetter->Call( revive->context, refObj, 1, args ).ToLocalChecked();
								off_stack = TRUE;
								LogObject( val_temp );
							}
							else {
								Local<String> pathval = String::NewFromUtf8(revive->isolate
									, pathVal->string
									, NewStringType::kNormal
									, (int)pathVal->stringLen).ToLocalChecked();
								//lprintf( "path is:%s", pathVal->string);
								{
									// if it's in the stack, prefer that value which is more current.
									struct reviveStackMember* member = (struct reviveStackMember*)PeekLinkEx( &revive->reviveStack, revive->reviveStack->Top - idx - 1 );
									if( !off_stack && idx < revive->reviveStack->Top ) {
#ifdef DEBUG_REFERENCE_FOLLOW
										lprintf( "Looking at reviveStack...  %d %d  %.*s %p", off_stack
											, member->nameLen
											, member->nameLen
											, member->name, member->name );
#endif
										if( ( member->nameLen == pathVal->stringLen )
											&& ( StrCmpEx( pathVal->string, member->name, pathVal->stringLen ) == 0 )
											) {
											val_temp = member->object;
											//lprintf( "Saving replacement(2), maybe we can re-apply a fixup?" );
											struct reviveMemberReplacement rep;
											off_stack = FALSE;
											rep.object = revive->refObject;
											rep.fieldName = revive->fieldName;
											AddDataItem( &member->pdlSubsts, &rep );
										} else {
											if( refObj->Has( revive->context, pathval ).ToChecked() ) {
												off_stack = TRUE; // probably was offstack too
												val_temp = refObj->Get( revive->context, pathval ).ToLocalChecked();
											} else {

												revive->isolate->ThrowException( Exception::TypeError(
													String::NewFromUtf8( revive->isolate, TranslateText( "bad path specified with reference" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
												//#ifdef DEBUG_REVIVAL_CALLBACKS
												lprintf( "(5)Error at high level parse - throw it to JS..." );
												//#endif
												revive->failed = TRUE;
												return Undefined( revive->isolate );
											}
										}
									}
									if( idx >= revive->reviveStack->Top || (off_stack) /*|| ( member->object != refObj )*/ ) {
										if( refObj->Has( revive->context, pathval ).ToChecked() ) {
#ifdef DEBUG_REFERENCE_FOLLOW
											lprintf ( "object already has path, use that." );
#endif
											val_temp = refObj->Get( revive->context, pathval ).ToLocalChecked();
											off_stack = TRUE;
										} else {
#ifdef DEBUG_REFERENCE_FOLLOW
											lprintf( "This fell off the stack, am assuming it's the current reference object..." );
#endif
											val_temp = revive->refObject;
											off_stack = FALSE;
										}
									} 
								}
							}
							LogObject( val_temp );
							if( val_temp->IsObject() ) {
								refObj = val_temp.As<Object>();
#ifdef DEBUG_REFERENCE_FOLLOW
								{
									MaybeLocal<String> mbString = refObj->ToString( revive->context );
									String::Utf8Value tmp( USE_ISOLATE( revive->isolate ) mbString.ToLocalChecked() );
									const char* tmp2 = *tmp;
									if( tmp2[0] == 0 ) DebugBreak();
									lprintf( "Changed to new object: %s", *tmp );
									if( !mbString.IsEmpty() ) {
										String::Utf8Value tmp( USE_ISOLATE( revive->isolate ) mbString.ToLocalChecked() );
										//if( StrCmp( *tmp, "[object Object]" ) == 0 )DebugBreak();
										lprintf( "Array member is : %s", *tmp );
									}
								}
#endif
							}  else {
								revive->isolate->ThrowException( Exception::TypeError(
									String::NewFromUtf8( revive->isolate, TranslateText( "Expected an object reference but path lookup failed" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
//#ifdef DEBUG_REVIVAL_CALLBACKS
								lprintf( "(6)Error at high level parse - throw it to JS..." );
//#endif
								revive->failed = TRUE;
								return Undefined( revive->isolate );

								//return val_temp;
							}
						}
						//lprintf( "%d %s", pathVal->value_type, pathVal->string );
					}
					result = refObj;
				}
				break;
			default:
				result = Undefined( revive->isolate );
			}
		}
		else {
			lprintf( "Parser faulted; should never have a uninitilized value." );
			result = Undefined( revive->isolate );
		}
		break;
	case JSOX_VALUE_NULL:
		result = Null( revive->isolate );
		break;
	case JSOX_VALUE_TRUE:
		result = True( revive->isolate );
		break;
	case JSOX_VALUE_FALSE:
		result = False( revive->isolate );
		break;
	case JSOX_VALUE_EMPTY:
		result = Undefined(revive->isolate);
		break;
	case JSOX_VALUE_ARRAY:
		fieldCb = revive->fieldCb;
		result = getArray( revive, val );
		if( !result->IsUndefined() ) {

		if(0) {
	case JSOX_VALUE_STRING:
			fieldCb = revive->fieldCb;
			result = String::NewFromUtf8( revive->isolate, val->string, MODE, (int)val->stringLen ).ToLocalChecked();
		}
		if( val->className ) {
			MaybeLocal<Value> valmethod;
			Local<Function> protoCon;
			Local<Function> cb;
			Local<String> className = String::NewFromUtf8( revive->isolate, val->className, v8::NewStringType::kNormal, (int)val->classNameLen ).ToLocalChecked();
#ifdef DEBUG_REVIVAL_CALLBACKS
			lprintf( "Class name is what? %.*s", val->classNameLen, val->className );
#endif
			if( revive->parser && !revive->parser->promiseFromPrototypeMap.IsEmpty() ) {
				valmethod = revive->parser->promiseFromPrototypeMap.Get( revive->isolate )->
					Get( revive->context, className );
					//lprintf( "method1? %d", valmethod.IsEmpty());
				if( !valmethod.IsEmpty() && !valmethod.ToLocalChecked()->IsUndefined() ) {
					Local<Value> args[] = { result };
					cb = valmethod.ToLocalChecked().As<Function>();
					result = cb->Call( revive->context, revive->_this, 1, args ).ToLocalChecked();
					LogObject( result );
					if( result.IsEmpty() ) {
						return Null( revive->isolate );
					}
#ifdef DEBUG_REVIVAL_CALLBACKS
					lprintf( "Result should still get pushed?" );
#endif
				}
				//lprintf( "Maybe it wasn't found? %d", valmethod.IsEmpty() );
			}
			{
				if( revive->parser && ( valmethod.IsEmpty() || (valmethod.ToLocalChecked()->IsUndefined()) ) && !revive->parser->fromPrototypeMap.IsEmpty() ) {
#ifdef DEBUG_REVIVAL_CALLBACKS
					lprintf( "method2?");
#endif
					valmethod = revive->parser->fromPrototypeMap.Get( revive->isolate )->Get( revive->context, className );

					if( !valmethod.IsEmpty() && !valmethod.ToLocalChecked()->IsUndefined() ){
#ifdef DEBUG_REVIVAL_CALLBACKS
						lprintf( "ya... method2." );
#endif
						Local<Object> protoDef;
						protoDef = valmethod.ToLocalChecked().As<Object>();
						//lprintf( "valMethod is an object though...");
						Local<Value> p = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "protoCon" ) ).ToLocalChecked();
						Local<Value> f = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "cb" ) ).ToLocalChecked();
						protoCon = p.As<Function>();
						cb = f.As<Function>();
						//cb = valmethod.ToLocalChecked().As<Function>();
					}
				}
				if( valmethod.IsEmpty() || (valmethod.ToLocalChecked()->IsUndefined()) ) {
					valmethod = c->fromPrototypeMap.Get( revive->isolate )->Get( revive->context, className );

#ifdef DEBUG_REVIVAL_CALLBACKS
					lprintf( "method3? %d %d", valmethod.IsEmpty(), valmethod.ToLocalChecked()->IsUndefined()  );
#endif
					if( !valmethod.IsEmpty() && !valmethod.ToLocalChecked()->IsUndefined() ){
#ifdef DEBUG_REVIVAL_CALLBACKS
						lprintf( "ya... method3..." );
#endif
						Local<Object> protoDef;
						protoDef = valmethod.ToLocalChecked().As<Object>();
						//lprintf( "valMethod is an object though...");
						Local<Value> p = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "protoCon" ) ).ToLocalChecked();
						Local<Value> f = protoDef->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "cb" ) ).ToLocalChecked();
						protoCon = p.As<Function>();
						cb = f.As<Function>();
					}
				}
#ifdef DEBUG_REVIVAL_CALLBACKS
				lprintf( "Check protocon: %d %d", !protoCon.IsEmpty(), !protoCon.IsEmpty()?protoCon->IsFunction():0 );
#endif
				if( !protoCon.IsEmpty() && protoCon->IsFunction() ) {
#ifdef DEBUG_REVIVAL_CALLBACKS
					lprintf( "method4 constructor passresing associatiated string?");
#endif
					if( val->value_type == JSOX_VALUE_STRING ) {
						Local<Value> args[] = { result };
						MaybeLocal<Object> mo = protoCon.As<Function>()->NewInstance( revive->context, 1, args );
						Local<Value> resultTmp;
						if( !mo.IsEmpty() ) {
							resultTmp = mo.ToLocalChecked();
							LogObject( resultTmp );

							if( !cb.IsEmpty() && cb->IsFunction() ) {
#ifdef DEBUG_REVIVAL_CALLBACKS
								lprintf( "method4a?");
#endif
								Local<Value> args[] = { Undefined(revive->isolate), resultTmp };
								MaybeLocal<Value> mv = cb->Call( revive->context, resultTmp, 0, NULL );
								if( !mv.IsEmpty() )
									result = mv.ToLocalChecked();
								else
									result = resultTmp;
							} else {
								lprintf( "created container reference... resulting without reviver" );
								result = resultTmp;
							}
							LogObject( result );

						}
						else
						{
							lprintf( "Threw an exception in constrcutor" );
						}
						// string revival can (and needs to) happen immediately.
						if(0)
							if( !fieldCb.IsEmpty() && fieldCb->IsFunction() ) {
#ifdef DEBUG_REVIVAL_CALLBACKS
								lprintf( "method4a?");
#endif
								Local<Value> args[] = { Undefined(revive->isolate), resultTmp };

								MaybeLocal<Value> mv = fieldCb->Call( revive->context, result, 2, args );
								if( !mv.IsEmpty() )
									result = mv.ToLocalChecked();
								LogObject( result );

							}else {
								lprintf( "Return" );
							}
					}
				}
				else
					if( val->value_type == JSOX_VALUE_STRING )
						// this is done when this returns.
						if( !fieldCb.IsEmpty() && fieldCb->IsFunction() ) {
#ifdef DEBUG_REVIVAL_CALLBACKS
							lprintf( "method4z?");
#endif
							//Local<Value> args[] = { result };
							MaybeLocal<Value> mv = fieldCb->Call( revive->context, result, 0, NULL );
							if( !mv.IsEmpty() )
								result = mv.ToLocalChecked();
							LogObject( result );

						}

			}
		}
		}
		break;
	case JSOX_VALUE_NUMBER:
		if( val->float_result )
			result = Number::New( revive->isolate, val->result_d );
		else
			result = Number::New( revive->isolate, (double)val->result_n );
		break;
	case JSOX_VALUE_OBJECT:
		result = getObject( revive, val );
		break;
	case JSOX_VALUE_NEG_NAN:
		result = Number::New(revive->isolate, -NAN);
		break;
	case JSOX_VALUE_NAN:
		result = Number::New(revive->isolate, NAN);
		break;
	case JSOX_VALUE_NEG_INFINITY:
		result = Number::New(revive->isolate, -INFINITY);
		break;
	case JSOX_VALUE_INFINITY:
		result = Number::New(revive->isolate, INFINITY);
		break;
	case JSOX_VALUE_BIGINT:
		script = Script::Compile( revive->context, String::NewFromUtf8( revive->isolate, val->string, NewStringType::kNormal, (int)val->stringLen ).ToLocalChecked()
#if ( NODE_MAJOR_VERSION >= 16 )
			, new ScriptOrigin( revive->isolate, String::NewFromUtf8Literal( revive->isolate, "BigIntFormatter" ) ) ).ToLocalChecked();
#else
			, new ScriptOrigin( String::NewFromUtf8Literal( revive->isolate, "BigIntFormatter" ) ) ).ToLocalChecked();
#endif			
		result = script->Run( revive->context ).ToLocalChecked();
		//result = BigInt::New( revive->isolate, 0 );
		break;
	case JSOX_VALUE_DATE:
		{
			class constructorSet* c = getConstructors( revive->isolate );
			char* dot = StrChr( val->string, '.' );
			char numdigits[16];
			int digit = 0;
			int nsVal = 0;
			if( dot ) {
				dot++;
				while( digit < 3 && dot[0] >= '0' && dot[0] <= '9' ) {
					dot++;
					digit++;
				}
				digit = 0;
				while( dot[0] >= '0' && dot[0] <= '9' && digit < 6 ) {
					// only have space for millions...
					numdigits[digit++] = (dot++)[0];
				}
				
				if( digit < 6 ) {
					numdigits[digit] = 0;
					nsVal = atoi( numdigits );
					while( digit < 6 ) {
						digit++; nsVal *= 10;
					}
				} else {
					numdigits[6] = 0;
					nsVal = atoi( numdigits );
				}
			}
			// val->string.matches( /\.( \d{ 6 }) / ) ) {
			Local<Value> argv[2] = { String::NewFromUtf8( revive->isolate, val->string, NewStringType::kNormal, (int)val->stringLen ).ToLocalChecked()
					, Number::New( revive->isolate, nsVal ) };
			Local<Object> tmpResult;
			if( nsVal )
				tmpResult = c->dateNsCons.Get( revive->isolate )->NewInstance( revive->context, 2, argv ).ToLocalChecked();
			else 
				tmpResult = c->dateCons.Get( revive->isolate )->NewInstance( revive->context, 1, argv ).ToLocalChecked();
		
#if 0
			MaybeLocal<Value> valid = tmpResult->Get( revive->context, String::NewFromUtf8Literal( revive->isolate, "getTime" ) );
			if( !valid.IsEmpty() ) {
				Local<Function> getTime = valid.ToLocalChecked().As<Function>();
				MaybeLocal<Value> timeVal = getTime->Call( revive->context, tmpResult, 0, NULL );
				Maybe<double> d = timeVal.ToLocalChecked()->NumberValue( revive->context );
				double dval;
				if( isnan(dval = d.FromMaybe( NAN ) ) ) {
					revive->isolate->ThrowException( Exception::TypeError(
						String::NewFromUtf8( revive->isolate, TranslateText( "Bad Number Conversion" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
//#ifdef DEBUG_REVIVAL_CALLBACKS
					lprintf( "(8)Error at high level parse - throw it to JS..." );
//#endif
					revive->failed = TRUE;
				}
			}
			result = tmpResult.As<Value>();
#endif
			result = tmpResult;
		}
		break;
	}
	if( revive->revive && !revive->fieldName.IsEmpty() ) {
		Local<Value> args[2] = { revive->fieldName, result };
		MaybeLocal<Value> r = revive->reviver->Call( revive->context, revive->_this, 2, args );
		if( !r.IsEmpty() )
			result = r.ToLocalChecked();
		LogObject( result );
	}
	return result;
}
static void buildObject( PDATALIST msg_data, Local<Object> o, struct reviver_data *revive ) {
	Isolate* isolate = revive->isolate;
	Local<Context> context = revive->context;
	Local<String> stringKey;
	Local<Value> thisKey;
	struct jsox_value_container *val;
	Local<Value> sub_v;
	INDEX idx;
	int index = 0;
	int currentIndex;
	Local<Value> priorRefField = revive->fieldName;
	Local<Object> priorRefObject = revive->refObject;
	if( revive->failed ) {
		lprintf( "Stopping build, revival already failed.");
		return;
	}
	//if (isolate.has_scheduled_exception()) {

	//}
	revive->refObject = o;
	//lprintf( "... ENTER BUILD OBJECT" );
	Local<Function> cb = revive->fieldCb;

	DATA_FORALL( msg_data, idx, struct jsox_value_container*, val )
	{
		Local<Value> sub_o_orig;
#ifdef DEBUG_REFERENCE_FOLLOW
		lprintf( "value name is : %d %.*s", val->value_type, val->nameLen, val->name ? val->name : "(NULL)" );
#endif
		if( val->name )
			revive->fieldName = String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked();
		else
			revive->fieldName = Number::New( isolate, currentIndex = index++ );
		switch( val->value_type ) {
		case JSOX_VALUE_EMPTY: // only occurs in arrays...
			SETN( o, currentIndex, Undefined(isolate) );
			o->Delete( revive->context, currentIndex );
			break;
		default:
#ifdef DEBUG_REVIVAL_CALLBACKS
			lprintf( "Should be issuing a revive callback with a fieldname %d", val->value_type );
#endif
			if( !cb.IsEmpty() ) {
				Local<Value> args[] = { revive->fieldName, makeValue( val, revive ) };
				if (revive->failed) return;
				// use the custom reviver to assign the field.
#ifdef DEBUG_REVIVAL_CALLBACKS
				lprintf( "Call reviver here with something...%d", val->value_type );
#endif
				if (args[1]->IsUndefined()) {
					lprintf("Reference failed to resolve?");
					return;
				}
				MaybeLocal<Value> newObj = cb->Call( revive->context, o, 2, args );
				if( !newObj.IsEmpty() ) {
					Local<Value> zObj = newObj.ToLocalChecked();
					LogObject( zObj );
					if (zObj->IsObject()) {
						sub_v = zObj;
					}
					if( !zObj->IsUndefined() ) {
						if( val->name )
							SETV( o, revive->fieldName, zObj );
						else
							SETN( o, currentIndex, zObj );
					}
				}
				else {
					lprintf( "Callback threw an error" );
				}

			} else {
				Local<Value> tmp = makeValue( val, revive );
#if defined( DEBUG_REFERENCE_FOLLOW ) || defined( DEBUG_REVIVAL_CALLBACKS )
				if( val->name )
					lprintf( "set value to fieldname: %.*s", val->nameLen, val->name );
				else
					lprintf( "set value to index: %d", currentIndex );
				
#endif
				if( val->value_type == JSOX_VALUE_UNDEFINED || !tmp->IsUndefined() ) {
					o->Set( revive->context, revive->fieldName, tmp );
				}
			}
			break;
		case JSOX_VALUE_ARRAY:
			sub_v = getArray( revive, val );
			if (revive->failed) return;
			if( !cb.IsEmpty() ) {
				Local<Value> args[] = { revive->fieldName
				                      , sub_v };
				// use the custom reviver to assign the field.
#ifdef DEBUG_REVIVAL_CALLBACKS
				lprintf( "Calling revive function.. in Array for something?" );
#endif
				MaybeLocal<Value> newObj = cb->Call( revive->context, o, 2, args );
				if( !newObj.IsEmpty() ) {
					sub_v = newObj.ToLocalChecked();
					LogObject( sub_v );

					if( !sub_v->IsUndefined() )
						o->Set( revive->context, revive->fieldName, newObj.ToLocalChecked() );
					else
						sub_v = o->Get( revive->context, revive->fieldName ).ToLocalChecked();
				}
				else {
					isolate->ThrowException( Exception::Error(
					        localString( isolate, TranslateText( "Already pending exception?" ) ) ) );
//#ifdef DEBUG_REVIVAL_CALLBACKS
					lprintf( "(9)Error at high level parse - throw it to JS..." );
//#endif
					revive->failed = TRUE;
				}

#ifdef DEBUG_REFERENCE_FOLLOW
				lprintf( "called callback to set array value %.*s", val->nameLen, val->name );
#endif
			}
			else {
				o->Set( revive->context, revive->fieldName, sub_v);
			}
#ifdef DEBUG_REVIVAL_CALLBACKS
			lprintf( "to build subobject( array ) isUndefined?...%d", o.IsEmpty()?0:o->IsUndefined() );
#endif
			struct reviveStackMember* member;member = new struct reviveStackMember();
			member->object = sub_v;
			if( val->name ) {
				member->isArray = false;
				member->fieldName = revive->fieldName;
				member->name = val->name;
				member->nameLen = val->nameLen;
			} else {
				member->isArray = true;
				member->index = currentIndex;
			}
			PushLink( &revive->reviveStack, member );

			if( !revive->fieldCb.IsEmpty() )
			{
				Local<Function> save = revive->fieldCb;
				revive->fieldCb.Clear();
				lprintf( "Cleared callback (FOR ARRAY)" );
				if( sub_v->IsObject() )
					buildObject( val->contains, sub_v.As<Object>(), revive );
				else
					lprintf( "current container is not an object, cannot build sub-object into it" );
				revive->fieldCb = save;
				sub_o_orig = sub_v;
			}else{
				if( sub_v->IsObject() ) {
					// this isn't assigned yet....
					buildObject( val->contains, sub_v.As<Object>(), revive );
				}
				sub_o_orig = sub_v;
			}
			delete (struct reviveStackMember*)PopLink( &revive->reviveStack );

			if( revive->failed ) {
				lprintf( "Revival is already failed..." );
			}
			// this is the call, 1 time after an object completes, with NULL arguments
			// this allows a flush/entire substituion of the 'this' object.

			if( !revive->fieldCb.IsEmpty() && revive->fieldCb->IsFunction() ) {
				//lprintf( "Call with null parameters here?" );
				sub_v = revive->fieldCb->Call( revive->context, sub_v, 0, NULL ).ToLocalChecked();
				LogObject( sub_v );

			}

			if( revive->revive ) {
				Local<Value> args[2] = { revive->fieldName, sub_v };
				sub_v = revive->reviver->Call( revive->context, revive->_this, 2, args ).ToLocalChecked();
				LogObject( sub_v );

			}

			if( sub_v != sub_o_orig ) {
				if( !cb.IsEmpty() ) {

					MaybeLocal<Value> newObj;
#ifdef DEBUG_REVIVAL_CALLBACKS
					lprintf( "Call field callback..." );
#endif
					if( val->name ) {
						Local<Value> args[] = { String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked()
							, sub_v };
						// use the custom reviver to assign the field.
						newObj = cb->Call( revive->context, o, 2, args );
						LogObject( newObj.ToLocalChecked() );

					}
					else {
						Local<Value> args[] = { Number::New(revive->isolate, 0), sub_v };
						// use the custom reviver to assign the field.
						newObj = cb->Call( revive->context, o, 2, args );
						LogObject( newObj.ToLocalChecked() );

					}
					if( !newObj.IsEmpty() ) {
						//lprintf( "This didn't auto set..." );
						sub_v = newObj.ToLocalChecked();

						if( !sub_v->IsUndefined() ) {
							if( val->name )
								SETV( o, revive->fieldName, sub_v );
							else
								SETN( o, currentIndex, sub_v );
						}
					}
				}
				else if( val->name ) {
					SETV( o, revive->fieldName, sub_v );
				}
				else {
					SETN( o, currentIndex, sub_v );
				}
			}
			break;
		case JSOX_VALUE_OBJECT:
			{
				//lprintf( "Setting CB callback(Object)?", revive->fieldCb.IsEmpty() );
				//cb = revive->fieldCb; // used again later in a loop
				// fieldCb is set in getObject to the new class value
				// so we need to save what the current is in CB.
				// it should already be in CB?

				// this will change cb potentially.
				sub_v = getObject( revive, val );
				{
					struct reviveStackMember* member; member = new struct reviveStackMember();
					member->object = sub_v;
					if( val->name ) {
						member->isArray = false;
						member->fieldName = revive->fieldName;
						member->name = val->name;
						member->nameLen = val->nameLen;
					} else {
						member->isArray = true;
						member->index = currentIndex;
					}
					PushLink( &revive->reviveStack, member );
				}
				Local<Function> finalCb = revive->fieldCb;

				if( sub_v->IsObject() )
					buildObject( val->contains, sub_v.As<Object>(), revive );
				else
					lprintf( "Failed to build sub object, current value is not an object" ); // should not happen.

				delete (struct reviveStackMember*)PopLink( &revive->reviveStack );
#ifdef DEBUG_REVIVAL_CALLBACKS
				if( revive->failed ) {
					lprintf( "Revival is already failed..." );
				}
#endif
#if 0
				// this uses old callback instead of new one.
				if( !cb.IsEmpty() ) {
					// expect the callback to set the field.
					Local<Value> args[] = { revive->fieldName, sub_v };
					lprintf( "Dispatch a thing:" );
					MaybeLocal<Value> result = cb->Call( revive->context, o, 2, args );
					if (!result.IsEmpty()) {
						if (!result.ToLocalChecked()->IsUndefined()) {
							sub_v = result.ToLocalChecked();
						}
					}
					else {
						lprintf("Excception from call.");
					}
#if defined( DEBUG_REFERENCE_FOLLOW ) || defined( DEBUG_REVIVAL_CALLBACKS )
					lprintf( "called callback to set object value %.*s", val->nameLen, val->name );
#endif
				}
				else { 
#if defined( DEBUG_REFERENCE_FOLLOW ) || defined( DEBUG_REVIVAL_CALLBACKS )
					if( val->name )
						lprintf( "Set object value %.*s:", val->nameLen, val->name );
					else
						lprintf( "set value to index: %d", currentIndex );
#endif
					o->Set( context, revive->fieldName, sub_v );
				}
#endif
#ifdef DEBUG_REVIVAL_CALLBACKS
				lprintf( "OLD BUILD3?" );
#endif
				sub_o_orig = sub_v;
				// this is the call, 1 time after an object completes, with NULL arguments
				// this allows a flush/entire substituion of the 'this' object.
				if( !finalCb.IsEmpty() ) {
#ifdef DEBUG_REVIVAL_CALLBACKS
					lprintf( "Revive with empty parameters here" );
#endif
					MaybeLocal<Value> r = finalCb->Call( revive->context, sub_v, 0, NULL );
					if( !r.IsEmpty() ) {
						sub_v = r.ToLocalChecked();
						{
							// This handles replacing values that were used when this was the old value
#ifdef DEBUG_REVIVAL_CALLBACKS
							lprintf( "Finall got the resolved value, let's check the stack?" );
#endif
							if( revive->reviveStack->Top )
							{
								struct reviveStackMember* member = (struct reviveStackMember*)PeekLink( &revive->reviveStack );
								if( member->pdlSubsts->Cnt ) {
									INDEX idx;
									struct reviveMemberReplacement* rep;
									DATA_FORALL( member->pdlSubsts, idx, struct reviveMemberReplacement*, rep ) {
										Local<Object> o = rep->object.As<Object>();
										o->Set( revive->context, rep->fieldName, sub_v );
#ifdef DEBUG_REVIVAL_CALLBACKS
										lprintf( "!!!!! REPLACED STUFF HERE(3)!" );
#endif
									}
								}
							}
						}
					} else {
//#ifdef DEBUG_REVIVAL_CALLBACKS
						lprintf( "Callback threw an exception" );
//#endif
						revive->failed = true;
					}
				}

#ifdef DEBUG_SET_FIELDCB
				lprintf( "Set protocon callback as field cb" );
#endif
				revive->fieldCb = cb; // restore fieldCb for remainder of this object's fields.

				{
					if( !cb.IsEmpty() ) {
						Local<Value> args[] = { revive->fieldName, sub_v };
						// use the custom reviver to assign the field.
#ifdef DEBUG_REVIVAL_CALLBACKS
						lprintf( "This finally sets the built value to the object" );
#endif
						MaybeLocal<Value> r = cb->Call(revive->context, o, 2, args);
						if (r.IsEmpty()) {
//#ifdef DEBUG_REVIVAL_CALLBACKS
							lprintf("Callback threw an exception");
//#endif
							revive->failed = true;
						} else
							sub_v = r.ToLocalChecked();
					}
				}

				if( revive->revive ) {
					Local<Value> args[2] = { revive->fieldName, sub_v };
					MaybeLocal<Value> r = revive->reviver->Call( revive->context, revive->_this, 2, args );
					if( !r.IsEmpty() )
						sub_v = r.ToLocalChecked();
					else {
//#ifdef DEBUG_REVIVAL_CALLBACKS
						lprintf( "Callback threw an exception" );
//#endif
						revive->failed = true;
					}
				}
				if( revive->failed )
					return;
				if( !sub_v->IsUndefined() )
					if( val->name ) {
						SETV( o, revive->fieldName, sub_v );
					}
					else {
						SETN( o, currentIndex, sub_v );
					}
			}
			break;
		}
	}
	//lprintf( "isUndefined?...%d", o.IsEmpty()?0:o->IsUndefined() );
	revive->fieldName = priorRefField;
	revive->refObject = priorRefObject;
}

Local<Value> convertMessageToJS2( PDATALIST msg, struct reviver_data *revive ) {
	Local<Object> o;
	Local<Value> v;// = Object::New( revive->isolate );

	struct jsox_value_container *val = (struct jsox_value_container *)GetDataItem( &msg, 0 );
	revive->fieldName = String::NewFromUtf8Literal( revive->isolate, "" );
	if( val && val->contains ) {
#ifdef DEBUG_REVIVAL_CALLBACKS
		lprintf( "makeValue3" );
#endif
		LOGICAL wantRevive = revive->revive;
		revive->revive = FALSE;
		Local<Value> root = makeValue( val, revive );
		revive->revive = wantRevive;
		o = root.As<Object>();

		revive->rootObject = o;
#ifdef DEBUG_REVIVAL_CALLBACKS
		lprintf( "Oldest thing" );
#endif
		Local<Function> cb;
		cb = revive->fieldCb;
		if( val->value_type == JSOX_VALUE_ARRAY ) {
			cb = revive->fieldCb;
			revive->fieldCb.Clear();
		}
		buildObject( val->contains, o, revive );
		if( revive->failed ) {
			lprintf( "Revival is already failed..." );
		}
		if( !revive->failed && val->className ) {
			MaybeLocal<Value> valmethod;

			// although at this time, this layer should always be empty(?)
			// this is the call, 1 time after an object completes, with NULL arguments
			// this allows a flush/entire substituion of the 'this' object.
			if( !cb.IsEmpty() && cb->IsFunction() ) {
				//lprintf( "Calling FINAL class reviver" );
				MaybeLocal<Value> retval = cb->Call( revive->context, o, 0, NULL );
				if( retval.IsEmpty() ) {
					//lprintf( "THIS FAILED BEFORE!? - THrew?" );
					return o;
				}
				//lprintf( "Had a field calllback, returning new object" );
				return o = retval.ToLocalChecked().As<Object>();
			}
			//else lprintf( "no field callback on this type..." );
			return o;
		}
		return o.As<Value>();
	}
	if( val ) {
#ifdef DEBUG_REVIVAL_CALLBACKS
		lprintf( "makeValue4" );
#endif
		return makeValue( val, revive );
	}
	return Undefined( revive->isolate );
}



void showTimings( const v8::FunctionCallbackInfo<Value>& args ) {
     uint32_t val;
#define LOGVAL(n) val = ConvertTickToMicrosecond( timings.deltas[n] ); printf( #n " : %d.%03d\n", val/1000, val%1000 );
LOGVAL(0);
LOGVAL(1);
LOGVAL(2);
LOGVAL(3);
LOGVAL(4);
LOGVAL(5);
LOGVAL(6);
LOGVAL(7);
	{
		int n;for(n=0;n<10;n++) timings.deltas[n] = 0;
	}
	logTick(-1);
}

void escapeJSOX( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = Isolate::GetCurrent();
	if( args.Length() == 0 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, TranslateText( "Missing parameter, string to escape" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	char *msg;
	String::Utf8Value tmp( isolate,  args[0] );
	size_t outlen;
	msg = jsox_escape_string_length( *tmp, tmp.length(), &outlen );
	args.GetReturnValue().Set( String::NewFromUtf8( isolate, msg, NewStringType::kNormal, (int)outlen ).ToLocalChecked() );
	Release( msg );
}


static Local<Value> ParseJSOX(  const char *utf8String, size_t len, struct reviver_data *revive, struct jsox_parse_state *state ) {
	PDATALIST parsed = NULL;
#ifdef DEBUG_INPUT
	lprintf("Parse:%.*s", len, utf8String );
#endif
	int result = jsox_parse_add_data( state, utf8String, len );
	if( !result )
		result = jsox_parse_add_data( state, NULL, 0 ); // expecting only a single object; go ahead and flush.
	//if( jxpsd._state ) jsox_parse_dispose_state( &jxpsd._state );
	if( result <= 0 || result > 1 ) { // 0 is 'no completed object.  1 is 'completed and no further' 2 is 'completed, and there's still data left.'
		//PTEXT error = jsox_parse_get_error( parser->state );
		//lprintf( "Failed to parse data..." );
		do {
			PTEXT error = jsox_parse_get_error( state );
			if( error ) {
				revive->isolate->ThrowException( Exception::Error( String::NewFromUtf8( revive->isolate, GetText( error ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
//#ifdef DEBUG_REVIVAL_CALLBACKS
				lprintf( "Error at high level parse - throw it to JS..." );
//#endif
				revive->failed = TRUE;
			} else {
				if( result > 1 ) {
					//lprintf( "WARNING: Extra data after JSOX message; Single message parse expects a closed, complete message." );
					break; // goto return anyway.
				}
				else {
					revive->isolate->ThrowException(Exception::Error(String::NewFromUtf8(revive->isolate, result > 1 ? "Extra data after message" : "Pending value could not complete", v8::NewStringType::kNormal ).ToLocalChecked() ));
//#ifdef DEBUG_REVIVAL_CALLBACKS
					lprintf( "(2)Error at high level parse - throw it to JS..." );
//#endif
					revive->failed = TRUE;
				}
			}
			LineRelease( error );
			return Undefined( revive->isolate );
		} while( 0 );
	}

	if( parsed && parsed->Cnt > 1 ) {
		lprintf( "Multiple values would result, invalid parse." );
		return Undefined(revive->isolate);
		// outside should always be a single value
	}
	parsed = jsox_parse_get_data( state ); // resulting message is removed from the parser.
	//logTick(3);
	Local<Value> value;

	revive->parser->currentReviver = revive;
	value = convertMessageToJS2( parsed, revive );
	//logTick(4);

	jsox_dispose_message( &parsed );
	//logTick(5);
	//lprintf( "RETURN REAL VALUE? %d %d", value.IsEmpty(), value.IsEmpty()?0:value->IsObject() );
	return value;
}

void JSOXObject::parse( const v8::FunctionCallbackInfo<Value>& args ){
	//logTick(0);
	struct reviver_data r;
	r.isolate = Isolate::GetCurrent();
	r.reviveStack = NULL;
	r.failed = FALSE;
	if( args.Length() == 0 ) {
		r.isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( r.isolate, TranslateText( "Missing parameter, data to parse" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	const char *msg;
	String::Utf8Value *tmp;
	Local<Function> reviver;
	Local<ArrayBuffer> ab;
	size_t len;
	if( args[0]->IsArrayBuffer() ) {
		tmp = NULL;
		ab = Local<ArrayBuffer>::Cast( args[0] );
#if ( NODE_MAJOR_VERSION >= 14 )
		msg = (const char*)ab->GetBackingStore()->Data();
#else
		msg = (const char*)ab->GetContents().Data();
#endif
		len = ab->ByteLength();
	}
	else {
		tmp = new String::Utf8Value( USE_ISOLATE( r.isolate ) args[0] );
		len = tmp[0].length();
		msg = *tmp[0];
	}
	r.parser = NULL;
	if( args.Length() > 1 ) {
		if( args[1]->IsFunction() ) {
			r._this = args.Holder();
			r.revive = TRUE;
			r.reviver = Local<Function>::Cast( args[1] );
		}
		else {
			r.isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( r.isolate, TranslateText( "Reviver parameter is not a function." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
//#ifdef DEBUG_REVIVAL_CALLBACKS
			lprintf( "(3)Error at high level parse - throw it to JS..." );
//#endif
			r.failed = TRUE;
			return;
		}
	}
	else
		r.revive = FALSE;

        //logTick(1);
	r.context = r.isolate->GetCurrentContext();
	r.parser = ObjectWrap::Unwrap<JSOXObject>( args.Holder() );
	struct jsox_parse_state *state = r.parser->state;
	args.GetReturnValue().Set( ParseJSOX( msg, len, &r, state ) );

	if( tmp )
		delete tmp;
	
}



void parseJSOX( const v8::FunctionCallbackInfo<Value>& args )
{
	//logTick(0);
	struct reviver_data r;
	r.isolate = Isolate::GetCurrent();
	r.reviveStack = NULL;
	r.failed = FALSE;
	if( args.Length() == 0 ) {
		r.isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( r.isolate, TranslateText( "Missing parameter, data to parse" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	const char *msg;
	String::Utf8Value *tmp;
	Local<Function> reviver;
	Local<ArrayBuffer> ab;
	size_t len;
	if( args[0]->IsArrayBuffer() ) {
		tmp = NULL;
		ab = Local<ArrayBuffer>::Cast( args[0] );
#if ( NODE_MAJOR_VERSION >= 14 )
		msg = (const char*)ab->GetBackingStore()->Data();
#else
		msg = (const char*)ab->GetContents().Data();
#endif
		len = ab->ByteLength();
	}
	else {
		tmp = new String::Utf8Value( USE_ISOLATE( r.isolate ) args[0] );
		len = tmp[0].length();
		msg = *tmp[0];
	}
	r.parser = new JSOXObject();
	if( args.Length() > 1 ) {
		if( args[1]->IsFunction() ) {
			r._this = args.Holder();
			r.revive = TRUE;
			r.reviver = Local<Function>::Cast( args[1] );
		}
		else {
			r.isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( r.isolate, TranslateText( "Reviver parameter is not a function." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
//#ifdef DEBUG_REVIVAL_CALLBACKS
			lprintf( "(4)Error at high level parse - throw it to JS..." );
//#endif
			r.failed = TRUE;
			return;
		}
	}
	else
		r.revive = FALSE;

	//logTick(1);
	r.context = r.isolate->GetCurrentContext();

	struct jsox_parse_state *state = r.parser->state;
	args.GetReturnValue().Set( ParseJSOX( msg, len, &r, state ) );
	//jsox_parse_dispose_state( &state ); // this is fairly cheap...
	if( tmp )
		delete tmp;
	delete r.parser;
}


void makeJSOX( const v8::FunctionCallbackInfo<Value>& args ) {
	args.GetReturnValue().Set( String::NewFromUtf8Literal( args.GetIsolate(), "undefined :) Stringify is not completed" ) );
}

void setFromPrototypeMap( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	class constructorSet *c = getConstructors( isolate );
	//lprintf( "Setting protomap from external...");
	c->fromPrototypeMap.Reset( args.GetIsolate(), args[0].As<Map>() );
	{
		Local<Context> context = isolate->GetCurrentContext();
		class constructorSet* c = getConstructors( isolate );
		if( c->dateNsCons.IsEmpty() ) {
#if 0
			Local<Script> script;
			Local<String> string = String::NewFromUtf8Literal( isolate, "DateNS" );
			script = Script::Compile( context, string
#if ( NODE_MAJOR_VERSION >= 16 )
				, new ScriptOrigin( isolate, String::NewFromUtf8Literal( isolate, "DateNSClassGetter" ) ) ).ToLocalChecked();
#else
				, new ScriptOrigin( String::NewFromUtf8Literal( isolate, "DateNSClassGetter" ) ) ).ToLocalChecked();
#endif
			MaybeLocal<Value> ml_result = script->Run( context );
			if( !ml_result.IsEmpty() ) {
				Local<Value> result = ml_result.ToLocalChecked();
				Local<Function> cons = Local<Function>::Cast( result );
				c->dateNsCons.Reset( isolate, cons );
			} else if( !args[1].IsEmpty() ) {
			}
#endif
			c->dateNsCons.Reset( isolate, args[1].As<Function>() );
		}
	}
}

void JSOXObject::setFromPrototypeMap( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	JSOXObject *parser = ObjectWrap::Unwrap<JSOXObject>( args.Holder() );
	parser->fromPrototypeMap.Reset( isolate, args[0].As<Map>() );
}
void JSOXObject::setPromiseFromPrototypeMap( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	JSOXObject *parser = ObjectWrap::Unwrap<JSOXObject>( args.Holder() );
	parser->promiseFromPrototypeMap.Reset( isolate, args[0].As<Map>() );
}
