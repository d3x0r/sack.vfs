// OpenXR module — implementation.
//
// First slice: prove loader linkage, runtime registration, and extension
// availability, with no Dawn involvement at all. If probe() reports a runtime
// name and lists XR_KHR_D3D12_enable, the D3D12 interop plan is viable on this
// machine; everything after this point can assume it.

#include "xr_module.h"
#include "xr_session.h"

#include <openxr/openxr.h>

#include <string>
#include <vector>

// XR_KHR_D3D12_ENABLE_EXTENSION_NAME is declared in openxr_platform.h behind
// XR_USE_GRAPHICS_API_D3D12, which would drag d3d12.h and dxgi into this TU for
// no benefit — a probe only ever compares these as strings. The session code
// that actually builds an XrGraphicsBindingD3D12KHR will include the platform
// header properly, and these names are frozen by the spec.
static const char* const kExtD3D12 = "XR_KHR_D3D12_enable";
static const char* const kExtD3D11 = "XR_KHR_D3D11_enable";

// xrResultToString needs an instance, which we don't have for failures during
// instance creation itself. Those are exactly the failures worth naming — a bare
// "-51" tells you nothing, while XR_ERROR_RUNTIME_UNAVAILABLE points straight at
// runtime registration — so map the handful reachable before an instance exists.
static std::string xr_result_text( XrInstance instance, XrResult res ) {
	if( instance != XR_NULL_HANDLE ) {
		char buf[ XR_MAX_RESULT_STRING_SIZE ];
		if( XR_SUCCEEDED( xrResultToString( instance, res, buf ) ) )
			return std::string( buf );
	}
	switch( res ) {
	case XR_ERROR_RUNTIME_UNAVAILABLE:
		return "XR_ERROR_RUNTIME_UNAVAILABLE (-51)";
	case XR_ERROR_RUNTIME_FAILURE:
		// Seen when the runtime is registered but not actually up — e.g.
		// SteamVR has exited since it was last used. Start it and retry.
		return "XR_ERROR_RUNTIME_FAILURE (-2)";
	case XR_ERROR_FILE_ACCESS_ERROR:
		return "XR_ERROR_FILE_ACCESS_ERROR (-32)";
	case XR_ERROR_API_VERSION_UNSUPPORTED:
		return "XR_ERROR_API_VERSION_UNSUPPORTED (-4)";
	case XR_ERROR_EXTENSION_NOT_PRESENT:
		return "XR_ERROR_EXTENSION_NOT_PRESENT (-7)";
	case XR_ERROR_INITIALIZATION_FAILED:
		return "XR_ERROR_INITIALIZATION_FAILED (-6)";
	case XR_ERROR_OUT_OF_MEMORY:
		return "XR_ERROR_OUT_OF_MEMORY (-3)";
	default:
		return std::to_string( (int)res );
	}
}

static std::string xr_version_text( XrVersion v ) {
	return std::to_string( (unsigned long long)XR_VERSION_MAJOR( v ) ) + "."
	     + std::to_string( (unsigned long long)XR_VERSION_MINOR( v ) ) + "."
	     + std::to_string( (unsigned long long)XR_VERSION_PATCH( v ) );
}

static Local<String> xr_str( Isolate* isolate, const std::string& s ) {
	return String::NewFromUtf8( isolate, s.c_str(), v8::NewStringType::kNormal,
	                            (int)s.length() ).ToLocalChecked();
}

// Builds { ok:false, stage, error } — a probe reports rather than throws,
// because most of its interesting outcomes (no runtime installed, headset
// unplugged) are normal states, not exceptions.
static Local<Object> xr_failure( Isolate* isolate,
                                 const char* stage, const std::string& error ) {
	Local<Object> out = Object::New( isolate );
	SET_READONLY( out, "ok", False( isolate ) );
	SET_READONLY( out, "stage", localStringExternal( isolate, stage ) );
	SET_READONLY( out, "error", xr_str( isolate, error ) );
	return out;
}

static void XR_probe( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	// ---- 1. Extensions, before any instance exists ----
	uint32_t extCount = 0;
	XrResult res = xrEnumerateInstanceExtensionProperties( NULL, 0, &extCount, NULL );
	if( XR_FAILED( res ) ) {
		// Almost always "no runtime registered" — the loader found nothing to
		// dispatch to. Check ActiveRuntime / XR_RUNTIME_JSON.
		args.GetReturnValue().Set( xr_failure( isolate,
			"xrEnumerateInstanceExtensionProperties",
			xr_result_text( XR_NULL_HANDLE, res ) ) );
		return;
	}

	std::vector<XrExtensionProperties> exts( extCount );
	for( uint32_t i = 0; i < extCount; i++ ) {
		exts[ i ].type = XR_TYPE_EXTENSION_PROPERTIES;
		exts[ i ].next = NULL;
	}
	res = xrEnumerateInstanceExtensionProperties( NULL, extCount, &extCount,
	                                              extCount ? exts.data() : NULL );
	if( XR_FAILED( res ) ) {
		args.GetReturnValue().Set( xr_failure( isolate,
			"xrEnumerateInstanceExtensionProperties(fill)",
			xr_result_text( XR_NULL_HANDLE, res ) ) );
		return;
	}

	Local<Array> extArray = Array::New( isolate, (int)extCount );
	bool haveD3D12 = false;
	bool haveD3D11 = false;
	for( uint32_t i = 0; i < extCount; i++ ) {
		const char* name = exts[ i ].extensionName;
		if( !strcmp( name, kExtD3D12 ) ) haveD3D12 = true;
		if( !strcmp( name, kExtD3D11 ) ) haveD3D11 = true;
		extArray->Set( context, i, xr_str( isolate, name ) ).FromMaybe( false );
	}

	// ---- 2. Instance ----
	// Enable the D3D12 binding when offered so this probe exercises the same
	// path session creation will take. Requesting an unsupported extension is a
	// hard failure, so it stays conditional.
	std::vector<const char*> enabled;
	if( haveD3D12 ) enabled.push_back( kExtD3D12 );

	XrInstanceCreateInfo ici = {};
	ici.type = XR_TYPE_INSTANCE_CREATE_INFO;
	// Ask for 1.0 rather than XR_CURRENT_API_VERSION (1.1.x for these headers):
	// a 1.0-only runtime rejects a 1.1 request outright, and nothing here needs
	// 1.1 semantics.
	ici.applicationInfo.apiVersion = XR_API_VERSION_1_0;
	snprintf( ici.applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE,
	          "sack-gui" );
	ici.applicationInfo.applicationVersion = 1;
	snprintf( ici.applicationInfo.engineName, XR_MAX_ENGINE_NAME_SIZE, "sack" );
	ici.applicationInfo.engineVersion = 1;
	ici.enabledExtensionCount = (uint32_t)enabled.size();
	ici.enabledExtensionNames = enabled.empty() ? NULL : enabled.data();

	XrInstance instance = XR_NULL_HANDLE;
	res = xrCreateInstance( &ici, &instance );
	if( XR_FAILED( res ) ) {
		args.GetReturnValue().Set( xr_failure( isolate, "xrCreateInstance",
			xr_result_text( XR_NULL_HANDLE, res ) ) );
		return;
	}

	Local<Object> out = Object::New( isolate );
	SET_READONLY( out, "ok", True( isolate ) );
	SET_READONLY( out, "d3d12", Boolean::New( isolate, haveD3D12 ) );
	SET_READONLY( out, "d3d11", Boolean::New( isolate, haveD3D11 ) );
	SET_READONLY( out, "extensions", extArray );

	// ---- 3. Runtime identity ----
	XrInstanceProperties iprops = {};
	iprops.type = XR_TYPE_INSTANCE_PROPERTIES;
	if( XR_SUCCEEDED( xrGetInstanceProperties( instance, &iprops ) ) ) {
		SET_READONLY( out, "runtimeName", xr_str( isolate, iprops.runtimeName ) );
		SET_READONLY( out, "runtimeVersion",
			xr_str( isolate, xr_version_text( iprops.runtimeVersion ) ) );
	}

	// ---- 4. System (headset) ----
	// A missing system is not a probe failure: XR_ERROR_FORM_FACTOR_UNAVAILABLE
	// just means nothing is plugged in / SteamVR isn't running. Report it and
	// keep the runtime facts we already gathered.
	XrSystemGetInfo sgi = {};
	sgi.type = XR_TYPE_SYSTEM_GET_INFO;
	sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

	XrSystemId systemId = XR_NULL_SYSTEM_ID;
	res = xrGetSystem( instance, &sgi, &systemId );
	if( XR_FAILED( res ) ) {
		SET_READONLY( out, "system", Null( isolate ) );
		SET_READONLY( out, "systemError",
			xr_str( isolate, xr_result_text( instance, res ) ) );
	} else {
		XrSystemProperties sprops = {};
		sprops.type = XR_TYPE_SYSTEM_PROPERTIES;
		if( XR_SUCCEEDED( xrGetSystemProperties( instance, systemId, &sprops ) ) ) {
			Local<Object> sys = Object::New( isolate );
			SET_READONLY( sys, "name", xr_str( isolate, sprops.systemName ) );
			SET_READONLY( sys, "vendorId",
				Number::New( isolate, (double)sprops.vendorId ) );
			SET_READONLY( sys, "maxSwapchainWidth",
				Number::New( isolate,
					(double)sprops.graphicsProperties.maxSwapchainImageWidth ) );
			SET_READONLY( sys, "maxSwapchainHeight",
				Number::New( isolate,
					(double)sprops.graphicsProperties.maxSwapchainImageHeight ) );
			SET_READONLY( sys, "maxLayerCount",
				Number::New( isolate,
					(double)sprops.graphicsProperties.maxLayerCount ) );
			SET_READONLY( sys, "orientationTracking",
				Boolean::New( isolate,
					sprops.trackingProperties.orientationTracking != XR_FALSE ) );
			SET_READONLY( sys, "positionTracking",
				Boolean::New( isolate,
					sprops.trackingProperties.positionTracking != XR_FALSE ) );
			SET_READONLY( out, "system", sys );
		}
	}

	// The probe owns nothing beyond this call; a real session keeps the instance.
	xrDestroyInstance( instance );

	args.GetReturnValue().Set( out );
}

void InitOpenXR( Isolate* isolate, Local<Object> exports ) {
	// SET_READONLY / SET_READONLY_METHOD reach for isolate->GetCurrentContext()
	// themselves, so no local context is needed here.
	InitXRSession( isolate, exports );

	Local<Object> xr = Object::New( isolate );
	SET_READONLY_METHOD( xr, "probe", XR_probe );
	SET_READONLY_METHOD( xr, "beginSession", XR_beginSession );
	SET_READONLY( exports, "xr", xr );
}
