
#include "global.h"

#ifdef _WIN32
// device something?
#include <dbt.h>
#include <cfgmgr32.h> 
#include <initguid.h>
#include <devpkey.h>
#include <setupapi.h>
#endif

enum msgbuf_op {
	MSG_OP_DATA = 0,
	MSG_OP_CLOSE,
	MSG_OP_REMOVE,
	MSG_OP_ADDED,
};

struct msgbuf {
	int op;
	size_t buflen;
	uint8_t buf[1];
};

static void asyncmsg__( Isolate *isolate, Local<Context> context, class ComObject * myself );
struct comAsyncTask : SackTask {
	class ComObject *myself;
	comAsyncTask( ComObject *myself )
	    : myself( myself ) {}
	void Run2( Isolate *isolate, Local<Context> context ) {
		asyncmsg__( isolate, context, this->myself );
	}
};


class ComObject : public node::ObjectWrap {
public:
	bool ivm_hosted = false;
	static bool _ivm_hosted;
	class constructorSet *c;
	static class constructorSet* _c;
	int handle;
	char* name;
	//static Persistent<Function> constructor;
	bool rts = 1;
	Persistent<Function>* readCallback; //
	static Persistent<Function> removeCallback; //
	static Persistent<Function> addCallback; //
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	static uv_async_t _async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE readQueue;
	static PLINKQUEUE _readQueue;

public:

	static void Init( Local<Object> exports );
	ComObject( char* name );
	Persistent<Object> jsObject;
	static void onRemove(Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void>& info);
	static void onAdd(Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void>& info);

private:
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void getPorts( Local<Name> property, const PropertyCallbackInfo<Value>& info );
	static void getRTS2( Local<Name> property, const PropertyCallbackInfo<Value>& args );
	static void getRTS( const v8::FunctionCallbackInfo<Value>& args );
	static void setRTS2( Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void>& args );
	static void setRTS( const v8::FunctionCallbackInfo<Value>& args );
	static void onRead( const v8::FunctionCallbackInfo<Value>& args );
	static void writeCom( const v8::FunctionCallbackInfo<Value>& args );
	static void closeCom( const v8::FunctionCallbackInfo<Value>& args );
	~ComObject();
};

Persistent<Function> ComObject::removeCallback; //
Persistent<Function> ComObject::addCallback; //
uv_async_t ComObject::_async; // keep this instance around for as long as we might need to do the periodic callback
class constructorSet* ComObject::_c;
bool ComObject::_ivm_hosted = false;
PLINKQUEUE ComObject::_readQueue;


ComObject::ComObject( char *name ) : jsObject() {
	this->readQueue = CreateLinkQueue();
	this->name = name;
	handle = SackOpenComm( name, 0, 0 );
	uintptr_t CPROC RegisterAndCreateMonitor(PTHREAD thread);
	ThreadTo( RegisterAndCreateMonitor, 0 );
}

ComObject::~ComObject() {
	if( handle >=0 )
		SackCloseComm( handle );
  	Deallocate( char*, name );
}

void ComObjectInit( Local<Object> exports ) {
	ComObject::Init( exports );
}

void test( Local<Name> property,
	const PropertyCallbackInfo<Value>& info ) {
}


void ComObject::Init( Local<Object> exports ) {
		Isolate* isolate = Isolate::GetCurrent();
		Local<Context> context = isolate->GetCurrentContext();
		Local<FunctionTemplate> comTemplate;
		void reEnablePort(char const* port);
			reEnablePort("");
		comTemplate = FunctionTemplate::New( isolate, New );
		comTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.ComPort" ) );
		comTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

		// Prototype
		NODE_SET_PROTOTYPE_METHOD( comTemplate, "onRead", onRead );
		NODE_SET_PROTOTYPE_METHOD( comTemplate, "write", writeCom );
		NODE_SET_PROTOTYPE_METHOD( comTemplate, "close", closeCom );

#if ( NODE_MAJOR_VERSION >= 22 )
		comTemplate->PrototypeTemplate()->SetNativeDataProperty( String::NewFromUtf8Literal( isolate, "rts" )
			, ComObject::getRTS2
			, ComObject::setRTS2
			, Local<Value>()
			, PropertyAttribute::ReadOnly
			, SideEffectType::kHasNoSideEffect
			, SideEffectType::kHasSideEffect
		);
#elif ( NODE_MAJOR_VERSION >= 18 )
		comTemplate->PrototypeTemplate()->SetNativeDataProperty( String::NewFromUtf8Literal( isolate, "rts" )
			, ComObject::getRTS2
			, ComObject::setRTS2
			, Local<Value>()
			, PropertyAttribute::ReadOnly
			, AccessControl::DEFAULT
			, SideEffectType::kHasNoSideEffect
			, SideEffectType::kHasSideEffect
		);
#else
		
		comTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "rts" )
			, FunctionTemplate::New( isolate, ComObject::getRTS )
			, FunctionTemplate::New( isolate, ComObject::setRTS )
		);
#endif
		Local<Function> ComFunc = comTemplate->GetFunction( isolate->GetCurrentContext() ).ToLocalChecked();
		/*
		ComFunc->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "ports" )
			, Function::New( context, ComObject::getPorts ).FromMaybe( Local<Function>() )
			, Local<Function>()
			, PropertyAttribute::ReadOnly );
			*/
		// PropertyCallbackInfo
		//Object::DefineAccessor
		/*
		Handle<i::AccessorInfo> info =
			MakeAccessorInfo( i_isolate, name, getter, setter, data, settings,
				is_special_data_property, replace_on_access );
		*/
		//info->set_getter_side_effect_type( getter_side_effect_type );
		//info->set_setter_side_effect_type( setter_side_effect_type );

		
		ComFunc->SetNativeDataProperty( context, String::NewFromUtf8Literal( isolate, "ports" )
			, ComObject::getPorts
			, nullptr //Local<Function>()
			, Local<Value>()
			, PropertyAttribute::ReadOnly
			, SideEffectType::kHasSideEffect
			, SideEffectType::kHasSideEffect
		);
		
		ComFunc->SetNativeDataProperty(context, String::NewFromUtf8Literal(isolate, "onRemove")
			, nullptr
			, ComObject::onRemove
			, Local<Value>()
			, PropertyAttribute::None
			, SideEffectType::kHasSideEffect
			, SideEffectType::kHasSideEffect
		);
		ComFunc->SetNativeDataProperty(context, String::NewFromUtf8Literal(isolate, "onAdd")
			, nullptr
			, ComObject::onAdd
			, Local<Value>()
			, PropertyAttribute::None
			, SideEffectType::kHasSideEffect
			, SideEffectType::kHasSideEffect
		);

		class constructorSet *c = getConstructors( isolate );
		c->comConstructor.Reset( isolate, ComFunc );
		SET( exports, "ComPort", ComFunc );
}


void ComObject::getRTS2( Local<Name> property, const PropertyCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	ComObject* obj = ObjectWrap::Unwrap<ComObject>( args.This() );
	if( obj )
		args.GetReturnValue().Set( Boolean::New( args.GetIsolate(), (int)obj->rts ) );

}


void ComObject::getRTS( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	ComObject* obj = ObjectWrap::Unwrap<ComObject>( args.This() );
	if( obj )
		args.GetReturnValue().Set( Boolean::New( args.GetIsolate(), (int)obj->rts ) );

}

void ComObject::setRTS2( Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void>& args ) {
	Isolate* isolate = args.GetIsolate();
	ComObject* obj = ObjectWrap::Unwrap<ComObject>( args.This() );
	if( obj )
		SetCommRTS( obj->handle, obj->rts = value.As<Boolean>()->BooleanValue( isolate ) );
}


void ComObject::setRTS( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() > 0 ) {
		ComObject* obj = ObjectWrap::Unwrap<ComObject>( args.This() );
		if( obj )
			SetCommRTS( obj->handle, obj->rts = args[0].As<Boolean>()->BooleanValue( isolate ) );
	}
}


void dont_releaseBufferBackingStore(void* data, size_t length, void* deleter_data) {
	(void)length;
	(void)deleter_data;;
}

static void asyncmsg( uv_async_t* handle ) {
	v8::Isolate *isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	asyncmsg__( isolate, isolate->GetCurrentContext(), (ComObject *)handle->data );
}

static void asyncmsg__( Isolate *isolate, Local<Context> context, ComObject * myself ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	{
		struct msgbuf *msg;
		while( (myself && (msg = (struct msgbuf *)DequeLink( &myself->readQueue ))) ||
			   (!myself && (msg = (struct msgbuf *)DequeLink( &ComObject::_readQueue))) ) {
			size_t length;
			if( msg->op == MSG_OP_CLOSE ) {
				myself->jsObject.Reset();
				Release( msg );
				uv_close( (uv_handle_t*)&myself->async, NULL ); // have to hold onto the handle until it's freed.
				return;
			}
			else if (msg->op == MSG_OP_DATA) {
#if ( NODE_MAJOR_VERSION >= 14 )
				std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore(msg->buf, length = msg->buflen, dont_releaseBufferBackingStore, NULL);
				Local<ArrayBuffer> ab = ArrayBuffer::New(isolate, bs);
#else
				Local<ArrayBuffer> ab =
					ArrayBuffer::New(isolate,
						msg->buf,
						length = msg->buflen);

#endif

				Local<Uint8Array> ui = Uint8Array::New(ab, 0, length);

				Local<Value> argv[] = { ui };
				Local<Function> cb = Local<Function>::New(isolate, myself->readCallback[0]);
				//lprintf( "callback ... %p", myself );
				// using obj->jsThis  fails. here...
				{
					MaybeLocal<Value> result = cb->Call(isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, argv);
					if (result.IsEmpty()) {
						Deallocate(struct msgbuf*, msg);
						return;
					}
				}
				//lprintf( "called ..." );	
				Deallocate(struct msgbuf*, msg);
			}
			else if (msg->op == MSG_OP_REMOVE) {
				if (!ComObject::removeCallback.IsEmpty()) {
					Local<Value> argv[] = { String::NewFromUtf8(isolate, (char const*)msg->buf, v8::NewStringType::kNormal, msg->buflen).ToLocalChecked() };
					Local<Function> cb = Local<Function>::New(isolate, ComObject::removeCallback);
					cb->Call(isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, argv);
				}
			}
			else if (msg->op == MSG_OP_ADDED) {
				if (!ComObject::addCallback.IsEmpty()) {
					Local<Value> argv[] = { String::NewFromUtf8( isolate, (char const*)msg->buf, v8::NewStringType::kNormal, msg->buflen ).ToLocalChecked() };
					Local<Function> cb = Local<Function>::New(isolate, ComObject::addCallback);
					cb->Call(isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, argv);
				}
			}
		}
	}
	//lprintf( "done calling message notice." );
	{
		class constructorSet* c = getConstructors( isolate );
		if( !c->ThreadObject_idleProc.IsEmpty() ) {
			Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
			cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
		}
	}
}

void ComObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		class constructorSet *c = getConstructors( isolate );
		if( args.IsConstructCall() ) {
			char *portName;
			int argc = args.Length();
			if( argc > 0 ) {
				String::Utf8Value fName( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
				portName = StrDup( *fName );
			} else {
				isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, TranslateText( "Must specify port name to open." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
				return;
			}
			// Invoked as constructor: `new MyObject(...)`
#ifdef _WIN32			
			if (portName[4] != 0 && portName[0] != '\\') {
				char* newPort = NewArray(char, 12);
				snprintf(newPort, 12, "\\\\.\\%s", portName);
				Deallocate(char*, portName);
				portName = newPort;
			}
#endif
			ComObject* obj = new ComObject( portName );
			if( obj->handle < 0 )
			{
				char msg[256];
				snprintf( msg, 256, "Failed to open %s", obj->name );
				isolate->ThrowException( Exception::Error( String::NewFromUtf8(isolate, msg, v8::NewStringType::kNormal ).ToLocalChecked() ) );
			}
			else {
				//lprintf( "empty async...." );
				//MemSet( &obj->async, 0, sizeof( obj->async ) );
				//Environment* env = Environment::GetCurrent(args);
				if( c->ivm_post ) {
					obj->ivm_hosted = true;
					obj->c = c;
				} else
					uv_async_init( c->loop, &obj->async, asyncmsg );
				obj->async.data = obj;
				obj->jsObject.Reset( isolate, args.This() );
				obj->Wrap( args.This() );
				args.GetReturnValue().Set( args.This() );
			}

		}
		else {
			// Invoked as plain function `MyObject(...)`, turn into construct call.
			int argc = args.Length();
			Local<Value> *argv = new Local<Value>[argc];
			for( int n = 0; n < argc; n++ )
				argv[n] = args[n];

			Local<Function> cons = Local<Function>::New( isolate, c->comConstructor );
			MaybeLocal<Object> newInst = cons->NewInstance( isolate->GetCurrentContext(), argc, argv );
			if( !newInst.IsEmpty() )
				args.GetReturnValue().Set( newInst.ToLocalChecked() );
			delete[] argv;
		}
}



static void CPROC dispatchRead( uintptr_t psv, int nCommId, POINTER buffer, int len ) {
	struct msgbuf *msgbuf = NewPlus( struct msgbuf, len );
	//lprintf( "got read: %p %d", buffer, len );
	msgbuf->op = MSG_OP_DATA;
	MemCpy( msgbuf->buf, buffer, len );
	msgbuf->buflen = len;
	ComObject *com = (ComObject*)psv;
	if( !com->readCallback->IsEmpty() ) {
		EnqueLink( &com->readQueue, msgbuf );
		if( com->ivm_hosted )
			com->c->ivm_post( com->c->ivm_holder, std::make_unique<comAsyncTask>( com ) );
		else
			uv_async_send( &com->async );
	}
}
static void CPROC dispatchAdd(char const* name, size_t len) {
	struct msgbuf* msgbuf = NewPlus(struct msgbuf, len);
	//lprintf( "got read: %p %d", buffer, len );
	msgbuf->op = MSG_OP_ADDED;
	MemCpy(msgbuf->buf, name, len);
	msgbuf->buflen = len;
	if (!ComObject::removeCallback.IsEmpty()) {
		EnqueLink(&ComObject::_readQueue, msgbuf);
		if (ComObject::_ivm_hosted)
			ComObject::_c->ivm_post(ComObject::_c->ivm_holder, std::make_unique<comAsyncTask>((ComObject*)NULL));
		else
			uv_async_send(&ComObject::_async);
	}
}



static void CPROC dispatchRemove(char const *name, size_t len) {
	struct msgbuf* msgbuf = NewPlus(struct msgbuf, len);
	//lprintf( "got read: %p %d", buffer, len );
	msgbuf->op = MSG_OP_REMOVE;
	MemCpy(msgbuf->buf, name, len);
	msgbuf->buflen = len;
	if (!ComObject::removeCallback.IsEmpty()) {
		EnqueLink(&ComObject::_readQueue, msgbuf);
		if (ComObject::_ivm_hosted)
			ComObject::_c->ivm_post(ComObject::_c->ivm_holder, std::make_unique<comAsyncTask>((ComObject*)NULL));
		else
			uv_async_send(&ComObject::_async);
	}
}


void ComObject::onRead( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc < 1 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Must pass callback to onRead handler" ) ) );
		return;
	}

	ComObject *com = ObjectWrap::Unwrap<ComObject>( args.This() );

	if( com->handle >= 0 ) {
		SackSetReadCallback( com->handle, dispatchRead, (uintptr_t)com );
	}

	Local<Function> arg0 = Local<Function>::Cast( args[0] );
	com->readCallback = new Persistent<Function>(isolate,arg0);
}

void ComObject::writeCom( const v8::FunctionCallbackInfo<Value>& args ) {
	int argc = args.Length();
	if( argc < 1 ) {
		//isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "required parameter missing" ) ) );
		return;
	}
	Isolate* isolate = args.GetIsolate();
	ComObject *com = ObjectWrap::Unwrap<ComObject>( args.This() );

	//assert(args[i]->IsFloat32Array());
	if (args[0]->IsString()) {
		String::Utf8Value u8str( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked());
		SackWriteComm(com->handle, *u8str, u8str.length());
	}
	else if (args[0]->IsUint8Array()) {
		Local<Uint8Array> myarr = args[0].As<Uint8Array>();
#if ( NODE_MAJOR_VERSION >= 14 )
		std::shared_ptr<BackingStore> ab_c = myarr->Buffer()->GetBackingStore();
		char *buf = static_cast<char*>(ab_c->Data()) + myarr->ByteOffset();
#else
		ArrayBuffer::Contents ab_c = myarr->Buffer()->GetContents();
		char *buf = static_cast<char*>(ab_c.Data()) + myarr->ByteOffset();
#endif
		SackWriteComm( com->handle, buf, (int)myarr->Length() );
	}
	else if (args[0]->IsArrayBuffer()) {
		Local<ArrayBuffer> ab = Local<ArrayBuffer>::Cast( args[0] );
#if ( NODE_MAJOR_VERSION >= 14 )
		std::shared_ptr<BackingStore> ab_c = ab->GetBackingStore();
		char *buf = static_cast<char*>(ab_c->Data()) ;
#else
		ArrayBuffer::Contents ab_c = ab->GetContents();
		char *buf = static_cast<char*>(ab_c.Data()) ;
#endif
		SackWriteComm( com->handle, buf, (int)ab->ByteLength() );
	}

}

void ComObject::closeCom( const v8::FunctionCallbackInfo<Value>& args ) {
	
	ComObject *com = ObjectWrap::Unwrap<ComObject>( args.This() );
	if( com->handle >= 0 )
		SackCloseComm( com->handle );
	com->handle = -1;

	{
		//lprintf( "Garbage collected" );
		struct msgbuf* msgbuf = NewPlus( struct msgbuf, 0 );
		msgbuf->op = MSG_OP_CLOSE;
		EnqueLink( &com->readQueue, msgbuf );
		if( com->ivm_hosted )
			com->c->ivm_post( com->c->ivm_holder, std::make_unique<comAsyncTask>( com ) );
		else
			uv_async_send( &com->async );
	}

}


void ComObject::onRemove(Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void>& args) {
	Isolate* isolate = args.GetIsolate();
	ComObject::removeCallback.Reset(isolate, value.As<Function>());
	if (!ComObject::_c) {
		ComObject::_c = getConstructors(isolate);
		if (ComObject::_c->ivm_post) {
			ComObject::_ivm_hosted = true;
		}
		else
			uv_async_init( ComObject::_c->loop, &ComObject::_async, asyncmsg);
	}
}
void ComObject::onAdd(Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void>& args) {
	Isolate* isolate = args.GetIsolate();
	ComObject::addCallback.Reset(isolate, value.As<Function>());
	if (!ComObject::_c) {
		ComObject::_c = getConstructors(isolate);
		if (ComObject::_c->ivm_post) {
			ComObject::_ivm_hosted = true;
		}
		else
			uv_async_init(ComObject::_c->loop, &ComObject::_async, asyncmsg);
	}
}

struct port_info {
	size_t portLen;
	size_t nameLen;
	size_t techLen;
	TEXTCHAR buf[];
};

static LOGICAL portCallback( uintptr_t psv, LISTPORTS_PORTINFO* lpPortInfo ) {
	size_t portLen = StrLen( lpPortInfo->lpPortName );
	size_t nameLen = StrLen( lpPortInfo->lpFriendlyName );
	size_t techLen = StrLen( lpPortInfo->lpTechnology );

	struct port_info *pi = NewPlus( struct port_info, portLen+nameLen+techLen );
	pi->portLen = portLen;
	pi->nameLen = nameLen;
	pi->techLen = techLen;
	MemCpy( pi->buf, lpPortInfo->lpPortName, portLen );
	MemCpy( pi->buf+portLen, lpPortInfo->lpFriendlyName, nameLen );
	MemCpy( pi->buf+portLen+nameLen, lpPortInfo->lpTechnology, techLen );
	PLIST* ppList = (PLIST*)psv;
	AddLink( ppList, pi );
	return TRUE;
}

void ComObject::getPorts( Local<Name> property, const PropertyCallbackInfo<Value>& args ) {
	//( const v8::FunctionCallbackInfo<Value>& args ) {
	PLIST list = NULL;
	ListPorts( portCallback, (uintptr_t)&list );
	INDEX idx;
	struct port_info *pi;

	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Array> jsList = Array::New( isolate );

	LIST_FORALL( list, idx, struct port_info*, pi ) {
		Local<Object> info = Object::New( isolate );
		SET( info, "port", String::NewFromUtf8( isolate, pi->buf, v8::NewStringType::kNormal, pi->portLen ).ToLocalChecked() );
		SET( info, "name", String::NewFromUtf8( isolate, pi->buf+pi->portLen, v8::NewStringType::kNormal, pi->nameLen ).ToLocalChecked() );
		SET( info, "technology", String::NewFromUtf8( isolate, pi->buf+pi->portLen+pi->nameLen, v8::NewStringType::kNormal, pi->techLen ).ToLocalChecked() );
		SETN( jsList, idx, info );
		Deallocate( struct port_info*, pi );
	}
	//lprintf( "Returning %d ports", idx );
	args.GetReturnValue().Set( jsList );	
}

#if defined( _WIN32 )

static LONG CALLBACK MonitorMessageHandler( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	switch( uMsg ) {
	case WM_DEVICECHANGE: {
		switch( wParam ) {
			case DBT_DEVICEARRIVAL: {
					DEV_BROADCAST_HDR* msg = (DEV_BROADCAST_HDR*)lParam;
					switch (msg->dbch_devicetype) {
						default:
							lprintf("Unhandled device type: %d", msg->dbch_devicetype);
							break;
						case DBT_DEVTYP_PORT: {
							DEV_BROADCAST_PORT_A* msg = (DEV_BROADCAST_PORT_A*)lParam;
							//lprintf("Added device: %s", msg->dbcp_name);
							dispatchAdd(msg->dbcp_name, msg->dbcp_size - sizeof(DEV_BROADCAST_HDR));
						}
						break;
					}
				}
				break;
			case DBT_DEVICEREMOVECOMPLETE: {
				DEV_BROADCAST_HDR *msg = (DEV_BROADCAST_HDR *)lParam;
				switch( msg->dbch_devicetype ) {
				default:
					lprintf("Unhandled device type: %d", msg->dbch_devicetype);
					break;
				case DBT_DEVTYP_PORT: {
					DEV_BROADCAST_PORT_A* msg = (DEV_BROADCAST_PORT_A*)lParam;
					//lprintf("Removed device: %s", msg->dbcp_name);
					dispatchRemove(msg->dbcp_name, msg->dbcp_size - sizeof(DEV_BROADCAST_HDR));
				}
						
					break;
				}
			}
				break;
			case DBT_DEVNODES_CHANGED:
				// no further information, just look at the port list?
				break;
			default: 
				lprintf( "Unhandled device change: %d", wParam );
				break;
			}
			return TRUE; // allow.
		}
	}
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}


static uintptr_t CPROC RegisterAndCreateMonitor( PTHREAD thread )
{
  // zero init.
	static WNDCLASS wc;
	static HWND ghWndIcon;
	static ATOM ac;
	static PTHREAD pMyThread;
	if( !ac )
	{
		//WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");
		memset( &wc, 0, sizeof( WNDCLASS ) );
		wc.lpfnWndProc   = (WNDPROC)MonitorMessageHandler;
		wc.hInstance     = GetModuleHandle( NULL ) ;
		wc.lpszClassName = "ComPortDeviceMonitor";
		if( !( ac = RegisterClass(&wc) ) )
		{
			TEXTCHAR byBuf[256];
			if( GetLastError() != ERROR_CLASS_ALREADY_EXISTS )
			{
				tnprintf( byBuf, sizeof( byBuf ), "RegisterClassError: %p %d", GetModuleHandle( NULL ), GetLastError() );
				MessageBox( NULL, byBuf, "BAD", MB_OK );
   // stop thread
				return FALSE;
			}
		}
		pMyThread = MakeThread();
		//AddIdleProc( systrayidle, 0 );
	}
	if( !ghWndIcon )
	{
		TEXTCHAR wndname[256];
		tnprintf( wndname, sizeof( wndname ), "ComPortDeviceMonitor:%s", GetProgramName() );
		ghWndIcon = CreateWindow( MAKEINTATOM(ac),
										 wndname,
										 0,0,0,0,0,NULL,NULL,NULL,NULL);
	}
	if( !ghWndIcon )
	{
		MessageBox( NULL, "Device Monitor cannot load (no window)", "Exiting now", MB_OK );
		return FALSE;
	}
	if( thread )
	{
		MSG msg;
		/*
		DEV_BROADCAST_PORT_A devmsg;
		DEV_BROADCAST_HDR *hdr   = (DEV_BROADCAST_HDR *)&devmsg;
		hdr->dbch_size = sizeof( DEV_BROADCAST_PORT_A );
		hdr->dbch_devicetype   = DBT_DEVTYP_PORT;
		hdr->dbch_reserved     = 0;
		// this fails if devtype_port is used anyway :) 
		HANDLE hDeviceNotication = RegisterDeviceNotification( (HANDLE)ghWndIcon, &devmsg, DEVICE_NOTIFY_WINDOW_HANDLE );
		*/
		// thread_ready = TRUE;
		while( GetMessage( &msg, NULL, 0, 0 ) )
			DispatchMessage( &msg );
		return 0;
	}
	return TRUE;
}

void reEnablePort( char const *port ) {
	DEVINST inst;

    GUID classGuid;
    ULONG classIndex = 0;
    CONFIGRET cr;
    lprintf("Enumerating Device Setup Classes:");

    do {
        cr = CM_Enumerate_Classes(classIndex, &classGuid, 0); // Enumerate device setup classes

        if (cr == CR_SUCCESS) {
            // Convert GUID to string for printing
            WCHAR guidString[MAX_GUID_STRING_LEN];
            StringFromGUID2(classGuid, guidString, MAX_GUID_STRING_LEN);
            lprintf("  Class GUID: %S", guidString);


			DEVPROPTYPE propType;
			std::vector<BYTE> propertyBuffer(256); // Start with a reasonable buffer size.
			ULONG propertyBufferSize = (ULONG)propertyBuffer.size();

			// Retrieve the class name property (DEVPKEY_NAME).
			cr = CM_Get_Class_Property_ExW(
				&classGuid,
				&DEVPKEY_NAME,
				&propType,
				propertyBuffer.data(),
				&propertyBufferSize,
				0,
				0
			);
			if (cr == CR_BUFFER_SMALL) {
				// Buffer was too small, resize and try again.
				propertyBuffer.resize(propertyBufferSize);
				cr = CM_Get_Class_Property_ExW(
					&classGuid,
					&DEVPKEY_NAME,
					&propType,
					propertyBuffer.data(),
					&propertyBufferSize,
					0,
					0
				);
			}

			if (cr == CR_SUCCESS) {
				// DEVPKEY_NAME returns a DEVPROP_TYPE_STRING type.
				if (propType == DEVPROP_TYPE_STRING) {
					const wchar_t* className = reinterpret_cast<const wchar_t*>(propertyBuffer.data());
					lprintf( "  - %S  (GUID: {%S}) ", className, classGuid );
					TEXTSTR t_className = WcharConvert((const wchar_t*)className);
					lprintf("(%s) (%s)", t_className, "Ports (COM & LPT)");
					if ( 1 || StrCmp(t_className, "Ports (COM & LPT)") == 0) {
						lprintf("So we should see what devices are there..");
						// GUID_DEVCLASS_PORTS
						HDEVINFO hdi = SetupDiGetClassDevs(&classGuid, NULL, NULL, DIGCF_PRESENT);
							//SetupDiCreateDeviceInfoList(&classGuid, NULL);
						DWORD devId = 0;
						SP_DEVINFO_DATA data;
						SP_DEVICE_INTERFACE_DATA intData;
						data.cbSize = sizeof(data);
						while (1) {

							if (SetupDiEnumDeviceInfo(hdi, devId, &data)) {
								//data.InterfaceClassGuid
								//data.Flags
								// is this the right device though?

								lprintf("Got a device... %d", data.DevInst);
								DWORD instId = 0;
								DEVPROPKEY keys[12];
								ULONG     keyCount = 12;
								if (!CM_Get_DevNode_Property_Keys(
									(DEVINST)data.DevInst,
									&keys[0],
									&keyCount,
									0
								)) {
								}
								else {
									lprintf("Got keys?");
									for (int i = 0; i < keyCount; i++) {
										lprintf( "something %d", keys[i].pid);
									}
								}

								if (SetupDiEnumDeviceInterfaces(hdi, &data, &classGuid, instId, &intData)) {
									lprintf("Got intdata?");
									//SetupDiGetDeviceInterfaceDetail(hdi,
								}
								else {
									DWORD dwError = GetLastError();
									lprintf("intdata Err: %d", dwError);
								}

							}
							else {
								DWORD dwError = GetLastError();
								lprintf("Err: %d", dwError );
								if (dwError == ERROR_NO_MORE_ITEMS) break;
							}
							devId++;
						}
						SetupDiDestroyDeviceInfoList(hdi);

					}
				}
			}
			else {
				lprintf( "CM_Get_Class_Property failed for a class with error: %d", cr );
			}

			

        } else if (cr == CR_NO_SUCH_VALUE) {
            // End of enumeration
            break;
        } else if (cr == CR_INVALID_DATA) {
            // Ignore invalid data entries and continue enumeration
            lprintf("  Warning: Invalid data encountered for class index %lu. Skipping.", classIndex);
        } else {
            lprintf("  Error enumerating classes: %lu", cr);
            break;
        }
        classIndex++;
    } while (TRUE);	


	CONFIGRET r1 = CM_Disable_DevNode( inst, CM_DISABLE_UI_NOT_OK );
	CONFIGRET r2 = CM_Enable_DevNode( inst, 0 );
	
}


#endif