
#include "global.h"
#include <ffmpeg_interface.h>

struct sound_cache_entry {
	TEXTSTR name;
	struct ffmpeg_file* file;
};

static struct sound_static_data {
	PLIST sounds;
} ssd;

// ffmpeg_UnloadFile

static void ended( uintptr_t sound_done ) {

}

static void playMedia( const v8::FunctionCallbackInfo<Value>& args ) {
	if( args.Length() > 0 ) {
		String::Utf8Value sound( args.GetIsolate(), args[0] );
		uintptr_t endedData = 0;
		struct ffmpeg_file* file = ffmpeg_LoadFile( *sound, NULL, 0, NULL, NULL, 0, ended, endedData, NULL );
		ffmpeg_PlayFile( file );
		ffmpeg_UnloadFile( file );
		//PlaySound( *sound, NULL, SND_ASYNC );
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


