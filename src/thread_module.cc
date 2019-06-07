
#include "global.h"

//-----------------------------------------------------------


void ThreadObject::Init( Local<Object> exports ) {
	Isolate* isolate = Isolate::GetCurrent();
	Local<Context> context = isolate->GetCurrentContext();
	NODE_SET_METHOD(exports, "Δ", relinquish );
	NODE_SET_METHOD(exports, "Λ", wake );
	Local<FunctionTemplate> threadTemplate;
	// Prepare constructor template
	threadTemplate = FunctionTemplate::New( isolate, New );
	threadTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.Thread", v8::NewStringType::kNormal ).ToLocalChecked() );
	threadTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap

	// Prototype

	constructor.Reset( isolate, threadTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	SET( exports, "Thread",
		threadTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
}

//-----------------------------------------------------------
Persistent<Function> ThreadObject::idleProc;

void ThreadObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() ) {
		if( idleProc.IsEmpty() && args[0]->IsFunction() ) {
			Local<Function> arg0 = Local<Function>::Cast( args[0] );
			idleProc.Reset( isolate, arg0 );
		}
	}
	else
		idleProc.Reset();
}
//-----------------------------------------------------------

static bool cbWoke;

void ThreadObject::wake( const v8::FunctionCallbackInfo<Value>& args ) {
	cbWoke = true;
}

//-----------------------------------------------------------
void ThreadObject::relinquish( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = Isolate::GetCurrent();
	//int delay = 0;
	//if( args.Length() > 0 && args[0]->IsNumber() )
	//	delay = (int)args[0]->ToNumber()->Value();

	//	nodeThread = MakeThread();
	if( idleProc.IsEmpty() ) {
		lprintf( "relinquish failed; no idle proc registered." );
		return;
	}
	Local<Function>cb = Local<Function>::New( isolate, idleProc );
	MaybeLocal<Value> r = cb->Call( isolate->GetCurrentContext(), Null(isolate), 0, NULL );
	if( r.IsEmpty() ) {
		// this should never happen... and I don't really care if there was an exception....
	}
	// r was always undefined.... so inner must wake.
	//String::Utf8Value fName( r->ToString() );
	//lprintf( "tick callback resulted %s", (char*)*fName);
	if( !cbWoke )
		if( uv_run( uv_default_loop(), UV_RUN_NOWAIT ) )
			uv_run( uv_default_loop(), UV_RUN_ONCE);
	cbWoke = false;
	/*
	if( delay ) {

		lprintf( "short sleep", delay, delay );
		WakeableSleep( 20 );
		lprintf( "short wake", delay, delay );
		cb->Call(Null(isolate), 0, NULL );
		uv_run( uv_default_loop(), UV_RUN_DEFAULT);
	
		lprintf( "sleep for %08x, %d", delay, delay );
		WakeableSleep( delay );
	}
	cb->Call(Null(isolate), 0, NULL );
	uv_run( uv_default_loop(), UV_RUN_DEFAULT);
	*/
}

//-----------------------------------------------------------

Persistent<Function> ThreadObject::constructor;
ThreadObject::ThreadObject( )
{
}

ThreadObject::~ThreadObject() {
}

