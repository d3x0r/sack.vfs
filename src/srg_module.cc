
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
	static PLINKQUEUE signingEntropies;
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
			{
				// if the callback exceptions, this will blindly continue to generate random entropy....
				MaybeLocal<Value> result = cb->Call( obj->isolate->GetCurrentContext()->Global(), 1, argv );
				if( result.IsEmpty() )
					return;
			}
			for( uint32_t n = 0; n < ui->Length(); n++ ) {
				Local<Value> elem = ui->Get( n );
				String::Utf8Value val( USE_ISOLATE( obj->isolate ) elem->ToString());
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
					String::Utf8Value seed( USE_ISOLATE( isolate ) args[0]->ToString() );
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
			args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
			delete[] argv;
		}
	}
	static void reset( const v8::FunctionCallbackInfo<Value>& args ) {
		SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
		SRG_ResetEntropy( obj->entropy );
	}
	static void seed( const v8::FunctionCallbackInfo<Value>& args ) {
		if( args.Length() > 0 ) {
			String::Utf8Value seed( USE_ISOLATE( args.GetIsolate() ) args[0]->ToString() );
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
			int32_t bits = args[0]->Int32Value( obj->isolate->GetCurrentContext() ).FromMaybe( 0 );
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
			PARRAY_BUFFER_HOLDER holder = GetHolder();
			holder->o.Reset( obj->isolate, arrayBuffer );
			holder->o.SetWeak< ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
			holder->buffer = buffer;

			args.GetReturnValue().Set( arrayBuffer );
		}
	}

	~SRGObject() {
		SRG_DestroyEntropy( &entropy );
	}

	static POINTER nextSalt;
	static size_t nextSaltLen;
	static void feedSignSalt( uintptr_t psvUser, POINTER *salt, size_t *saltlen ) {
		salt[0] = nextSalt;
		saltlen[0] = nextSaltLen;
	}

	static LOGICAL signCheck( uint8_t *buf ) {
		int n, b;
		int is0 = 0;
		int is1 = 0;
		int long0 = 0;
		int long1 = 0;
		int longest0 = 0;
		int longest1 = 0;
		int ones = 0;
		for( n = 0; n < 32; n++ ) {
			for( b = 0; b < 8; b++ ) {
				if( buf[n] & (1 << b) ) {
					ones++;
					if( is1 ) {
						long1++;
					}
					else {
						if( long0 > longest0 ) longest0 = long0;
						is1 = 1;
						is0 = 0;
						long1 = 1;
					}
				}
				else {
					if( is0 ) {
						long0++;
					}
					else {
						if( long1 > longest1 ) longest1 = long1;
						is0 = 1;
						is1 = 0;
						long0 = 1;
					}
				}
			}
		}

// 167-128 = 39 = 40+ dif == 30 bits in a row approx
#define overbal (167-128)
		if( longest0 > 29 || longest1 > 29 || ones > (128+overbal) || ones < (128-overbal) ) {
			if( ones > ( 128+overbal )|| ones < (128 - overbal) )
				printf( "STRMb: %d %d  0s:%d 1s:%d ", longest0, longest1, 256-ones, ones );
			else
				printf( "STRMl: %d %d  0s:%d 1s:%d ", longest0, longest1, 256 - ones, ones );
			return 1;
		}
		return 0;
	}

	static void sign( const v8::FunctionCallbackInfo<Value>& args ) {
		String::Utf8Value buf( args[0]->ToString() );
		//SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
		char *id;
		int tries = 0;
		POINTER state = NULL;
		nextSalt = NewArray( uint8_t, nextSaltLen = 32 );
		//memcpy( nextSalt, *buf, buf.length() );
		struct random_context *signEntropy = (struct random_context *)DequeLink( &signingEntropies );
		if( !signEntropy )
			signEntropy = SRG_CreateEntropy2( feedSignSalt, (uintptr_t)0 );

		SRG_ResetEntropy( signEntropy );
		SRG_FeedEntropy( signEntropy, (const uint8_t*)*buf, buf.length() );
		SRG_SaveState( signEntropy, &state );
		do {
			SRG_RestoreState( signEntropy, state );
			{
				size_t len;
				uint8_t outbuf[32];
				uint8_t *bytes;
				id = SRG_ID_Generator();
				bytes = DecodeBase64Ex( id, 40, &len, (const char*)1 );
				memcpy( (uint8_t*)nextSalt, bytes, 32 );
				Release( bytes );
				SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );
				tries++;
				if( signCheck( outbuf ) )
					printf( " %d  %s\n", tries, id );
				else {
					Release( id );
					id = NULL;
				}
			}
		} while( !id );
		EnqueLink( &signingEntropies, signEntropy );
		args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), id ) );
		Release( state );
	}

	static void verify( const v8::FunctionCallbackInfo<Value>& args ) {
		if( args.Length() > 1 ) {
			String::Utf8Value buf( args[0]->ToString() );
			String::Utf8Value hash( args[1]->ToString() );
			//SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
			char *id;
			int tries = 0;
			struct random_context *signEntropy = (struct random_context *)DequeLink( &signingEntropies );
			nextSalt = NewArray( uint8_t, nextSaltLen = buf.length() + 32 );
			memcpy( nextSalt, *buf, buf.length() );

			if( !signEntropy )
				signEntropy = SRG_CreateEntropy2( feedSignSalt, (uintptr_t)0 );
			SRG_ResetEntropy( signEntropy );
			{
				size_t len;
				uint8_t outbuf[32];
				uint8_t *bytes;
				id = *hash;
				bytes = DecodeBase64Ex( id, 40, &len, (const char*)1 );
				memcpy( ((uint8_t*)nextSalt) + nextSaltLen - 32, bytes, 32 );
				Release( bytes );
				SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );
				tries++;
				if( signCheck( outbuf ) )
					args.GetReturnValue().Set( True( args.GetIsolate() ) );
				else {
					args.GetReturnValue().Set( False( args.GetIsolate() ) );
				}
			}
		}
	}

};

POINTER SRGObject::nextSalt;
size_t  SRGObject::nextSaltLen;
PLINKQUEUE SRGObject::signingEntropies;
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
	Local<Function> f = srgTemplate->GetFunction();
	SRGObject::constructor.Reset( isolate, f );

	SET_READONLY( exports, "SaltyRNG", f );
	SET_READONLY_METHOD( f, "sign", SRGObject::sign );
	SET_READONLY_METHOD( f, "verify", SRGObject::verify );

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

