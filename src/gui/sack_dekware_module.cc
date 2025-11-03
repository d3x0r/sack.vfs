
#include "../global.h"

static void startDekware(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	class constructorSet* c = getConstructors(isolate);
	String::Utf8Value startArgs(isolate, args[0]->ToString(context).ToLocalChecked());
	// comma is a special path character indicating install path.
	void (*f)( char*) = (void(*)(char*))LoadFunction( ",/lib/SACK/applicationCore/dekware.core", "Startup");
	f(*startArgs);
	
}

void InitDekware(Isolate *isolate, Local<Object> exports) {
	
	Local<Context> context = isolate->GetCurrentContext();
	class constructorSet* c = getConstructors(isolate);
	Local<Object> dekwareNamespace;

	dekwareNamespace = Object::New(isolate);
	SET(dekwareNamespace, "start", Function::New(context, startDekware).ToLocalChecked());

	SET(exports, "Dekware", dekwareNamespace);
}
