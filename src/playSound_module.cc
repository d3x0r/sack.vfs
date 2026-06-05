#define FIX_RELEASE_COM_COLLISION

#include "global.h"
#if defined( SACK_CORE ) && defined( SACK_GUI )
#  include <ffmpeg_interface.h>

//#include <mmsystem.h>

#endif

#ifdef WIN32
#include "win32/PolicyConfig.h"
#include <Functiondiscoverykeys_devpkey.h>
#include <endpointvolume.h>
#include <PropIdl.h>
#include <SetupAPI.h>
#include <cfgmgr32.h>
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

#ifdef WIN32
static bool setDefaultDevice(LPCWSTR devID) {
	//HRESULT SetDefaultAudioPlaybackDevice(LPCWSTR devID)
	{
		IPolicyConfigVista* pPolicyConfig;
		ERole reserved = eConsole;

		HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient),
			NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID*)&pPolicyConfig);
		if (SUCCEEDED(hr))
		{
			hr = pPolicyConfig->SetDefaultEndpoint(devID, reserved);
			pPolicyConfig->Release();
			return true;
		}
	}
	//SetDefaultAudioPlaybackDevice();
	return false;
}
#endif


static void SetDefault(const v8::FunctionCallbackInfo<Value>& args) {
#ifdef WIN32
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	
	Local<Object> dev = args.This().As<Object>();
	String::Value id(USE_ISOLATE(isolate) GET(dev, "id")/*->ToString(isolate->GetCurrentContext()).ToLocalChecked()*/);
	LPCWSTR devID = (wchar_t const*)*id;
	setDefaultDevice( devID );
#endif
}

static void SetDeviceVolume( String::Value *id, double val ) {
#ifdef WIN32

	HRESULT hr;

	IMMDevice* pDevice;
	IMMDeviceEnumerator* pEnum = NULL;
	// Create a multimedia device enumerator.
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
	hr = pEnum->GetDevice((const wchar_t*)**id, &pDevice);
	IAudioEndpointVolume* pVolInfo = NULL;
	hr = pDevice->Activate(__uuidof(IAudioEndpointVolume),
		CLSCTX_ALL, NULL, (void**)&pVolInfo);
	//double val = args[0]->NumberValue(context).FromMaybe(0.0);
	if( !val )
		pVolInfo->SetMute( true, NULL );
	else
		pVolInfo->SetMute( false, NULL );
	hr = pVolInfo->SetMasterVolumeLevelScalar( val, NULL);
	pEnum->Release();
	pDevice->Release();
	pVolInfo->Release();
#endif
}

static double GetDeviceVolume(String::Value* id) {
#ifdef WIN32

	HRESULT hr;

	IMMDevice* pDevice;
	IMMDeviceEnumerator* pEnum = NULL;
	// Create a multimedia device enumerator.
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
	hr = pEnum->GetDevice((const wchar_t*)**id, &pDevice);
	IAudioEndpointVolume* pVolInfo = NULL;
	hr = pDevice->Activate(__uuidof(IAudioEndpointVolume),
		CLSCTX_ALL, NULL, (void**)&pVolInfo);
	//double val = args[0]->NumberValue(context).FromMaybe(0.0);
	float fVolume;
	hr = pVolInfo->GetMasterVolumeLevelScalar(&fVolume);
	pEnum->Release();
	pDevice->Release();
	pVolInfo->Release();
	return fVolume;
#endif
}


static bool GetDevInfoInstanceIdA(
	HDEVINFO devs,
	SP_DEVINFO_DATA* devInfo,
	char* out,
	DWORD outSize
) {
	if (!out || !outSize) return false;
	out[0] = 0;

	return SetupDiGetDeviceInstanceIdA(
		devs,
		devInfo,
		out,
		outSize,
		NULL
	) ? true : false;
}

static bool StartsWithIW(const wchar_t* s, const wchar_t* prefix) {
	if (!s || !prefix)
		return false;

	size_t n = wcslen(prefix);
	return _wcsnicmp(s, prefix, n) == 0;
}

static bool EndpointIdToMMDevApiInstanceIdW(
	const wchar_t* endpointId,
	wchar_t* out,
	DWORD outChars
) {
	if (!endpointId || !endpointId[0] || !out || !outChars)
		return false;

	out[0] = 0;

	// Already a devnode instance id.
	if (StartsWithIW(endpointId, L"SWD\\MMDEVAPI\\")) {
		wcsncpy_s(out, outChars, endpointId, _TRUNCATE);
		return true;
	}

	// Usual IMMDevice::GetId() form:
	// {0.0.0.00000000}.{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
	if (endpointId[0] == L'{') {
		_snwprintf_s(
			out,
			outChars,
			_TRUNCATE,
			L"SWD\\MMDEVAPI\\%s",
			endpointId
		);
		return true;
	}

	// Some APIs expose device-interface-ish paths. This module's IMMDevice::GetId()
	// path usually does not, but leave a failure path explicit.
	return false;
}

static bool GetDeviceIdFromDevInstW(
	DEVINST devInst,
	wchar_t* out,
	ULONG outChars
) {
	if (!out || !outChars)
		return false;

	out[0] = 0;

	CONFIGRET cr = CM_Get_Device_IDW(
		devInst,
		out,
		outChars,
		0
	);

	return cr == CR_SUCCESS;
}

static bool ResolveResetInstanceIdFromEndpointIdW(
	const wchar_t* endpointId,
	wchar_t* endpointInstanceId,
	DWORD endpointInstanceChars,
	wchar_t* resetInstanceId,
	DWORD resetInstanceChars
) {
	if (endpointInstanceId && endpointInstanceChars)
		endpointInstanceId[0] = 0;

	if (resetInstanceId && resetInstanceChars)
		resetInstanceId[0] = 0;

	if (!endpointId || !endpointId[0] || !resetInstanceId || !resetInstanceChars)
		return false;

	wchar_t mmdevapiId[4096];

	if (!EndpointIdToMMDevApiInstanceIdW(
		endpointId,
		mmdevapiId,
		(DWORD)(sizeof(mmdevapiId) / sizeof(mmdevapiId[0]))
	)) {
		return false;
	}

	if (endpointInstanceId && endpointInstanceChars)
		wcsncpy_s(endpointInstanceId, endpointInstanceChars, mmdevapiId, _TRUNCATE);

	DEVINST devInst = 0;
	CONFIGRET cr = CM_Locate_DevNodeW(
		&devInst,
		mmdevapiId,
		CM_LOCATE_DEVNODE_NORMAL
	);

	if (cr != CR_SUCCESS) {
		// Last-resort: return the synthetic SWD instance id.
		// Your reset may or may not work against this node, but it is useful
		// for diagnostics and for devices where the SWD node is restartable.
		wcsncpy_s(resetInstanceId, resetInstanceChars, mmdevapiId, _TRUNCATE);
		return true;
	}

	wchar_t currentId[4096];
	if (GetDeviceIdFromDevInstW(
		devInst,
		currentId,
		(ULONG)(sizeof(currentId) / sizeof(currentId[0]))
	)) {
		wcsncpy_s(resetInstanceId, resetInstanceChars, currentId, _TRUNCATE);
	}

	// Walk upward until we leave SWD\MMDEVAPI.
	// The parent is usually the useful reset target:
	// HDAUDIO\..., USB\..., BTHENUM\..., INTELAUDIO\..., etc.
	DEVINST current = devInst;

	for (;; ) {
		DEVINST parent = 0;
		cr = CM_Get_Parent(&parent, current, 0);
		if (cr != CR_SUCCESS)
			break;

		wchar_t parentId[4096];
		if (!GetDeviceIdFromDevInstW(
			parent,
			parentId,
			(ULONG)(sizeof(parentId) / sizeof(parentId[0]))
		)) {
			break;
		}

		wcsncpy_s(resetInstanceId, resetInstanceChars, parentId, _TRUNCATE);

		if (!StartsWithIW(parentId, L"SWD\\MMDEVAPI\\"))
			break;

		current = parent;
	}

	return resetInstanceId[0] != 0;
}
static bool GetParentInstanceIdA(
	DEVINST child,
	char* out,
	DWORD outSize
) {
	DEVINST parent;
	CONFIGRET cr;

	if (!out || !outSize) return false;
	out[0] = 0;

	cr = CM_Get_Parent(&parent, child, 0);
	if (cr != CR_SUCCESS)
		return false;

	cr = CM_Get_Device_IDA(parent, out, outSize, 0);
	return cr == CR_SUCCESS;
}


static BOOL ResetDeviceByInstanceIdW(const wchar_t* instanceId, BOOL* rebootRequired) {
	if (rebootRequired)
		*rebootRequired = FALSE;

	if (!instanceId || !instanceId[0]) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	HDEVINFO devs = SetupDiGetClassDevsA(
		NULL,
		NULL,
		NULL,
		DIGCF_ALLCLASSES | DIGCF_PRESENT
	);

	if (devs == INVALID_HANDLE_VALUE)
		return FALSE;

	BOOL ok = FALSE;
	DWORD lastError = ERROR_NOT_FOUND;

	SP_DEVINFO_DATA devInfo;
	devInfo.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD index = 0; SetupDiEnumDeviceInfo(devs, index, &devInfo); ++index) {
		wchar_t foundId[4096];

		if (!SetupDiGetDeviceInstanceIdW(
			devs,
			&devInfo,
			foundId,
			sizeof(foundId),
			NULL
		)) {
			continue;
		}

		if (StrCaseCmpW(foundId, instanceId) != 0)
			continue;

		SP_PROPCHANGE_PARAMS params;
		ZeroMemory(&params, sizeof(params));

		params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
		params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
		params.StateChange = DICS_PROPCHANGE;
		params.Scope = DICS_FLAG_GLOBAL;
		params.HwProfile = 0;

		if (!SetupDiSetClassInstallParamsA(
			devs,
			&devInfo,
			&params.ClassInstallHeader,
			sizeof(params)
		)) {
			lastError = GetLastError();
			break;
		}

		if (!SetupDiCallClassInstaller(
			DIF_PROPERTYCHANGE,
			devs,
			&devInfo
		)) {
			lastError = GetLastError();
			break;
		}

		SP_DEVINSTALL_PARAMS_A installParams;
		ZeroMemory(&installParams, sizeof(installParams));
		installParams.cbSize = sizeof(installParams);

		if (SetupDiGetDeviceInstallParamsA(devs, &devInfo, &installParams)) {
			if (installParams.Flags & (DI_NEEDREBOOT | DI_NEEDRESTART)) {
				if (rebootRequired)
					*rebootRequired = TRUE;
			}
		}

		ok = TRUE;
		lastError = ERROR_SUCCESS;
		break;
	}

	SetupDiDestroyDeviceInfoList(devs);
	if (!ok)
		SetLastError(lastError);

	return ok;
}
static BOOL ChangeDeviceEnabledByInstanceIdW(const wchar_t* instanceId, BOOL enable, BOOL* rebootRequired) {
	if (rebootRequired)
		*rebootRequired = FALSE;

	if (!instanceId || !instanceId[0]) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	HDEVINFO devs = SetupDiGetClassDevsA(
		NULL,
		NULL,
		NULL,
		DIGCF_ALLCLASSES | DIGCF_PRESENT
	);

	if (devs == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	BOOL ok = FALSE;
	DWORD lastError = ERROR_NOT_FOUND;

	SP_DEVINFO_DATA devInfo;
	devInfo.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD index = 0; SetupDiEnumDeviceInfo(devs, index, &devInfo); ++index) {
		wchar_t foundId[4096];
		if (!SetupDiGetDeviceInstanceIdW(
			devs,
			&devInfo,
			foundId,
			sizeof(foundId),
			NULL
		)) {
			continue;
		}
//		lprintf("Setup DI info? %S %S", foundId, instanceId);

		if (StrCaseCmpW(foundId, instanceId) != 0)
			continue;

		SP_PROPCHANGE_PARAMS params;
		ZeroMemory(&params, sizeof(params));

		params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
		params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
		params.StateChange = enable ? DICS_ENABLE : DICS_DISABLE;
		params.Scope = DICS_FLAG_GLOBAL;
		params.HwProfile = 0;

		if (!SetupDiSetClassInstallParamsA(
			devs,
			&devInfo,
			&params.ClassInstallHeader,
			sizeof(params)
		)) {
			lastError = GetLastError();
			break;
		}

		if (!SetupDiCallClassInstaller(
			DIF_PROPERTYCHANGE,
			devs,
			&devInfo
		)) {
			lastError = GetLastError();
			break;
		}

		SP_DEVINSTALL_PARAMS_A installParams;
		ZeroMemory(&installParams, sizeof(installParams));
		installParams.cbSize = sizeof(installParams);

		if (SetupDiGetDeviceInstallParamsA(devs, &devInfo, &installParams)) {
			if (installParams.Flags & (DI_NEEDREBOOT | DI_NEEDRESTART)) {
				if (rebootRequired)
					*rebootRequired = TRUE;
			}
		}

		ok = TRUE;
		lastError = ERROR_SUCCESS;
		break;
	}
	SetupDiDestroyDeviceInfoList(devs);
	if (!ok)
		SetLastError(lastError);

	return ok;
}

static BOOL ResetDeviceByInstanceIdWithFallbackW(const wchar_t* instanceId, BOOL* rebootRequired) {
	BOOL needReboot1 = FALSE;
	BOOL needReboot2 = FALSE;
	BOOL needReboot3 = FALSE;

	if (ResetDeviceByInstanceIdW(instanceId, &needReboot1)) {
		if (rebootRequired)
			*rebootRequired = needReboot1;
		return TRUE;
	}

	DWORD resetError;

	if (!ChangeDeviceEnabledByInstanceIdW(instanceId, FALSE, &needReboot2)) {
		resetError = GetLastError();
		SetLastError(resetError);
		return FALSE;
	}
	Sleep(250);

	if (!ChangeDeviceEnabledByInstanceIdW(instanceId, TRUE, &needReboot3)) {
		resetError = GetLastError();
		return FALSE;
	}

	if (rebootRequired)
		*rebootRequired = needReboot2 || needReboot3;

	return TRUE;
}

static void resetSoundDevice( const v8::FunctionCallbackInfo<Value>& args ) {
#ifdef WIN32
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	Local<Object> dev = args.This().As<Object>();
	String::Value id(USE_ISOLATE(isolate) GET(dev, "resetId")/*->ToString(isolate->GetCurrentContext()).ToLocalChecked()*/);
	//String::Utf8Value idUtf8( id );
	const wchar_t* instanceId = (const wchar_t*)*id;
	BOOL fallback = TRUE;
	if( args.Length() > 1 )
		fallback = args[1]->TOBOOL( isolate ) ? TRUE : FALSE;

	BOOL rebootRequired = FALSE;
	BOOL ok = fallback
		? ResetDeviceByInstanceIdWithFallbackW( instanceId, &rebootRequired )
		: ResetDeviceByInstanceIdW( instanceId, &rebootRequired );

	DWORD err = ok ? ERROR_SUCCESS : GetLastError();

	Local<Object> result = Object::New( isolate );

	SET( result, "ok", ok ? True( isolate ) : False( isolate ) );
	SET( result, "rebootRequired", rebootRequired ? True( isolate ) : False( isolate ) );
	SET( result, "error", Integer::New( isolate, (int)err ) );
	args.GetReturnValue().Set( result );
#endif
}

static void setCurrentVolume(Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void>& args) {
#ifdef WIN32
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> dev = args.This().As<Object>();
	String::Value id(USE_ISOLATE(isolate) GET(dev, "id")/*->ToString(isolate->GetCurrentContext()).ToLocalChecked()*/);

	double val = value->NumberValue(context).FromMaybe(0.0);
	SetDeviceVolume(&id, val);
#endif
}

static void getCurrentVolume(Local<Name> property, const PropertyCallbackInfo<Value>& args) {
#ifdef WIN32
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> dev = args.This().As<Object>();
	String::Value id(USE_ISOLATE(isolate) GET(dev, "id")/*->ToString(isolate->GetCurrentContext()).ToLocalChecked()*/);

	args.GetReturnValue().Set( Number::New( isolate, GetDeviceVolume(&id) ) );
#endif
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
			LPWSTR wstrDefaultComID = NULL;
			LPWSTR wstrDefaultConID = NULL;
			IMMDevice* defaultDevice;
			IMMDevice* defaultComDevice;
			IMMDevice* defaultConDevice;
			hr = pEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &defaultDevice);
			hr = defaultDevice->GetId(&wstrDefaultID);
			hr = pEnum->GetDefaultAudioEndpoint(eRender, eCommunications, &defaultComDevice);
			hr = defaultComDevice->GetId(&wstrDefaultComID);
			hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &defaultConDevice);
			hr = defaultConDevice->GetId(&wstrDefaultConID);
			hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
			if (SUCCEEDED(hr)) {

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
										Local<Object> dev = Object::New(isolate);
										if (SUCCEEDED(hr))
										{
											IAudioEndpointVolume* pVolInfo = NULL;
											//IAudioMeterInformation* pMeterInfo = NULL;
											float fVolume;
											hr = pDevice->Activate(__uuidof(IAudioEndpointVolume),
												CLSCTX_ALL, NULL, (void**)&pVolInfo);
											//hr = pDevice->Activate(__uuidof(IAudioMeterInformation),
											//	CLSCTX_ALL, NULL, (void**)&pMeterInfo);
											pVolInfo->GetMasterVolumeLevelScalar(&fVolume);

											SET(dev, "name", String::NewFromTwoByte(USE_ISOLATE(isolate) (const uint16_t*)friendlyName.pwszVal).ToLocalChecked());
											SET(dev, "id", String::NewFromTwoByte(USE_ISOLATE(isolate) (const uint16_t*)wstrID).ToLocalChecked());

											wchar_t endpointInstanceId[4096];
											wchar_t resetInstanceId[4096];

											if (ResolveResetInstanceIdFromEndpointIdW(
												wstrID,
												endpointInstanceId,
												(DWORD)(sizeof(endpointInstanceId) / sizeof(endpointInstanceId[0])),
												resetInstanceId,
												(DWORD)(sizeof(resetInstanceId) / sizeof(resetInstanceId[0]))
											)) {
												SET(
													dev,
													"pnpId",
													String::NewFromTwoByte(
														USE_ISOLATE(isolate)
														(const uint16_t*)endpointInstanceId
													).ToLocalChecked()
												);

												SET(
													dev,
													"resetId",
													String::NewFromTwoByte(
														USE_ISOLATE(isolate)
														(const uint16_t*)resetInstanceId
													).ToLocalChecked()
												);
											}

											dev->SetNativeDataProperty(context, String::NewFromUtf8Literal(isolate, "volume")
												, getCurrentVolume
												, setCurrentVolume
												, Local<Value>()
												, PropertyAttribute::None
												, SideEffectType::kHasSideEffect
												, SideEffectType::kHasSideEffect
											);
											SET(dev, "default", (wcscmp(wstrDefaultID, wstrID) == 0) ? True(isolate) : False(isolate));
											SET(dev, "communications", (wcscmp(wstrDefaultComID, wstrID) == 0) ? True(isolate) : False(isolate));
											SET(dev, "console", (wcscmp(wstrDefaultConID, wstrID) == 0) ? True(isolate) : False(isolate));
											NODE_SET_METHOD(dev, "setDefault", SetDefault);
											SET_READONLY_METHOD( dev, "reset", resetSoundDevice );
											SETV(list, i, dev);

											// if no options, print the device
											// otherwise, find the selected device and set it to be default
											//if (option == -1) printf("Audio Device %d: %ws\n", i, friendlyName.pwszVal);
											//if (i == option) SetDefaultAudioPlaybackDevice(wstrID);
											PropVariantClear(&friendlyName);
										}
#if 0
										PROPVARIANT pv;
											
										PropVariantClear(&pv);
										if( SUCCEEDED(pStore->GetValue( PKEY_Device_InstanceId, &pv ) ) ) {
											if( pv.vt == VT_LPWSTR && pv.pwszVal ) {
												lprintf("Got PHP");
												// This is the value to expose as pnpId if present.
												SET(dev, "pnpId", String::NewFromTwoByte(USE_ISOLATE(isolate) (const uint16_t*)pv.pwszVal).ToLocalChecked());
											}
											else lprintf("no WStr? %d", pv.vt);
											if (pv.vt == VT_LPSTR && pv.pszVal) {
												lprintf("Got PHPa");
												// This is the value to expose as pnpId if present.
												SET(dev, "pnpId", String::NewFromUtf8(USE_ISOLATE(isolate) pv.pszVal).ToLocalChecked());
											}
											else lprintf("no ASTR? %d", pv.vt);
											PropVariantClear( &pv );
										}
										else lprintf("FAILED");
#endif
										pStore->Release();
									}
								}
								CoTaskMemFree(wstrID);
								pDevice->Release();
							}
						}
					}
				}
			}
			CoTaskMemFree( wstrDefaultID );
			CoTaskMemFree(wstrDefaultComID );
			CoTaskMemFree(wstrDefaultConID );

			defaultDevice->Release();
			defaultComDevice->Release();
			defaultConDevice->Release();
			pEnum->Release();
			pDevices->Release();
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

	//Local<Function> soundFunc = comTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();

	SET( exports, "sound", soundInterface );

}


