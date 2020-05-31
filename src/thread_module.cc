
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
	class constructorSet *c = getConstructors( isolate );
	c->threadConstructor.Reset( isolate, threadTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	SET( exports, "Thread",
		threadTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
}

//-----------------------------------------------------------

void ThreadObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	class constructorSet* c = getConstructors( isolate );
	if( args.Length() ) {
		if( c->ThreadObject_idleProc.IsEmpty() && args[0]->IsFunction() ) {
			Local<Function> arg0 = Local<Function>::Cast( args[0] );
			c->ThreadObject_idleProc.Reset( isolate, arg0 );
		}
	}
	else
		c->ThreadObject_idleProc.Reset();
}
//-----------------------------------------------------------

static bool cbWoke;
void ThreadObject::wake( const v8::FunctionCallbackInfo<Value>& args ) {
	cbWoke = true;
}

//-----------------------------------------------------------
void ThreadObject::relinquish( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = Isolate::GetCurrent();
	class constructorSet* c = getConstructors( isolate );

	if( c->ThreadObject_idleProc.IsEmpty() ) {
		lprintf( "relinquish failed; no idle proc registered." );
		return;
	}
	Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
	MaybeLocal<Value> r = cb->Call( isolate->GetCurrentContext(), Null(isolate), 0, NULL );
	if( r.IsEmpty() ) {
		// this should never happen... and I don't really care if there was an exception....
	}
	// r was always undefined.... so inner must wake.
	//String::Utf8Value fName( r->ToString() );
	//lprintf( "tick callback resulted %s", (char*)*fName);
	if( !cbWoke )
		if( uv_run( c->loop, UV_RUN_NOWAIT ) )
			uv_run( c->loop, UV_RUN_ONCE);
	cbWoke = false;
}

//-----------------------------------------------------------

ThreadObject::ThreadObject( )
{
}

ThreadObject::~ThreadObject() {
}

