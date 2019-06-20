
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
	PLINKQUEUE SigningEntropies = NULL;
public:

	static void Init( Isolate *isolate, Local<Object> exports );
	SRGObject( Persistent<Function, CopyablePersistentTraits<Function>> *callback );
	SRGObject( const char *seed, size_t seedLen );
	SRGObject();

private:

	static void idGenerator( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		if( args.Length() ) {
			int version = -1;
			if( args[0]->IsString() ) {
				char *r;
				struct random_context *ctx;
				if( args.Length() > 1 && args[1]->IsNumber() )
					version = (int)args[1]->IntegerValue( args.GetIsolate()->GetCurrentContext() ).FromMaybe(0);
				switch( version ) {
				case 0:
					ctx = SRG_CreateEntropy( NULL, 0 );
					break;
				case 1:

					ctx = SRG_CreateEntropy2_256( NULL, 0 );
					break;
				case 2:
					ctx = SRG_CreateEntropy3( NULL, 0 );
					break;
				default:
				case 3:
					ctx = SRG_CreateEntropy4( NULL, 0 );
					break;
				}
				String::Utf8Value val( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );

				/* Regenerator version 0 */
				uint32_t buf[256 / 32];
				SRG_FeedEntropy( ctx, (uint8_t*)"\0\0\0\0", 4 );
				SRG_StepEntropy( ctx );
				SRG_FeedEntropy( ctx, (uint8_t*)* val, val.length() );
				SRG_StepEntropy( ctx );
				SRG_GetEntropyBuffer( ctx, buf, 256 );

				size_t outlen;
				r = EncodeBase64Ex( (uint8_t*)buf, (16 + 16), &outlen, (const char*)1 );
				SRG_DestroyEntropy( &ctx );

				args.GetReturnValue().Set( localString( isolate, r, (int)outlen ) );
			}
			else
			{
				if( args[0]->IsNumber() )
					version = (int)args[0]->IntegerValue( args.GetIsolate()->GetCurrentContext() ).FromMaybe( 0 );

				char *r;
				switch( version ) {
				case 0:
					r = SRG_ID_Generator();
					break;
				case 1:
					r = SRG_ID_Generator_256();
					break;
				case 2:
					r = SRG_ID_Generator3();
					break;
				default:
				case 3:
					r = SRG_ID_Generator4();
					break;
				}
				args.GetReturnValue().Set( localString( isolate, r, (int)strlen(r) ) );
			}
		} else {
			char *r = SRG_ID_Generator();
			args.GetReturnValue().Set( localString( isolate, r, (int)strlen( r ) ) );
		}
	}

	static void getSeed( uintptr_t psv, POINTER *salt, size_t *salt_size ) {
		SRGObject* obj = (SRGObject*)psv;
		Isolate* isolate = obj->isolate;
		Local<Context> context = isolate->GetCurrentContext();
		if( obj->seedBuf ) {
			Deallocate( char *, obj->seedBuf );
			obj->seedBuf = NULL;
			obj->seedLen = 0;
		}
		if( obj->seedCallback ) {
			Local<Function> cb = Local<Function>::New( obj->isolate, obj->seedCallback[0] );
			Local<Array> ui = Array::New( obj->isolate );
			Local<Value> argv[] = { ui };
			{
				// if the callback exceptions, this will blindly continue to generate random entropy....
				MaybeLocal<Value> result = cb->Call( obj->isolate->GetCurrentContext(), obj->isolate->GetCurrentContext()->Global(), 1, argv );
				if( result.IsEmpty() )
					return;
			}
			for( uint32_t n = 0; n < ui->Length(); n++ ) {
				Local<Value> elem = GETN( ui, n );
				String::Utf8Value val( USE_ISOLATE( obj->isolate ) elem->ToString(obj->isolate->GetCurrentContext() ).ToLocalChecked()  );
				obj->seedBuf = (char*)Reallocate( obj->seedBuf, obj->seedLen + val.length() );
				memcpy( obj->seedBuf + obj->seedLen, (*val), val.length() );
			}
		}
		if( obj->seedBuf ) {
			salt[0] = (POINTER)obj->seedBuf;
			salt_size[0] = obj->seedLen;
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
					Local<Function> arg0 = Local<Function>::Cast( args[0] );
					obj = new SRGObject( new Persistent<Function, CopyablePersistentTraits<Function>>( isolate, arg0 ) );
				}
				else {
					String::Utf8Value seed( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
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
	static void setVersion( const v8::FunctionCallbackInfo<Value>& args ) {
		SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
		if( args.Length() > 0 ) {
			int64_t version = args[0]->IntegerValue(args.GetIsolate()->GetCurrentContext()).ToChecked();
			//lprintf( "Make will be %d?", version );
			if( version == 0 )
				obj->MakeEntropy = SRG_CreateEntropy;
			if( version == 1 )
				obj->MakeEntropy = SRG_CreateEntropy2_256;
			if( version == 2 )
				obj->MakeEntropy = SRG_CreateEntropy3;
			if( version == 3 )
				obj->MakeEntropy = SRG_CreateEntropy4;

			SRGObject *srg = ObjectWrap::Unwrap<SRGObject>( args.This() );
			srg->entropy = obj->MakeEntropy( SRGObject::getSeed, (uintptr_t)srg );

		}
	}
	static void reset( const v8::FunctionCallbackInfo<Value>& args ) {
		SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
		SRG_ResetEntropy( obj->entropy );
	}
	static void seed( const v8::FunctionCallbackInfo<Value>& args ) {
		if( args.Length() > 0 ) {
			String::Utf8Value seed( USE_ISOLATE( args.GetIsolate() ) args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
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
				sign = args[0]->TOBOOL( obj->isolate );
			r = SRG_GetEntropy( obj->entropy, bits, sign );
		}
		args.GetReturnValue().Set( Integer::New( obj->isolate, r ) );
	}
	static void getBuffer( const v8::FunctionCallbackInfo<Value>& args ) {
		SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
		obj->isolate = args.GetIsolate();
		if( !args.Length() ) {
			obj->isolate->ThrowException( Exception::Error( String::NewFromUtf8( obj->isolate, "required parameter missing, count of bits", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		}
		else {
			int32_t bits = args[0]->Int32Value( args.GetIsolate()->GetCurrentContext() ).FromMaybe(0);
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


	struct bit_count_entry {
		uint8_t ones, in0, in1, out0, out1, changes;
	};
	static struct bit_count_entry bit_counts[256];

	static void InitBitCountLookupTables( void ) {
		int in0 = 0;
		int in1 = 0;
		int out0 = 0;
		int out1 = 0;
		int ones;
		int n;
		for( n = 0; n < 256; n++ ) {
			int changed;
			in0 = 0;
			in1 = 0;
			out0 = 0;
			out1 = 0;
			ones = 0;
			changed = 1;
			if( (n & (1 << 7)) ) ones++;
			if( (n & (1 << 6)) ) ones++;
			if( (n & (1 << 5)) ) ones++;
			if( (n & (1 << 4)) ) ones++;
			if( (n & (1 << 3)) ) ones++;
			if( (n & (1 << 2)) ) ones++;
			if( (n & (1 << 1)) ) ones++;
			if( (n & (1 << 0)) ) ones++;
			do {
				if( (n & (1 << 7)) ) {
					out1++;
					if( (n & (1 << 6)) ) out1++; else break;
					if( (n & (1 << 5)) ) out1++; else break;
					if( (n & (1 << 4)) ) out1++; else break;
					if( (n & (1 << 3)) ) out1++; else break;
					if( (n & (1 << 2)) ) out1++; else break;
					if( (n & (1 << 1)) ) out1++; else break;
					if( (n & (1 << 0)) ) out1++; else break;
					changed = 0;
				}
				else {
					out0++;
					if( !(n & (1 << 6)) ) out0++; else break;
					if( !(n & (1 << 5)) ) out0++; else break;
					if( !(n & (1 << 4)) ) out0++; else break;
					if( !(n & (1 << 3)) ) out0++; else break;
					if( !(n & (1 << 2)) ) out0++; else break;
					if( !(n & (1 << 1)) ) out0++; else break;
					if( !(n & (1 << 0)) ) out0++; else break;
					changed = 0;
				}
			} while( 0 );
			if( !changed ) {
				in1 = out1; in0 = out0;
			} else do {
				if( (n & (1 << 0)) ) {
					in1++;
					if( (n & (1 << 1)) ) in1++; else break;
					if( (n & (1 << 2)) ) in1++; else break;
					if( (n & (1 << 3)) ) in1++; else break;
					if( (n & (1 << 4)) ) in1++; else break;
					if( (n & (1 << 5)) ) in1++; else break;
					if( (n & (1 << 6)) ) in1++; else break;
					if( (n & (1 << 7)) ) in1++; else break;
				}
				else {
					in0++;
					if( !(n & (1 << 1)) ) in0++; else break;
					if( !(n & (1 << 2)) ) in0++; else break;
					if( !(n & (1 << 3)) ) in0++; else break;
					if( !(n & (1 << 4)) ) in0++; else break;
					if( !(n & (1 << 5)) ) in0++; else break;
					if( !(n & (1 << 6)) ) in0++; else break;
					if( !(n & (1 << 7)) ) in0++; else break;
				}
			} while( 0 );
			bit_counts[n].in0 = in0;
			bit_counts[n].in1 = in1;
			bit_counts[n].out0 = out0;
			bit_counts[n].out1 = out1;
			bit_counts[n].changes = changed;
			bit_counts[n].ones = ones;
		}
	}

	struct signature {
		const char *id;
		int extent;
		int classifier;
	};
	static int signCheck( uint8_t *buf, int del1, int del2, struct signature *s ) {
		int n;
		int is0 = bit_counts[buf[0]].in0 != 0;
		int is1 = bit_counts[buf[0]].in1 != 0;
		int long0 = 0;
		int long1 = 0;
		int longest0 = 0;
		int longest1 = 0;
		int ones = 0;
		int rval;
		//LogBinary( buf, 32 );
		for( n = 0; n < 32; n++ ) {
			struct bit_count_entry *e = bit_counts + buf[n];
			ones += e->ones;
			if( is0 && e->in0 ) long0 += e->in0;
			if( is1 && e->in1 ) long1 += e->in1;
			if( e->changes ) {
				if( long0 > longest0 ) longest0 = long0;
				if( long1 > longest1 ) longest1 = long1;
				if( long0 = e->out0 ) {
					is0 = 1;
					is1 = 0;
				} 
				else {
					is1 = 1;
					is0 = 0;
				}
				long1 = e->out1;
			}
			else {
				if( !is0 && e->in0 ) {
					if( long1 > longest1 ) longest1 = long1;
					long0 = e->out0;
					long1 = e->out1;
					is0 = 1;
					is1 = 0;
				}
				else if( !is1 && e->in1 ) {
					if( long0 > longest0 ) longest0 = long0;
					long0 = e->out0;
					long1 = e->out1;
					is0 = 0;
					is1 = 1;
				}
			}
		}
		if( long0 > longest0 ) longest0 = long0;
		if( long1 > longest1 ) longest1 = long1;

// 167-128 = 39 = 40+ dif == 30 bits in a row approx
#define overbal (167-128)
		if( longest0 > (29+del1) || longest1 > (29+del1) || ones > (128+overbal+del2) || ones < (128-overbal-del2) ) {
			if( ones > ( 128 + overbal + del2 ) ) {
				s->classifier = rval = 1;
				s->extent = ones-128 - overbal;
			} 
			else if( ones < (128 - overbal - del2) ) {
				s->classifier = rval = 2;
				s->extent = 128-ones - overbal;
			}
			else if( longest0 > ( 29 + del1 ) ) {
				s->classifier = rval = 3;
				s->extent = longest0 - 29;
			}
			else if( longest1 > (29 + del1) ) {
				s->classifier = rval = 4;
				s->extent = longest1 - 29;
			}
			else {
				s->classifier = rval = 5;
				s->extent = 0;
			}
			return rval;
		}
		s->classifier = 0;
		s->extent = 0;
		return 0;
	}


	struct signParams {
		PTHREAD main;
		struct random_context *signEntropy;
		POINTER state; // this thread's entropy working state
		//char *salt;  // this thread's salt.
		//int saltLen;
#ifdef DEBUG_SIGNING
		int tries;
#endif
		char *id;  // result ID
		int pad1;  // extra length 1 to try
		int pad2;  // extra length 2 to try
		int ended; // this thread has ended.
		int *done; // all threads need to be done.
	};
	static int signingThreads;
	int SigningThreads = 1;
	static struct random_context *(*makeEntropy)( void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size), uintptr_t psv_user );
	struct random_context *(*MakeEntropy)(void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size), uintptr_t psv_user);

	static uintptr_t signWork( PTHREAD thread ) {
		struct signParams *params = (struct signParams *)GetThreadParam( thread );

		do {
			{
				size_t len;
				uint8_t outbuf[32];
				uint8_t *bytes;
				int passed_as;
				struct signature s;
				params->id = SRG_ID_Generator_256();
				bytes = DecodeBase64Ex( params->id, 44, &len, (const char*)1 );
				SRG_ResetEntropy( params->signEntropy );
				SRG_FeedEntropy( params->signEntropy, bytes, 32 );
				SRG_GetEntropyBuffer( params->signEntropy, (uint32_t*)outbuf, 256 );
				Release( params->id );
				params->id = EncodeBase64Ex( outbuf, 32, &len, (const char*)1 );
				SRG_RestoreState( params->signEntropy, params->state );
				SRG_FeedEntropy( params->signEntropy, outbuf, 32 );
				SRG_GetEntropyBuffer( params->signEntropy, (uint32_t*)outbuf, 256 );

#ifdef DEBUG_SIGNING
				params->tries++;
#endif
				if( (passed_as = signCheck( outbuf, params->pad1, params->pad2, &s )) ) {
#ifdef DEBUG_SIGNING
					lprintf( "FEED %s", params->id );
					LogBinary( bytes, len );
					lprintf( "GOT" );
					LogBinary( outbuf, 256 / 8 );
					printf( " %d  %s  %d\n", params->tries, params->id, passed_as );
#endif
				} 
				else {
					Release( params->id );
					params->id = NULL;
				}
				Release( bytes );
			}
		} while( !params->id && !params->done[0] );
		if( !params->done[0] ) {
			params->done[0] = TRUE;
			WakeThread( params->main );
		}
		params->ended = 1;
		return 0;
	}

	static void sign( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		String::Utf8Value buf( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		static signParams threadParams[32];
		int found = 0;
#ifdef DEBUG_SIGNING
		int tries = 0;
#endif
		int done = 0;
		int pad1 = 0, pad2 = 0;
		int n = 0;
		int argn = 1;
		int threads = signingThreads;
		POINTER state = NULL;
		while( argn < args.Length() ) {
			if( args[argn]->IsNumber() ) {
				if( n ) {
					pad2 = args[argn]->Int32Value( isolate->GetCurrentContext() ).FromMaybe(0);
				}
				else {
					n = 1;
					pad1 = args[argn]->Int32Value( isolate->GetCurrentContext() ).FromMaybe(0);
				}
			}
			argn++;
		}

#ifdef DEBUG_SIGNING
		lprintf( "RESET ENTROPY TO START" );
		LogBinary( (const uint8_t*)*buf, buf.length() );
#endif

		for( n = 0; n < threads; n++ ) {
			threadParams[n].main = MakeThread();
			if( !threadParams[n].signEntropy )
				threadParams[n].signEntropy = makeEntropy( NULL, 0 );

			SRG_ResetEntropy( threadParams[n].signEntropy );
			SRG_FeedEntropy( threadParams[n].signEntropy, (const uint8_t*)*buf, buf.length() );
			threadParams[n].state = NULL;
			SRG_SaveState( threadParams[n].signEntropy, &threadParams[n].state, NULL );
			//threadParams[n].salt = SRG_ID_Generator_256(); // give the thread a starting point
			//threadParams[n].saltLen = (int)strlen( threadParams[n].salt );
			threadParams[n].pad1 = pad1;
			threadParams[n].pad2 = pad2;
			threadParams[n].ended = 0;
#ifdef DEBUG_SIGNING
			threadParams[n].tries = 0;
#endif
			threadParams[n].done = &done;
			ThreadTo( signWork, (uintptr_t)(threadParams +n) );
		}
	}

	static void setThraads( const v8::FunctionCallbackInfo<Value>& args ) {
		if( args.Length() )
			signingThreads = args[0]->Int32Value( args.GetIsolate()->GetCurrentContext() ).FromMaybe(0);
	}


	static void verify( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		Local<Context> context = isolate->GetCurrentContext();
		if( args.Length() > 1 ) {
			String::Utf8Value buf( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
			String::Utf8Value hash( USE_ISOLATE( isolate ) args[1]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
			//SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
			char *id;
			int pad1 = 0, pad2 = 0;
			int n = 0;
			int argn = 1;
			struct random_context *signEntropy = (struct random_context *)DequeLink( &signingEntropies );
			while( argn < args.Length() ) {
				if( args[argn]->IsNumber() ) {
					if( n ) {
						pad2 = args[argn]->Int32Value( isolate->GetCurrentContext() ).FromMaybe(0);
					} else {
						n = 1;
						pad1 = args[argn]->Int32Value( isolate->GetCurrentContext() ).FromMaybe(0);
					}
				}
				argn++;
			}

			if( !signEntropy )
				signEntropy = makeEntropy( NULL, (uintptr_t)0 );
			SRG_ResetEntropy( signEntropy );
			SRG_FeedEntropy( signEntropy, (const uint8_t*)*buf, buf.length() );
			{
				size_t len;
				uint8_t outbuf[32];
				uint8_t *bytes;
				id = *hash;
				bytes = DecodeBase64Ex( id, 44, &len, (const char*)1 );
#ifdef DEBUG_SIGNING
				lprintf( "FEED INIT %s", id );
				LogBinary( (*buf), buf.length() );
				lprintf( "FEED" );
				LogBinary( bytes,len );
#endif
				SRG_FeedEntropy( signEntropy, bytes, len );
				Release( bytes );
				SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );
#ifdef DEBUG_SIGNING
				lprintf( "GET" );
				LogBinary( outbuf, 256 / 8 );
#endif
				Local<Object> result = Object::New( isolate );
				struct signature s;
				signCheck( outbuf, pad1, pad2, &s );
				SET( result, "classifier", Number::New( args.GetIsolate(), s.classifier ) );
				SET( result, (const char*)"extent", Number::New( args.GetIsolate(), s.extent ) );
				char *rid = EncodeBase64Ex( outbuf, 256 / 8, &len, (const char *)1 );
				SET( result, (const char*)"key", localString( isolate, rid, (int)(len -1)) );
				args.GetReturnValue().Set( result );
			}
			EnqueLink( &signingEntropies, signEntropy );
		}
	}

	static void srg_sign_work( signParams threadParams[32], int *done, const uint8_t *buf, size_t bufLen, int pad1, int pad2 )
	{
		int n;
		//static signParams threadParams[32];
		for( n = 0; n < signingThreads; n++ ) {
			threadParams[n].main = MakeThread();
			if( !threadParams[n].signEntropy )
				threadParams[n].signEntropy = SRG_CreateEntropy4( NULL, 0 );

			SRG_ResetEntropy( threadParams[n].signEntropy );
			SRG_FeedEntropy( threadParams[n].signEntropy, buf, bufLen );
			threadParams[n].state = NULL;
			SRG_SaveState( threadParams[n].signEntropy, &threadParams[n].state, NULL );
			//threadParams[n].salt = SRG_ID_Generator_256(); // give the thread a starting point
			//threadParams[n].saltLen = (int)strlen( threadParams[n].salt );
			threadParams[n].pad1 = pad1;
			threadParams[n].pad2 = pad2;
			threadParams[n].ended = 0;
#ifdef DEBUG_SIGNING
			threadParams[n].tries = 0;
#endif
			threadParams[n].done = done;
			ThreadTo( signWork, (uintptr_t)(threadParams + n) );
		}

		while( !done ) {
			WakeableSleep( 500 );
		}
		for( n = 0; n < signingThreads; n++ ) {
			while( !threadParams[n].ended ) Relinquish();
#ifdef DEBUG_SIGNING
			tries += threadParams[n].tries;
#endif
		}
		int found = 0;
		for( n = 0; n < signingThreads; n++ ) {
			if( threadParams[n].id ) {
				if( found ) {
				}
				else {
#ifdef DEBUG_SIGNING
					lprintf( " %d  %s \n", tries, threadParams[n].id );
#endif
					//args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), threadParams[n].id ) );
					//threadParams[n].id
				}
				Release( threadParams[n].id );
				found++;
			}
			threadParams[n].id = NULL;
		}
		return;
	}

	static char * wait_for_signing( signParams threadParams[32], int *done ) {
		char *result;
		int n;
		while( !(*done) ) {
			WakeableSleep( 500 );
		}
		for( n = 0; n < signingThreads; n++ ) {
			while( !threadParams[n].ended ) Relinquish();
#ifdef DEBUG_SIGNING
			tries += threadParams[n].tries;
#endif
		}
		int found = 0;
		for( n = 0; n < signingThreads; n++ ) {
			while( !threadParams[n].ended )
				Relinquish();
			if( threadParams[n].id ) {
				if( found ) {
					Release( threadParams[n].id );
				}
				else {
#ifdef DEBUG_SIGNING
					lprintf( " %d  %s \n", tries, threadParams[n].id );
#endif
					result = threadParams[n].id;
				}
				found++;
			}
			threadParams[n].id = NULL;
		}
		return result;
	}


	static void srg_sign( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		SRGObject *srg = ObjectWrap::Unwrap<SRGObject>( args.This() );
		String::Utf8Value buf( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		static signParams threadParams[32];
		int found = 0;
#ifdef DEBUG_SIGNING
		int tries = 0;
#endif
		int done = 0;
		int pad1 = 0, pad2 = 0;
		int n = 0;
		int argn = 1;
		int threads = signingThreads;
		POINTER state = NULL;
		while( argn < args.Length() ) {
			if( args[argn]->IsNumber() ) {
				if( n ) {
					pad2 = args[argn]->Int32Value( isolate->GetCurrentContext() ).FromMaybe(0);
				}
				else {
					n = 1;
					pad1 = args[argn]->Int32Value( isolate->GetCurrentContext() ).FromMaybe(0);
				}
			}
			argn++;
		}

		srg_sign_work( threadParams, &done, (const uint8_t*)*buf, buf.length(), pad1, pad2 );
#ifdef DEBUG_SIGNING
		lprintf( "RESET ENTROPY TO START" );
		LogBinary( (const uint8_t*)*buf, buf.length() );
#endif

		while( !done ) {
			WakeableSleep( 500 );
		}
		for( n = 0; n < threads; n++ ) {
			while( !threadParams[n].ended ) Relinquish();
#ifdef DEBUG_SIGNING
			tries += threadParams[n].tries;
#endif
		}
		for( n = 0; n < threads; n++ ) {
			if( threadParams[n].id ) {
				if( found ) {
				}
				else {
#ifdef DEBUG_SIGNING
					lprintf( " %d  %s \n", tries, threadParams[n].id );
#endif
					args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), threadParams[n].id, v8::NewStringType::kNormal ).ToLocalChecked() );
				}
				Release( threadParams[n].id );
				found++;
			}
			threadParams[n].id = NULL;
		}
	}

	static void srg_setThraads( const v8::FunctionCallbackInfo<Value>& args ) {
		SRGObject *srg = ObjectWrap::Unwrap<SRGObject>( args.This() );
		if( args.Length() )
			srg->SigningThreads = args[0]->Int32Value( args.GetIsolate()->GetCurrentContext() ).FromMaybe(0);
	}


	static void srg_verify( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		Local<Context> context = isolate->GetCurrentContext();
		SRGObject *srg = ObjectWrap::Unwrap<SRGObject>( args.This() );
		if( args.Length() > 1 ) {
			String::Utf8Value buf( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
			String::Utf8Value hash( USE_ISOLATE( isolate ) args[1]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
			//SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
			char *id;
			int pad1 = 0, pad2 = 0;
			int n = 0;
			int argn = 1;
			struct random_context *signEntropy = (struct random_context *)DequeLink( &srg->SigningEntropies );
			while( argn < args.Length() ) {
				if( args[argn]->IsNumber() ) {
					if( n ) {
						pad2 = args[argn]->Int32Value( isolate->GetCurrentContext() ).FromMaybe(0);
					}
					else {
						n = 1;
						pad1 = args[argn]->Int32Value( isolate->GetCurrentContext() ).FromMaybe(0);
					}
				}
				argn++;
			}

			if( !signEntropy )
				signEntropy = srg->MakeEntropy( NULL, (uintptr_t)0 );
			SRG_ResetEntropy( signEntropy );
			SRG_FeedEntropy( signEntropy, (const uint8_t*)*buf, buf.length() );
			{
				size_t len;
				uint8_t outbuf[32];
				uint8_t *bytes;
				id = *hash;
				bytes = DecodeBase64Ex( id, 44, &len, (const char*)1 );
				SRG_ResetEntropy( signEntropy );
				SRG_FeedEntropy( signEntropy, bytes, len );
				SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );

				SRG_ResetEntropy( signEntropy );
				SRG_FeedEntropy( signEntropy, (const uint8_t*)*buf, buf.length() );

#ifdef DEBUG_SIGNING
				lprintf( "FEED INIT %s", id );
				LogBinary( (*buf), buf.length() );
				lprintf( "FEED" );
				LogBinary( bytes, len );
#endif
				SRG_FeedEntropy( signEntropy, outbuf, 32 );
				Release( bytes );
				SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );
#ifdef DEBUG_SIGNING
				lprintf( "GET" );
				LogBinary( outbuf, 256 / 8 );
#endif
				Local<Object> result = Object::New( isolate );
				struct signature s;
				signCheck( outbuf, pad1, pad2, &s );
				SET( result, "classifier", Integer::New( isolate, s.classifier ) );
				SET( result, "extent", Integer::New( isolate, s.extent ) );
				char *rid = EncodeBase64Ex( outbuf, 256 / 8, &len, (const char *)1 );
				SET( result, "key", localString( isolate, rid, (int)(len - 1) ) );
				args.GetReturnValue().Set( result );
			}
			EnqueLink( &srg->SigningEntropies, signEntropy );
		}
	}
};

int SRGObject::signingThreads = 1;
struct random_context *(*SRGObject::makeEntropy)(void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size), uintptr_t psv_user) = SRG_CreateEntropy2_256;

struct SRGObject::bit_count_entry SRGObject::bit_counts[256];
PLINKQUEUE SRGObject::signingEntropies;
v8::Persistent<v8::Function> SRGObject::constructor;


void InitSRG( Isolate *isolate, Local<Object> exports ) {
	SRGObject::Init( isolate, exports );
}

void SRGObject::Init( Isolate *isolate, Local<Object> exports )
{
	InitBitCountLookupTables();
	Local<FunctionTemplate> srgTemplate;
	srgTemplate = FunctionTemplate::New( isolate, New );
	srgTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.srg", v8::NewStringType::kNormal ).ToLocalChecked() );
	srgTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "seed", SRGObject::seed );
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "reset", SRGObject::reset );
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "getBits", SRGObject::getBits );
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "getBuffer", SRGObject::getBuffer );
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "setVersion", SRGObject::setVersion );
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "sign", SRGObject::srg_sign );
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "setSigningThreads", SRGObject::srg_setThraads );
	NODE_SET_PROTOTYPE_METHOD( srgTemplate, "verify", SRGObject::srg_verify );
	Local<Function> f = srgTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
	SRGObject::constructor.Reset( isolate, f );

	SET_READONLY( exports, "SaltyRNG", f );
	SET_READONLY_METHOD( f, "id", SRGObject::idGenerator );
	SET_READONLY_METHOD( f, "sign", SRGObject::sign );
	SET_READONLY_METHOD( f, "setSigningThreads", SRGObject::setThraads );
	SET_READONLY_METHOD( f, "verify", SRGObject::verify );

}

SRGObject::SRGObject( Persistent<Function, CopyablePersistentTraits<Function>> *callback ) {
	this->MakeEntropy = SRG_CreateEntropy2_256;
	this->seedBuf = NULL;
	this->seedLen = 0;
	this->seedCallback = callback;

	this->entropy = SRG_CreateEntropy2( SRGObject::getSeed, (uintptr_t) this );
}

SRGObject::SRGObject() {
	this->MakeEntropy = SRG_CreateEntropy2_256;
	this->seedBuf = NULL;
	this->seedLen = 0;
	this->seedCallback = NULL;
	this->entropy = SRG_CreateEntropy2( SRGObject::getSeed, (uintptr_t) this );
}

SRGObject::SRGObject( const char *seed, size_t seedLen ) {
	this->MakeEntropy = SRG_CreateEntropy2_256;
	this->seedBuf = StrDup( seed );
	this->seedLen = seedLen;
	this->seedCallback = NULL;
	this->entropy = SRG_CreateEntropy2_256( SRGObject::getSeed, (uintptr_t) this );
}

