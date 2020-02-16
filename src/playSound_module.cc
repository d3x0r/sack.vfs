
#include "global.h"


static void playMedia( const v8::FunctionCallbackInfo<Value>& args ) {
	if( args.Length() > 0 ) {
		String::Utf8Value sound( args.GetIsolate(), args[0] );
#if WIN32
		PlaySound( *sound, NULL, SND_ASYNC| SND_SYSTEM );
#endif
	}
}

static void loadMedia( const v8::FunctionCallbackInfo<Value>& args ) {

}

void SoundInit( Isolate *isolate, Local<Object> exports )
{
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> soundInterface = Object::New( isolate );

	//regInterface->Set( String::NewFromUtf8( isolate, "get", v8::NewStringType::kNormal ).ToLocalChecked(),

	NODE_SET_METHOD( soundInterface, "play", playMedia );
	NODE_SET_METHOD( soundInterface, "load", loadMedia );

	SET( exports, "sound", soundInterface );

}


