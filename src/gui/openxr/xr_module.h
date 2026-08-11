// OpenXR module — entry point.
//
// Call InitOpenXR(isolate, exports) from the main module dispatcher to expose
// `sack.xr`. Currently a single `probe()` call that reports what the active
// OpenXR runtime is and whether it offers the graphics bindings we need.
//
// Runtime selection is not ours to make: the loader dispatches to whatever is
// registered at HKLM\SOFTWARE\Khronos\OpenXR\1\ActiveRuntime, or to whatever
// the XR_RUNTIME_JSON environment variable points at. probe() reporting an
// unexpected runtimeName means one of those two is pointing somewhere else.

#pragma once

#include "../../global.h"

extern void InitOpenXR( v8::Isolate* isolate, v8::Local<v8::Object> exports );
