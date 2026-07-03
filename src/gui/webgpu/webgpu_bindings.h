// WebGPU bindings — shared helpers used by both hand-written and
// generated code. Include from webgpu_module.cc and any per-class file.
//
// Conventions:
//   - Each wrapped GPU object is a node::ObjectWrap class with a `handle_`
//     field holding the corresponding WGPU* C handle.
//   - Each class has a constructor template + Persistent<Function> in the
//     per-isolate constructorSet (see global.h).
//   - JS methods declared with WGPU_METHOD(name); body uses WGPU_THIS(T) to
//     unwrap `this`, then standard sack-gui macros (GETV, GET_NUMBER, ...).

#pragma once

#include "../../global.h"

#include "webgpu/webgpu.h"  // Dawn's C header

// ---------- method declaration / prologue ----------

// Declares the V8 callback static method.
#define WGPU_METHOD( name ) \
	static void name( const v8::FunctionCallbackInfo<v8::Value>& args )

// Body prologue: unwrap `this` to T*, set up isolate/context locals.
#define WGPU_THIS( T )                                                  \
	v8::Isolate* isolate = args.GetIsolate();                       \
	v8::Local<v8::Context> context = isolate->GetCurrentContext();  \
	T* self = node::ObjectWrap::Unwrap<T>( getFCIHolder( args ) );  \
	(void)context; (void)self

// ---------- result wrapping ----------

// Build a new JS wrapper around a freshly-produced WGPU handle.
// WrapperT must (a) have a `<WrapperT>_constructor` slot in constructorSet,
// (b) expose a public `handle_` field of the matching WGPU* type.
//
// The C++ wrapper is created via NewInstance() with no args; the wrapper's
// JS-side `New` callback should accept zero-arg construction as a no-op and
// rely on the C++ caller to fill `handle_` immediately after.
#define WGPU_RETURN_NEW( WrapperT, handle )                                          \
	do {                                                                         \
		v8::Local<v8::Function> _ctor = getConstructors( isolate )           \
			->WrapperT##_constructor.Get( isolate );                     \
		v8::Local<v8::Object> _obj = _ctor->NewInstance( context, 0, NULL )  \
			.ToLocalChecked();                                           \
		WrapperT* _w = node::ObjectWrap::Unwrap<WrapperT>( _obj );           \
		_w->handle_ = (handle);                                              \
		args.GetReturnValue().Set( _obj );                                   \
	} while( 0 )

// Same as above, but resolves a Promise resolver with the new wrapper instead
// of returning it. Used by async callbacks dispatching back into V8.
#define WGPU_RESOLVE_NEW( WrapperT, handle, resolver )                                \
	do {                                                                          \
		v8::Local<v8::Function> _ctor = getConstructors( isolate )            \
			->WrapperT##_constructor.Get( isolate );                      \
		v8::Local<v8::Object> _obj = _ctor->NewInstance( context, 0, NULL )   \
			.ToLocalChecked();                                            \
		WrapperT* _w = node::ObjectWrap::Unwrap<WrapperT>( _obj );            \
		_w->handle_ = (handle);                                               \
		(resolver)->Resolve( context, _obj ).Check();                         \
	} while( 0 )

// ---------- small inline utilities ----------

// WGPUStringView (data, length) ↔ V8 String.
static inline v8::Local<v8::String> WGPU_StringViewToV8(
	v8::Isolate* isolate, WGPUStringView sv )
{
	if( !sv.data || sv.length == 0 )
		return v8::String::Empty( isolate );
	int len = sv.length == SIZE_MAX
		? (int)strlen( sv.data )
		: (int)sv.length;
	return v8::String::NewFromUtf8( isolate, sv.data,
		v8::NewStringType::kNormal, len ).ToLocalChecked();
}

// V8 String ↔ WGPUStringView. The returned view is valid only as long as
// the v8::String::Utf8Value lives in the caller's scope.
struct WGPU_StringViewHolder {
	v8::String::Utf8Value utf8;
	WGPU_StringViewHolder( v8::Isolate* iso, v8::Local<v8::Value> v )
		: utf8( iso, v ) {}
	WGPUStringView view() const {
		WGPUStringView sv;
		sv.data = *utf8;
		sv.length = (size_t)utf8.length();
		return sv;
	}
};
