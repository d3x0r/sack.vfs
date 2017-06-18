

#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>
//#include <nan.h>

#include <stdhdrs.h>
#include <sack_vfs.h>

#undef New

using namespace v8;

class VolumeObject : node::ObjectWrap {
public:
	struct volume *vol;
	static v8::Persistent<v8::Function> constructor;
public:

	static void Init( Handle<Object> exports );
	VolumeObject( const char *filename, const char *key, const char *key2 );

	static void openFile( const FunctionCallbackInfo<Value>& args );

	static void New( const FunctionCallbackInfo<Value>& args );

};


class FileObject : public node::ObjectWrap {
	struct sack_vfs_file *file;
	static v8::Persistent<v8::Function> constructor;
public:

	static void readFile( const FunctionCallbackInfo<Value>& args );

	FileObject( VolumeObject* vol, const char *filename );


	void DoWrap( Local<Object> This );


};




void VolumeObject::Init( Handle<Object> exports ) {
		Isolate* isolate = Isolate::GetCurrent();

		// Prepare constructor template
		Local<FunctionTemplate> tpl = FunctionTemplate::New( isolate, New );
		tpl->SetClassName( String::NewFromUtf8( isolate, "VolumeObject" ) );
		tpl->InstanceTemplate()->SetInternalFieldCount( 1 );

		// Prototype
		NODE_SET_PROTOTYPE_METHOD( tpl, "openFile", openFile );

		constructor.Reset( isolate, tpl->GetFunction() );
		exports->Set( String::NewFromUtf8( isolate, "VolumeObject" ),
			tpl->GetFunction() );
		
	}

VolumeObject::VolumeObject( const char *filename, const char *key, const char *key2 )  {

	vol = sack_vfs_load_crypt_volume( filename, key, key2 );

}

	void VolumeObject::openFile( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		if( args.Length() < 1 )
			isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( isolate, "Requires filename to open" ) ) );
		if( args.IsConstructCall() ) {
			char *filename;
			String::Utf8Value fName( args[0] );
			filename = *fName;
			VolumeObject* vol = ObjectWrap::Unwrap<VolumeObject>( args.Holder() );
			// Invoked as constructor: `new MyObject(...)`
			FileObject* obj = new FileObject( vol, filename );
			obj->DoWrap( args.This() );
			args.GetReturnValue().Set( args.This() );
		}
		else {
			// Invoked as plain function `MyObject(...)`, turn into construct call.
			const int argc = 1;
			Local<Value> argv[argc] = { args[0] };
			Local<Function> cons = Local<Function>::New( isolate, constructor );
			args.GetReturnValue().Set( cons->NewInstance( argc, argv ) );
		}
	}

	void VolumeObject::New( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		if( args.IsConstructCall() ) {
			char *filename = "default.vfs";
			char *key = NULL;
			char *key2 = NULL;
			int argc = args.Length();
			if( argc > 0 ) {
				String::Utf8Value fName( args[0] );
				filename = *fName;
			}
			if( argc > 1 ) {
				String::Utf8Value k( args[1] );
				key = *k;
			}
			if( argc > 2 ) {
				String::Utf8Value k( args[2] );
				key2 = *k;
			}
			// Invoked as constructor: `new MyObject(...)`
			VolumeObject* obj = new VolumeObject( filename, key, key2 );
			obj->Wrap( args.This() );
			args.GetReturnValue().Set( args.This() );
		}
		else {
			// Invoked as plain function `MyObject(...)`, turn into construct call.
			const int argc = 1;
			Local<Value> argv[argc] = { args[0] };
			Local<Function> cons = Local<Function>::New( isolate, constructor );
			args.GetReturnValue().Set( cons->NewInstance( argc, argv ) );
		}
	}



	void FileObject::readFile( const FunctionCallbackInfo<Value>& args ) {
	}

	FileObject::FileObject( VolumeObject* vol, const char *filename ) {
		file = sack_vfs_openfile( vol->vol, filename );
	}


	void FileObject::DoWrap( Local<Object> This ) {
		Wrap( This );
	}

Persistent<Function> VolumeObject::constructor;
Persistent<Function> FileObject::constructor;

NODE_MODULE( vfs_module, VolumeObject::Init)

