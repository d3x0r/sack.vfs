
#include "global.h"
#include <math.h>

static void buildObject( PDATALIST msg_data, Local<Object> o, struct reviver_data *revive );
static Local<Value> makeValue( struct jsox_value_container *val, struct reviver_data *revive, Local<Object> container, int index, Local<Value> name );

static struct timings {
	uint64_t start;
	uint64_t deltas[10];
}timings;

static void makeJSOX( const v8::FunctionCallbackInfo<Value>& args );
static void escapeJSOX( const v8::FunctionCallbackInfo<Value>& args );
static void parseJSOX( const v8::FunctionCallbackInfo<Value>& args );
static void setFromPrototypeMap( const v8::FunctionCallbackInfo<Value>& args );
static void showTimings( const v8::FunctionCallbackInfo<Value>& args );

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
		parseTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.jsox_parser", v8::NewStringType::kNormal ).ToLocalChecked() );
		parseTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "write", JSOXObject::write );
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "setFromPrototypeMap", JSOXObject::setFromPrototypeMap );
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "setPromiseFromPrototypeMap", JSOXObject::setPromiseFromPrototypeMap );

		class constructorSet *c = getConstructors( isolate );
		c->jsoxConstructor.Reset( isolate, parseTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );

		if( c->dateCons.IsEmpty() ) {
			Local<Date> tmp = Local<Date>::Cast( Date::New( context, 0 ).ToLocalChecked() );
			Local<Function> cons = Local<Function>::Cast(
				tmp->Get( context, String::NewFromUtf8( isolate, "constructor", NewStringType::kNormal ).ToLocalChecked() ).ToLocalChecked()
			);
			c->dateCons.Reset( isolate, cons );
		}

		//SET_READONLY( o2, "begin", parseTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
		SET( o2, "begin", parseTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	}

}

JSOXObject::JSOXObject() {
	state = jsox_begin_parse();
	prototypes = NULL;
}

JSOXObject::~JSOXObject() {
	jsox_parse_dispose_state( &state );
}

#define logTick(n) do { uint64_t tick = GetCPUTick(); if( n >= 0 ) timings.deltas[n] += tick-timings.start; timings.start = tick; } while(0)

void ReportException( v8::Isolate* isolate, v8::TryCatch* handler ) {
}

const char* ToCString( const v8::String::Utf8Value& value ) {
	return *value ? *value : "<string conversion failed>";
}

void JSOXObject::write( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	JSOXObject *parser = ObjectWrap::Unwrap<JSOXObject>( args.Holder() );
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Missing data parameter.", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}

	String::Utf8Value data( isolate,  args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	int result;
	//Local<Function> cb = Local<Function>::New( isolate, parser->readCallback );
	Local<Context> context = isolate->GetCurrentContext();
	Local<Value> global = context->Global();
	for( result = jsox_parse_add_data( parser->state, *data, data.length() );
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
			r.revive = FALSE;
			r.isolate = isolate;
			r.context = r.isolate->GetCurrentContext();
			r.parser = parser;
			argv[0] = convertMessageToJS2( elements, &r );
			{
				
				Local<Function> cb = Local<Function>::New( isolate, parser->readCallback );
				MaybeLocal<Value> cbResult = cb->Call( context, global, 1, argv );
				if( cbResult.IsEmpty() ) {
					lprintf( "Callback failed." );
					return;
				}
			}
		}
		jsox_dispose_message( &elements );
		if( result < 2 )
			break;
	}
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
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must callback to read into.", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		JSOXObject* obj = new JSOXObject();
		Local<Function> arg0 = Local<Function>::Cast( args[0] );
		obj->readCallback.Reset( isolate, arg0 );

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
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete[] argv;
	}

}



#define MODE NewStringType::kNormal
//#define MODE NewStringType::kInternalized

static inline Local<Value> makeValue( struct jsox_value_container *val, struct reviver_data *revive, Local<Object> container, int index, Local<Value> name ) {

	Local<Value> result;
	Local<Script> script;
	class constructorSet *c = getConstructors( revive->isolate );
	switch( val->value_type ) {
	case JSOX_VALUE_UNDEFINED:
		result = Undefined( revive->isolate );
		break;
	default:
		if( val->value_type >= JSOX_VALUE_TYPED_ARRAY && val->value_type <= JSOX_VALUE_TYPED_ARRAY_MAX ) {
			Local<ArrayBuffer> ab;
			if( val->value_type < JSOX_VALUE_TYPED_ARRAY_MAX )
				ab = ArrayBuffer::New( revive->isolate, val->string, val->stringLen, ArrayBufferCreationMode::kExternalized );
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
				lprintf( "Resolving a ref...." );
#endif
				{
					struct jsox_value_container *pathVal;
					INDEX idx;
					Local<Object> refObj = revive->rootObject;
					DATA_FORALL( val->contains, idx, struct jsox_value_container *, pathVal ) {
#ifdef DEBUG_REFERENCE_FOLLOW
						lprintf( "get reference:%s", pathVal->string );
#endif
						
						if( pathVal->value_type == JSOX_VALUE_NUMBER ) {
							Local<Value> arraymember = refObj->Get( revive->context, (uint32_t)pathVal->result_n ).ToLocalChecked();
							String::Utf8Value tmp( USE_ISOLATE( revive->isolate )   arraymember->ToString(revive->context).ToLocalChecked() );
#ifdef DEBUG_REFERENCE_FOLLOW
							lprintf( "Array member is : %s", *tmp );
#endif
							MaybeLocal<Object> maybeRefObj = arraymember->ToObject( revive->isolate->GetCurrentContext() );
							if( maybeRefObj.IsEmpty() ) {
								lprintf( "Referenced array member is not an object!. " );
								DebugBreak();
							}
							refObj = maybeRefObj.ToLocalChecked();
						}
						else if( pathVal->value_type == JSOX_VALUE_STRING ) {
							Local<Value> val;
							if( refObj->IsMap() ) {
								//Local<Map> map = refObj.As<Map>();
								Local<Function> mapGetter = refObj->Get( revive->context, localStringExternal( revive->isolate, "get" ) ).ToLocalChecked().As<Function>();
								Local<Value> args[] = { localStringExternal( revive->isolate, pathVal->string, (int)pathVal->stringLen ) };
								val = mapGetter->Call( revive->context, refObj, 1, args ).ToLocalChecked();
							}
							else {
								val = refObj->Get( revive->context
									, String::NewFromUtf8( revive->isolate
										, pathVal->string
										, NewStringType::kNormal
										, (int)pathVal->stringLen ).ToLocalChecked() ).ToLocalChecked();
							}
							if( val->IsObject() )
								refObj = val->ToObject( revive->isolate->GetCurrentContext() ).ToLocalChecked();
							else
								return val;
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
	case JSOX_VALUE_STRING:
		result = String::NewFromUtf8( revive->isolate, val->string, MODE, (int)val->stringLen ).ToLocalChecked();
		if( val->className ) {
			MaybeLocal<Value> valmethod;
			Local<Function> cb;
			if( revive->parser && !revive->parser->promiseFromPrototypeMap.IsEmpty() ) {
				
				valmethod = revive->parser->promiseFromPrototypeMap.Get( revive->isolate )->
					Get( revive->context
						, String::NewFromUtf8( revive->isolate, val->className, v8::NewStringType::kNormal, (int)val->classNameLen ).ToLocalChecked()
					);
				if( !valmethod.IsEmpty() && !valmethod.ToLocalChecked()->IsUndefined() ) {
					struct PromiseWrapper *pw = makePromise( revive->context, revive->isolate );
					Local<Value> args[] = { result, pw->resolve.Get( revive->isolate ), pw->reject.Get( revive->isolate ) };
					cb = valmethod.ToLocalChecked().As<Function>();
					result = cb->Call( revive->context, revive->_this, 3, args ).ToLocalChecked();
					if( result.IsEmpty() ) {
						return Null( revive->isolate );
					}
				}
			}
			else {
				if( revive->parser && !revive->parser->fromPrototypeMap.IsEmpty() ) {
					valmethod = revive->parser->fromPrototypeMap.Get( revive->isolate )->
						Get( revive->context
							, String::NewFromUtf8( revive->isolate, val->className, v8::NewStringType::kNormal, (int)val->classNameLen ).ToLocalChecked()
						);
					if( !valmethod.IsEmpty() && !valmethod.ToLocalChecked()->IsUndefined() )
						cb = valmethod.ToLocalChecked().As<Function>();
				}
				if( valmethod.IsEmpty() || (valmethod.ToLocalChecked()->IsUndefined()) ) {
					valmethod = c->fromPrototypeMap.Get( revive->isolate )->
						Get( revive->context
							, String::NewFromUtf8( revive->isolate, val->className, v8::NewStringType::kNormal, (int)val->classNameLen ).ToLocalChecked()
						);
					if( !valmethod.IsEmpty() && !valmethod.ToLocalChecked()->IsUndefined() )
						cb = valmethod.ToLocalChecked().As<Function>();
				}

				if( !cb.IsEmpty() && cb->IsFunction() ) {
					Isolate *isolate = revive->isolate;
					Local<Object> ref = Object::New( revive->isolate );
					SET_READONLY( ref, "o", container );
					if( name->IsNull() )
						SET_READONLY( ref, "f", Number::New( isolate, index ) );
					else
						SET_READONLY( ref, "f", name );
					Local<Value> args[] = { result, ref };
					result = cb->Call( revive->context, result, 2, args ).ToLocalChecked();
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
	case JSOX_VALUE_ARRAY:
		result = Array::New( revive->isolate );
		break;
	case JSOX_VALUE_OBJECT:
		if( val->className ) {
			lprintf( "This is resulting with a new object becuase there's a classname:%s", val->className );
			result = Object::New( revive->isolate );
		}
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
			, new ScriptOrigin( String::NewFromUtf8( revive->isolate, "BigIntFormatter", NewStringType::kInternalized ).ToLocalChecked() ) ).ToLocalChecked();
		result = script->Run( revive->context ).ToLocalChecked();
		//result = BigInt::New( revive->isolate, 0 );
		break;
	case JSOX_VALUE_DATE:
		{
			class constructorSet* c = getConstructors( revive->isolate );
			static const int argc = 1;
			Local<Value> argv[argc] = { String::NewFromUtf8( revive->isolate, val->string, NewStringType::kNormal ).ToLocalChecked() };
			result = c->dateCons.Get( revive->isolate )->NewInstance( revive->context, argc, argv ).ToLocalChecked();
		}
		break;
	}
	if( revive->revive ) {
		Local<Value> args[2] = { revive->value, result };
		Local<Value> r = revive->reviver->Call( revive->context, revive->_this, 2, args ).ToLocalChecked();
	}
	return result;
}

Local<Object> getObject( struct reviver_data *revive, struct jsox_value_container *val ) {
	Local<Object> sub_o;
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
		if( mprotoDef.IsEmpty() && revive->parser && !revive->parser->fromPrototypeMap.IsEmpty() ) {
			mprotoDef = revive->parser->fromPrototypeMap.Get( revive->isolate )->Get( revive->context, className );
			if( !mprotoDef.IsEmpty() && !mprotoDef.ToLocalChecked()->IsUndefined() ) {
				if( !mprotoDef.ToLocalChecked()->IsObject() ) {
					String::Utf8Value data( revive->isolate, mprotoDef.ToLocalChecked()->ToString( revive->isolate->GetCurrentContext() ).ToLocalChecked() );

					lprintf( "Expected prototype definition object... failed. %s", *data );
					
				}
				else {
					protoDef = mprotoDef.ToLocalChecked().As<Object>();
					Local<Value> p = protoDef->Get( revive->context, String::NewFromUtf8( revive->isolate, "protoCon", v8::NewStringType::kNormal, (int)8 ).ToLocalChecked() ).ToLocalChecked();
					Local<Value> f = protoDef->Get( revive->context, String::NewFromUtf8( revive->isolate, "cb", v8::NewStringType::kNormal, (int)2 ).ToLocalChecked() ).ToLocalChecked();
					if( f->IsFunction() )
						revive->fieldCb = f.As<Function>();
					else
						revive->fieldCb.Clear();
					if( p->IsFunction() ) {
						MaybeLocal<Object> mo = p.As<Function>()->NewInstance( revive->context, 0, NULL );
						if( !mo.IsEmpty() )
							sub_o = mo.ToLocalChecked();
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
					Local<Value> p = protoDef->Get( revive->context, String::NewFromUtf8( revive->isolate, "protoCon", v8::NewStringType::kNormal, (int)8 ).ToLocalChecked() ).ToLocalChecked();
					Local<Value> f = protoDef->Get( revive->context, String::NewFromUtf8( revive->isolate, "cb", v8::NewStringType::kNormal, (int)2 ).ToLocalChecked() ).ToLocalChecked();
					{
						String::Utf8Value db1( revive->isolate, p->ToString( revive->isolate ) );
						String::Utf8Value db2( revive->isolate, f->ToString( revive->isolate ) );
						lprintf( "WHAT? %s %s", *db1, *db2 );
					}
					if( f->IsFunction() )
						revive->fieldCb = f.As<Function>();
					else
						revive->fieldCb.Clear();
					if( p->IsFunction() ) {
						MaybeLocal<Object> mo = p.As<Function>()->NewInstance( revive->context, 0, NULL );
						if( !mo.IsEmpty() )
							sub_o = mo.ToLocalChecked();
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
		revive->fieldCb.Clear();
		sub_o = Object::New( revive->isolate );
	}

	return sub_o;
}

static Local<Object> getArray( struct reviver_data* revive, struct jsox_value_container* val ) {
	Local<Object> sub_o;
	if( val->className ) {
		MaybeLocal<Value> mprotoDef;
		Local<Object> protoDef;
		Local<String> className = String::NewFromUtf8( revive->isolate, val->className, v8::NewStringType::kNormal, (int)val->classNameLen ).ToLocalChecked();

		if( mprotoDef.IsEmpty() && revive->parser && !revive->parser->fromPrototypeMap.IsEmpty() ) {
			mprotoDef = revive->parser->fromPrototypeMap.Get( revive->isolate )->Get( revive->context, className );
			if( !mprotoDef.IsEmpty() && !mprotoDef.ToLocalChecked()->IsUndefined() ) {
				if( !mprotoDef.ToLocalChecked()->IsObject() ) {
					String::Utf8Value data( revive->isolate, mprotoDef.ToLocalChecked()->ToString( revive->isolate->GetCurrentContext() ).ToLocalChecked() );
					lprintf( "Expected prototype definition object... failed. %s", *data );
				}
				else {
					protoDef = mprotoDef.ToLocalChecked().As<Object>();
					Local<Value> p = protoDef->Get( revive->context, String::NewFromUtf8( revive->isolate, "protoCon", v8::NewStringType::kNormal, (int)8 ).ToLocalChecked() ).ToLocalChecked();
					Local<Value> f = protoDef->Get( revive->context, String::NewFromUtf8( revive->isolate, "cb", v8::NewStringType::kNormal, (int)2 ).ToLocalChecked() ).ToLocalChecked();
					if( f->IsFunction() )
						revive->fieldCb = f.As<Function>();
					else
						revive->fieldCb.Clear();
					if( p->IsFunction() ) {
						MaybeLocal<Object> mo = p.As<Function>()->NewInstance( revive->context, 0, NULL );
						if( !mo.IsEmpty() )
							sub_o = mo.ToLocalChecked();
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
					Local<Value> p = protoDef->Get( revive->context, String::NewFromUtf8( revive->isolate, "protoCon", v8::NewStringType::kNormal, (int)8 ).ToLocalChecked() ).ToLocalChecked();
					Local<Value> f = protoDef->Get( revive->context, String::NewFromUtf8( revive->isolate, "cb", v8::NewStringType::kNormal, (int)2 ).ToLocalChecked() ).ToLocalChecked();
					if( f->IsFunction() )
						revive->fieldCb = f.As<Function>();
					else
						revive->fieldCb.Clear();
					if( p->IsFunction() ) {
						MaybeLocal<Object> mo = p.As<Function>()->NewInstance( revive->context, 0, NULL );
						if( !mo.IsEmpty() )
							sub_o = mo.ToLocalChecked();
					}
				}
			}
		}
	}
	else {
		revive->fieldCb.Clear();
	}
	if( sub_o.IsEmpty() )
		sub_o = Array::New( revive->isolate );
	return sub_o;
}

static void buildObject( PDATALIST msg_data, Local<Object> o, struct reviver_data *revive ) {
	Isolate* isolate = revive->isolate;
	Local<Context> context = revive->context;
	Local<Value> thisVal;
	Local<String> stringKey;
	Local<Value> thisKey;
	struct jsox_value_container *val;
	Local<Object> sub_o;
	INDEX idx;
	int index = 0;
	int currentIndex;
	DATA_FORALL( msg_data, idx, struct jsox_value_container*, val )
	{
		Local<Function> cb;
		Local<Object> sub_o_orig;
#ifdef DEBUG_REFERENCE_FOLLOW
		lprintf( "value name is : %d %s", val->value_type, val->name ? val->name : "(NULL)" );
#endif
		switch( val->value_type ) {
		case JSOX_VALUE_EMPTY: // only occurs in arrays...
			SETN( o, index, Undefined(isolate) );
			o->Delete( revive->context, index );
			index++;
			break;
		default:
			if( !revive->fieldCb.IsEmpty() ) {
				if( val->name ) {
					stringKey = String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked();
					Local<Value> args[] = { stringKey, makeValue( val, revive, o, 0, stringKey ) };
					// use the custom reviver to assign the field.
					revive->fieldCb->Call( revive->context, o, 2, args );
				}
				else {
					Local<Value> args[] = { Number::New( isolate, index++ ), makeValue( val, revive, o, 0, stringKey ) };
					// use the custom reviver to assign the field.
					revive->fieldCb->Call( revive->context, o, 2, args );
				}
			} else if( val->name ) {
				stringKey = String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked();
				revive->value = stringKey;
#ifdef DEBUG_REFERENCE_FOLLOW
				lprintf( "set value to fieldname: %s", val->name );
#endif
					o->CreateDataProperty( revive->context, stringKey
						, makeValue( val, revive, o, 0, stringKey ) );
			}
			else {
				if( revive->revive )
					revive->value = Integer::New( revive->isolate, index );
				//lprintf( "set value to index: %d", index );
				SETN( o, index, thisVal = makeValue( val, revive, o, index, Null(isolate).As<Value>() ) );
				index++;
			}
			break;
		case JSOX_VALUE_ARRAY:
			cb = revive->fieldCb;
			sub_o = getArray( revive, val );
			if( !cb.IsEmpty() ) {
				if( val->name ) {
					Local<Value> args[] = { stringKey = String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked()
					                      , sub_o };
					// use the custom reviver to assign the field.
					cb->Call( revive->context, o, 2, args );
				} else {
					Local<Value> args[] = { Number::New( isolate, index++ ), sub_o };
					// use the custom reviver to assign the field.
					cb->Call( revive->context, o, 2, args );
				}
			}
			else
				if( val->name ) {
					//lprintf( "set value to fieldname: %s", val->name );
					o->CreateDataProperty( revive->context,
						stringKey = String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked()
						, sub_o );
				thisKey = stringKey;
			}
			else {
				if( revive->revive )
					thisKey = Integer::New( revive->isolate, index );
				//lprintf( "set value to index: %d", index );
				SETN( o, index++, sub_o );
			}

			buildObject( val->contains, sub_o, revive );
			sub_o_orig = sub_o;

			// this is the call, 1 time after an object completes, with NULL arguments
			// this allows a flush/entire substituion of the 'this' object.
			if( !revive->fieldCb.IsEmpty() && revive->fieldCb->IsFunction() ) {
				sub_o = revive->fieldCb->Call( revive->context, sub_o, 0, NULL ).ToLocalChecked().As<Object>();
			}

			revive->fieldCb = cb;

			if( revive->revive ) {
				Local<Value> args[2] = { thisKey, sub_o };
				sub_o = revive->reviver->Call( revive->context, revive->_this, 2, args ).ToLocalChecked().As<Object>();
			}

			if( sub_o != sub_o_orig ) {
				if( !revive->fieldCb.IsEmpty() ) {
					if( val->name ) {
						Local<Value> args[] = { String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked()
							, sub_o };
						// use the custom reviver to assign the field.
						cb->Call( revive->context, o, 2, args );
					}
					else {
						Local<Value> args[] = { sub_o };
						// use the custom reviver to assign the field.
						cb->Call( revive->context, o, 1, args );
					}
				}
				else if( val->name ) {
					SETV( o, thisKey, sub_o );
				}
				else {
					SETN( o, currentIndex, sub_o );
				}
			}
			break;
		case JSOX_VALUE_OBJECT:
			{
				Local<Function> cb; cb = revive->fieldCb;
				// this will change cb potentially.
				sub_o = getObject( revive, val );

				// this uses old callback instead of new one.
				if( !cb.IsEmpty() ) {
					if( val->name ) {
						Local<Value> args[] = { String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked()
							, sub_o };
						// use the custom reviver to assign the field.
						cb->Call( revive->context, o, 2, args );
					}
					else {
						Local<Value> args[] = { Number::New( isolate, index++ ), sub_o };
						// use the custom reviver to assign the field.
						cb->Call( revive->context, o, 2, args );
					}
				}
				else if( val->name ) {
					stringKey = String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked();
					o->CreateDataProperty( revive->context, stringKey, sub_o );
					thisKey = stringKey;
				}
				else {
					if( revive->revive )
						thisKey = Integer::New( revive->isolate, index );
					//lprintf( "set value to index: %d", index );

					SETN( o, (currentIndex = index++), sub_o );
				}

				buildObject( val->contains, sub_o, revive );
				
				sub_o_orig = sub_o;
				// this is the call, 1 time after an object completes, with NULL arguments
				// this allows a flush/entire substituion of the 'this' object.
				if( !revive->fieldCb.IsEmpty() && revive->fieldCb->IsFunction() ) {
					sub_o = revive->fieldCb->Call( revive->context, sub_o, 0, NULL ).ToLocalChecked().As<Object>();
				}

				revive->fieldCb = cb; // restore fieldCb for remainder of this object's fields.

				if( revive->revive ) {
					Local<Value> args[2] = { thisKey, sub_o };
					sub_o = revive->reviver->Call( revive->context, revive->_this, 2, args ).ToLocalChecked().As<Object>();
				}

				if( sub_o != sub_o_orig ) {
					if( !revive->fieldCb.IsEmpty() ) {
						if( val->name ) {
							Local<Value> args[] = { String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked()
								, sub_o };
							// use the custom reviver to assign the field.
							cb->Call( revive->context, o, 2, args );
						}
						else {
							Local<Value> args[] = { sub_o };
							// use the custom reviver to assign the field.
							cb->Call( revive->context, o, 1, args );
						}
					} else if( val->name ) {
						SETV( o, thisKey, sub_o );
					}
					else {
						SETN( o, currentIndex, sub_o );
					}
				}
			}
			break;
		}
	}
}

Local<Value> convertMessageToJS2( PDATALIST msg, struct reviver_data *revive ) {
	Local<Object> o;
	Local<Value> v;// = Object::New( revive->isolate );

	struct jsox_value_container *val = (struct jsox_value_container *)GetDataItem( &msg, 0 );
	if( val && val->contains ) {
		if( val->value_type == JSOX_VALUE_OBJECT )
			o = getObject( revive, val );
		else if( val->value_type == JSOX_VALUE_ARRAY )
			o = Array::New( revive->isolate );
		revive->rootObject = o;
		if( val->className ) {
			MaybeLocal<Value> valmethod;
			Local<Function> cb;

			buildObject( val->contains, o, revive );

			// although at this time, this layer should always be empty(?)
			// this is the call, 1 time after an object completes, with NULL arguments
			// this allows a flush/entire substituion of the 'this' object.
			if( !revive->fieldCb.IsEmpty() && revive->fieldCb->IsFunction() ) {
				return o = revive->fieldCb->Call( revive->context, o, 0, NULL ).ToLocalChecked().As<Object>();
			}
			return o;
		}
		buildObject( val->contains, o, revive );
		return o.As<Value>();
	}
	if( val )
		return makeValue( val, revive, Null( revive->isolate ).As<Object>(), 0, Null( revive->isolate).As<String>() );
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


Local<Value> ParseJSOX(  const char *utf8String, size_t len, struct reviver_data *revive ) {
	PDATALIST parsed = NULL;

	struct jsox_parse_state *state = jsox_begin_parse();
	//static struct jsox_parse_state *_state;
	int result = jsox_parse_add_data( state, utf8String, len );
	//if( jxpsd._state ) jsox_parse_dispose_state( &jxpsd._state );
	if( result <= 0 ) {
		//PTEXT error = jsox_parse_get_error( parser->state );
		//lprintf( "Failed to parse data..." );
		PTEXT error = jsox_parse_get_error( state );
		if( error )
			revive->isolate->ThrowException( Exception::Error( String::NewFromUtf8( revive->isolate, GetText( error ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		else
			revive->isolate->ThrowException( Exception::Error( String::NewFromUtf8( revive->isolate, "No Error Text" STRSYM(__LINE__), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		LineRelease( error );
		return Undefined(revive->isolate);
	}

	if( parsed && parsed->Cnt > 1 ) {
		lprintf( "Multiple values would result, invalid parse." );
		return Undefined(revive->isolate);
		// outside should always be a single value
	}
	parsed = jsox_parse_get_data( state ); // resulting message is removed from the parser.
	revive->parser = new JSOXObject();
        //logTick(3);
	Local<Value> value = convertMessageToJS2( parsed, revive );
        //logTick(4);
	delete revive->parser;

	jsox_dispose_message( &parsed );
	jsox_parse_dispose_state( &state ); // this is fairly cheap...
    //logTick(5);

	return value;
}

void parseJSOX( const v8::FunctionCallbackInfo<Value>& args )
{
	//logTick(0);
	struct reviver_data r;
	r.isolate = Isolate::GetCurrent();
	if( args.Length() == 0 ) {
		r.isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( r.isolate, TranslateText( "Missing parameter, data to parse" ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	const char *msg;
	String::Utf8Value tmp( USE_ISOLATE( r.isolate ) args[0] );
	Local<Function> reviver;
	msg = *tmp;
	r.parser = NULL;
	if( args.Length() > 1 ) {
		if( args[1]->IsFunction() ) {
			r._this = args.Holder();
			r.value = String::NewFromUtf8( r.isolate, "", v8::NewStringType::kNormal ).ToLocalChecked();
			r.revive = TRUE;
			r.reviver = Local<Function>::Cast( args[1] );
		}
		else {
			r.isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( r.isolate, TranslateText( "Reviver parameter is not a function." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
			return;
		}
	}
	else
		r.revive = FALSE;

        //logTick(1);
	r.context = r.isolate->GetCurrentContext();

	args.GetReturnValue().Set( ParseJSOX( msg, tmp.length(), &r ) );

}


void makeJSOX( const v8::FunctionCallbackInfo<Value>& args ) {
	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), "undefined :) Stringify is not completed", v8::NewStringType::kNormal ).ToLocalChecked() );
}

void setFromPrototypeMap( const v8::FunctionCallbackInfo<Value>& args ) {
	class constructorSet *c = getConstructors( args.GetIsolate() );
	c->fromPrototypeMap.Reset( args.GetIsolate(), args[0].As<Map>() );
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
