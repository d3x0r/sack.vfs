// OpenXR session — D3D12 graphics binding over the app's existing Dawn device.
//
// Registers the XRSession class. sack.xr.beginSession(device) lives here;
// InitOpenXR in xr_module.cc calls InitXRSession to wire it up.

#pragma once

#include "../../global.h"

extern void InitXRSession( v8::Isolate* isolate, v8::Local<v8::Object> exports );

// sack.xr.beginSession( GPUDevice ) -> XRSession
extern void XR_beginSession( const v8::FunctionCallbackInfo<v8::Value>& args );
