
#include "global.h"


class SRGObject : public node::ObjectWrap {

public:
	char *seedBuf;
	size_t seedLen;
	struct random_context *entropy;
	static v8::Persistent<v8::Function> constructor;
	Persistent<Function, CopyablePersistentTraits<Function>> *seedCallback;
	Isolate *isolate;
	Persistent<Array> seedArray;
public:

	static void Init( Isolate *isolate, Handle<Object> exports );
	SRGObject( Persistent<Function, CopyablePersistentTraits<Function>> *callback );
	SRGObject( const char *seed, size_t seedLen );
	SRGObject();

private:
	static void getSeed( uintptr_t psv, POINTER *salt, size_t *salt_size ) {
		SRGObject* obj = (SRGObject*)psv;
		if( obj->seedCallback ) {
			Local<Function> cb = Local<Function>::New( obj->isolate, obj->seedCallback[0] );
			Local<Array> ui = Array::New( obj->isolate );
			Local<Value> argv[] = { ui };
			cb->Call( obj->isolate->GetCurrentContext()->Global(), 1, argv );
			for( uint32_t n = 0; n < ui->Length(); n++ ) {
				Local<Value> elem = ui->Get( n );
				String::Utf8Value val(elem->ToString());
				obj->seedBuf = (char*)Reallocate( obj->seedBuf, obj->seedLen + val.length() );
				memcpy( obj->seedBuf + obj->seedLen, (*val), val.length() );
			}
		}
		if( obj->seedBuf ) {
			salt[0] = (POINTER)obj->seedBuf;
			salt_size[0] = obj->seedLen;
			Deallocate( char *, obj->seedBuf );
			obj->seedBuf = NULL;
			obj->seedLen = 0;
		}
		else
			salt_size[0] = 0;
	}
	static void New( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		if( args.IsConstructCall() ) {
			SRGObject* obj;
			int argc = args.Length();
			if( argc > 0 ) {
				if( args[0]->IsFunction() ) {
					Handle<Function> arg0 = Handle<Function>::Cast( args[0] );
					obj = new SRGObject( new Persistent<Function, CopyablePersistentTraits<Function>>( isolate, arg0 ) );
				}
				else {
					String::Utf8Value seed( args[0]->ToString() );
					obj = new SRGObject( *seed, seed.length() );
				}
				obj->Wrap( args.This() );
				args.GetReturnValue().Set( args.This() );
			}
			else
			{
				obj = new SRGObject();
				obj->Wrap( args.This() );
				args.GetReturnValue().Set( args.This() );
			}
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
	static void reset( const v8::FunctionCallbackInfo<Value>& args ) {
		SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
		SRG_ResetEntropy( obj->entropy );
	}
	static void seed( const v8::FunctionCallbackInfo<Value>& args ) {
		if( args.Length() > 0 ) {
			String::Utf8Value seed( args[0]->ToString() );
			SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
			if( obj->seedBuf )
				Deallocate( char *, obj->seedBuf );
			obj->seedBuf = StrDup( *seed );
			obj->seedLen = seed.length();
		}
	}
	static void getBits( const v8::FunctionCallbackInfo<Value>& args ) {
		SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
		obj->isolate = args.GetIsolate();
		int32_t r;
		if( !args.Length() )
			r = SRG_GetEntropy( obj->entropy, 32, true );
		else {
			int32_t bits = args[0]->Int32Value();
			bool sign = false;
			if( args.Length() > 1 )
				sign = args[0]->BooleanValue();
			r = SRG_GetEntropy( obj->entropy, bits, sign );
		}
		args.GetReturnValue().Set( Integer::New( obj->isolate, r ) );
	}
	static void getBuffer( const v8::FunctionCallbackInfo<Value>& args ) {
		SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
		obj->isolate = args.GetIsolate();
		if( !args.Length() ) {
			obj->isolate->ThrowException( Exception::Error( String::NewFromUtf8( obj->isolate, "required parameter missing, count of bits" ) ) );
		}
		else {
			int32_t bits = args[0]->Int32Value();
			uint32_t *buffer = NewArray( uint32_t, (bits +31)/ 32 );
			SRG_GetEntropyBuffer( obj->entropy, buffer, bits );
			
			Local<Object> arrayBuffer = ArrayBuffer::New( obj->isolate, buffer, (bits+7)/8 );
			args.GetReturnValue().Set( arrayBuffer );
		}
	}

	~SRGObject() {
		SRG_DestroyEntropy( &entropy );
	}
};

v8::Persistent<v8::Function> SRGObject::constructor;


void InitSRG( Isolate *isolate, Handle<Object> exports ) {
	SRGObject::Init( isolate, exports );
}

void SRGObject::Init( Isolate *isolate, Handle<Object> exports )
{
	Local<FunctionTemplate> srgTemplate;
	srgTemplate = FunctionTemplate::New( isolate, New );
	srgTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.srg" ) );
	srgTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "seed", SRGObject::seed );
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "reset", SRGObject::reset );
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "getBits", SRGObject::getBits );
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "getBuffer", SRGObject::getBuffer );

	SRGObject::constructor.Reset( isolate, srgTemplate->GetFunction() );

	exports->Set( String::NewFromUtf8( isolate, "SaltyRNG" ), srgTemplate->GetFunction() );

}

SRGObject::SRGObject( Persistent<Function, CopyablePersistentTraits<Function>> *callback ) {
	this->seedBuf = NULL;
	this->seedLen = 0;
	this->seedCallback = callback;

	this->entropy = SRG_CreateEntropy2( SRGObject::getSeed, (uintptr_t) this );
}

SRGObject::SRGObject() {
	this->seedBuf = NULL;
	this->seedLen = 0;
	this->seedCallback = NULL;
	this->entropy = SRG_CreateEntropy2( SRGObject::getSeed, (uintptr_t) this );
}

SRGObject::SRGObject( const char *seed, size_t seedLen ) {
	this->seedBuf = StrDup( seed );
	this->seedLen = seedLen;
	this->seedCallback = NULL;
	this->entropy = SRG_CreateEntropy2( SRGObject::getSeed, (uintptr_t) this );
}