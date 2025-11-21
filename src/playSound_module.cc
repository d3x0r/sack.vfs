#define FIX_RELEASE_COM_COLLISION

#include "global.h"
#if defined( SACK_CORE ) && defined( SACK_GUI )
#  include <ffmpeg_interface.h>

//#include <mmsystem.h>

#endif

#ifdef WIN32
#include "win32/PolicyConfig.h"
#include <Functiondiscoverykeys_devpkey.h>

#include <PropIdl.h>
#endif

struct sound_cache_entry {
	TEXTSTR name;
#if defined( SACK_CORE ) && defined( SACK_GUI )
	struct ffmpeg_file* file;
#endif
};

static void playMedia( const v8::FunctionCallbackInfo<Value>& args ) {
	if( args.Length() > 0 ) {
		String::Utf8Value sound( args.GetIsolate(), args[0] );
// ffmpeg functionality only exists in the -gui version.
#if defined( SACK_CORE ) && defined( SACK_GUI )
		uintptr_t endedData = 0;
		struct ffmpeg_file* file = ffmpeg_LoadFile( *sound, NULL, 0, NULL, NULL, 0, ended, endedData, NULL );
		ffmpeg_PlayFile( file );
		ffmpeg_UnloadFile( file );
#else
#  ifdef WIN32
		PlaySound( *sound, NULL, SND_ASYNC );
#  endif
#endif
	}
}

static void loadMedia( const v8::FunctionCallbackInfo<Value>& args ) {
	lprintf( "Load media function hasn't been implemented yet to pre-cache sounds" );
}

static void listDevices(Local<Name> property, const PropertyCallbackInfo<Value>& args) {
#ifdef WIN32
	Isolate *isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Array> list = Array::New(isolate);

	{
		HRESULT hr;
		IMMDeviceEnumerator* pEnum = NULL;
		// Create a multimedia device enumerator.
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
			CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
		if (SUCCEEDED(hr))
		{
			IMMDeviceCollection* pDevices;
			// Enumerate the output devices.
			LPWSTR wstrDefaultID = NULL;
			IMMDevice* defaultDevice;
			hr = pEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &defaultDevice);
			if (SUCCEEDED(hr)) {
				hr = defaultDevice->GetId(&wstrDefaultID);

				hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
				if (SUCCEEDED(hr))
				{
					UINT count;
					pDevices->GetCount(&count);
					if (SUCCEEDED(hr))
					{
						for (int i = 0; i < count; i++)
						{
							IMMDevice* pDevice;
							hr = pDevices->Item(i, &pDevice);
							if (SUCCEEDED(hr))
							{
								LPWSTR wstrID = NULL;
								hr = pDevice->GetId(&wstrID);
								if (SUCCEEDED(hr))
								{
									IPropertyStore* pStore;
									hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);
									if (SUCCEEDED(hr))
									{
										PROPVARIANT friendlyName;
										PropVariantInit(&friendlyName);
										hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
										if (SUCCEEDED(hr))
										{
											Local<Object> dev = Object::New(isolate);
											SET(dev, "name", String::NewFromTwoByte(USE_ISOLATE(isolate) (const uint16_t*)friendlyName.pwszVal).ToLocalChecked());
											SET(dev, "id", String::NewFromTwoByte(USE_ISOLATE(isolate) (const uint16_t*)wstrID).ToLocalChecked());
											SET(dev, "default", (wcscmp(wstrDefaultID, wstrID) == 0) ? True(isolate) : False(isolate));
											SETV(list, i, dev);

											// if no options, print the device
											// otherwise, find the selected device and set it to be default
											//if (option == -1) printf("Audio Device %d: %ws\n", i, friendlyName.pwszVal);
											//if (i == option) SetDefaultAudioPlaybackDevice(wstrID);
											PropVariantClear(&friendlyName);
										}
										pStore->Release();
									}
								}
								pDevice->Release();
							}
						}
					}
					pDevices->Release();
				}
				defaultDevice->Release();
				pEnum->Release();
			}
		}
	}

#if 0
    int nSoundCardCount = waveOutGetNumDevs();
    for (int i = 0; i < nSoundCardCount; i++)
    {
        WAVEOUTCAPS woc;
        waveOutGetDevCaps(i, &woc, sizeof(woc));
			Local<String> name = String::NewFromUtf8( isolate, woc.szPname ).ToLocalChecked();
			SETN( list, i, name );
        //cout << woc.szPname << endl; 
    }
#endif
	args.GetReturnValue().Set( list );
#endif
}

static void setDevice(Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void>& args) {
	//HRESULT SetDefaultAudioPlaybackDevice(LPCWSTR devID)
#ifdef WIN32
	{
		Isolate* isolate = args.GetIsolate();
		Local<Context> context = isolate->GetCurrentContext();
		Local<Object> dev = value.As<Object>();
		String::Value id(USE_ISOLATE(isolate) GET( dev, "id" )/*->ToString(isolate->GetCurrentContext()).ToLocalChecked()*/ );
		LPCWSTR devID = (wchar_t const*)*id;
		IPolicyConfigVista* pPolicyConfig;
		ERole reserved = eConsole;

		HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient),
			NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID*)&pPolicyConfig);
		if (SUCCEEDED(hr))
		{
			hr = pPolicyConfig->SetDefaultEndpoint(devID, reserved);
			pPolicyConfig->Release();
			args.GetReturnValue().Set(True(isolate));
			return;
		}
		args.GetReturnValue().Set(False(isolate));
	}
#endif
	//SetDefaultAudioPlaybackDevice();
}

void SoundInit( Isolate *isolate, Local<Object> exports )
{
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> soundInterface = Object::New( isolate );

	//regInterface->Set( String::NewFromUtf8Literal( isolate, "get" ),

	NODE_SET_METHOD( soundInterface, "play", playMedia );
	NODE_SET_METHOD( soundInterface, "load", loadMedia );

	soundInterface->SetNativeDataProperty(context, String::NewFromUtf8Literal(isolate, "devices")
		, listDevices
		, nullptr //Local<Function>()
		, Local<Value>()
		, PropertyAttribute::ReadOnly
		, SideEffectType::kHasSideEffect
		, SideEffectType::kHasSideEffect
	);

	soundInterface->SetNativeDataProperty(context, String::NewFromUtf8Literal(isolate, "default")
		, nullptr
		, setDevice //nullptr //Local<Function>()
		, Local<Value>()
		, PropertyAttribute::None
		, SideEffectType::kHasSideEffect
		, SideEffectType::kHasSideEffect
	);

	//Local<Function> soundFunc = comTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();

	SET( exports, "sound", soundInterface );

}


