// OpenXR session — stand-in for platforms without a graphics binding.
//
// Compiled instead of xr_session.cc wherever XR_BINDING_UNSUPPORTED is defined
// (see CMakeLists.openxr.txt). It supplies the two symbols xr_module.cc owes
// its linker and nothing else.
//
// Why a stub rather than simply not building the module:
//
//   sack.xr is wired up from vfs_module.cc behind #ifdef INCLUDE_OPENXR. Drop
//   that and `sack.xr` is undefined, so the first thing a caller sees is
//   "Cannot read properties of undefined (reading 'beginSession')" — which
//   describes the symptom and hides the cause. An object that exists and
//   explains itself is worth the dozen lines.
//
// probe() is NOT stubbed. xr_module.cc compares extension names as strings
// rather than including openxr_platform.h, so it is platform-neutral: on Linux
// it still reports the runtime name, version, system and extension list. What
// it will report is a runtime offering XR_KHR_vulkan_enable rather than
// XR_KHR_D3D12_enable — which is exactly the information someone needs before
// writing the binding this file is standing in for.
//
// Implementing that binding means XrGraphicsBindingVulkan2KHR over Dawn's
// Vulkan backend. It is viable on Linux for the reason it is not on Windows:
// Dawn's vulkan/external_memory/ implements OpaqueFD, while the Win32 path
// falls through to a `void*` stub. See the notes in xr_session.cc.

#include "xr_session.h"

#include <string>

// The message is shared so the log line and the exception cannot drift apart.
static const char* const kXRUnsupported =
	"OpenXR is not available in this build: the session binding requires the "
	"D3D12 graphics binding, which exists only on Windows. sack.xr.probe() "
	"still reports the runtime on this platform.";

// ---- InitXRSession ---------------------------------------------------
//
// Registers nothing. Logged once at module init so the limitation shows up in
// the log of anyone wondering why their headset is idle, rather than only at
// the moment of failure.

void InitXRSession( Isolate* isolate, Local<Object> exports ) {
	(void)isolate;
	(void)exports;
	lprintf( "%s", kXRUnsupported );
}

// ---- beginSession ----------------------------------------------------
//
// Throws rather than returning null: a caller asked for a session, and there is
// no version of "carry on" that works. The message names the cause and the
// platform, because the common way to arrive here is running a test written on
// Windows.

void XR_beginSession( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	isolate->ThrowException( Exception::Error(
		localStringExternal( isolate, kXRUnsupported ) ) );
}
