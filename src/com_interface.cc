#define FIX_RELEASE_COM_COLLISION
#include "global.h"

#ifdef _WIN32
// device something?
#include <dbt.h>
#include <cfgmgr32.h> 
#include <initguid.h>
#include <devpkey.h>
#include <setupapi.h>
#include <guiddef.h>
#include <ntstatus.h>

#include "makepkeytable.h"
void updateNames( void );

#endif

enum com_interface_msgbuf_op {
	MSG_OP_DATA = 0,
	MSG_OP_CLOSE,
	MSG_OP_REMOVE,
	MSG_OP_ADDED,
};

struct com_interface_msgbuf {
	int op;
	size_t buflen;
	uint8_t buf[1];
};

static struct com_port_local {
	PLIST ports;
	PLIST want_enable;
	PTHREAD event_monitor; // windows message thread for device changes
} l;

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
	char *portName; // pointer into `name` that is just the last com port name part
	//static Persistent<Function> constructor;
	bool rts = 1;
	bool wantEnable;
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
	static void resetCom( const v8::FunctionCallbackInfo<Value> &args );
	static void resetComByName( const v8::FunctionCallbackInfo<Value> &args );
	static void disableComByName( const v8::FunctionCallbackInfo<Value> &args );
	static void enableComByName( const v8::FunctionCallbackInfo<Value> &args );
	static void getProperties( Local<Name> property, const PropertyCallbackInfo<Value> &args );
	
	~ComObject();
};

Persistent<Function> ComObject::removeCallback; //
Persistent<Function> ComObject::addCallback; //
uv_async_t ComObject::_async; // keep this instance around for as long as we might need to do the periodic callback
class constructorSet* ComObject::_c;
bool ComObject::_ivm_hosted = false;
PLINKQUEUE ComObject::_readQueue;

static	uintptr_t CPROC RegisterAndCreateMonitor( PTHREAD thread );
static void reEnablePort( char const *port, bool enable = false );
static void getPortProperties( char const*com, Isolate * isolate, Local<Object> result );
int64_t FiletimeToJavascriptTick( FILETIME ft );


ComObject::ComObject( char *name ) : jsObject() {
	this->readQueue = CreateLinkQueue();
	this->name = name;
	this->portName  = name;
	AddLink( &l.ports, this );
	if( this->portName[ 0 ] == '\\' )
		this->portName += 4;
	handle = SackOpenComm( name, 0, 0 );
	// when opening a port, start monitoring for change events.

	ThreadTo( RegisterAndCreateMonitor, 0 );
}

ComObject::~ComObject() {
	DeleteLink( &l.ports, this );
	if( handle >= 0 )
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
#ifdef _WIN32
	   updateNames(); // this is used for port[].properties
	   if(0)
	   {
		   ThreadTo( RegisterAndCreateMonitor, 0 );
	   }
#endif
		//reEnablePort("");
	   comTemplate = FunctionTemplate::New( isolate, New );
		comTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.ComPort" ) );
		comTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

		// Prototype
		NODE_SET_PROTOTYPE_METHOD( comTemplate, "onRead", onRead );
		NODE_SET_PROTOTYPE_METHOD( comTemplate, "write", writeCom );
		NODE_SET_PROTOTYPE_METHOD( comTemplate, "close", closeCom );
	   NODE_SET_PROTOTYPE_METHOD( comTemplate, "reset", resetCom );

#if ( NODE_MAJOR_VERSION >= 22 )
		comTemplate->PrototypeTemplate()->SetNativeDataProperty( String::NewFromUtf8Literal( isolate, "rts" )
			, ComObject::getRTS2
			, ComObject::setRTS2
			, Local<Value>()
			, PropertyAttribute::None // readonly blocks setter from happening.
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

		SET( ComFunc, "reset", Function::New( context, resetComByName ).ToLocalChecked() );
	   SET( ComFunc, "disable", Function::New( context, disableComByName ).ToLocalChecked() );
	   SET( ComFunc, "enable", Function::New( context, enableComByName ).ToLocalChecked() );


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
		struct com_interface_msgbuf *msg;
		while( ( myself && ( msg = (struct com_interface_msgbuf *)DequeLink( &myself->readQueue ) ) )
		     || ( !myself && ( msg = (struct com_interface_msgbuf *)DequeLink( &ComObject::_readQueue ) ) ) ) {
			size_t length;
			if( msg->op == MSG_OP_CLOSE ) {
				myself->jsObject.Reset();
				Deallocate( struct com_interface_msgbuf *, msg );
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
						Deallocate( struct com_interface_msgbuf *, msg );
						return;
					}
				}
				//lprintf( "called ..." );	
				Deallocate( struct com_interface_msgbuf *, msg );
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
				snprintf(newPort, 12, "\\\\.\\COM%s", portName+3);
				Deallocate(char*, portName);
				portName = newPort;
		   } else {
			   portName[ 0 ] = 'C';
			   portName[ 1 ] = 'O';
			   portName[ 2 ] = 'M';
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
	struct com_interface_msgbuf *msgbuf = NewPlus( struct com_interface_msgbuf, len );
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
	struct com_interface_msgbuf* msgbuf = NewPlus(struct com_interface_msgbuf, len);
	//lprintf( "got read: %p %d", buffer, len );
	size_t reallen                      = 0;
	while( name[ reallen++ ] );
	if( reallen <= len )
		len = reallen-1;
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
	struct com_interface_msgbuf* msgbuf = NewPlus(struct com_interface_msgbuf, len);
	//lprintf( "got read: %p %d", buffer, len );
	msgbuf->op = MSG_OP_REMOVE;
	size_t reallen                      = 0;
	while( name[ reallen++ ] );
	if( reallen <= len )
		len = reallen-1;
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

void ComObject::resetComByName( const v8::FunctionCallbackInfo<Value> &args ) {
	Isolate *isolate = args.GetIsolate();
	String::Utf8Value port( isolate, args[ 0 ].As<String>() );
	//com->wantEnable = true;
	AddLink( &l.want_enable, StrDup( *port ) );
	reEnablePort( *port, false );
}

void ComObject::disableComByName( const v8::FunctionCallbackInfo<Value> &args ) {
	Isolate *isolate = args.GetIsolate();
	String::Utf8Value port( isolate, args[ 0 ].As<String>() );
	reEnablePort( *port, false );
}
void ComObject::enableComByName( const v8::FunctionCallbackInfo<Value> &args ) {
	Isolate *isolate = args.GetIsolate();
	String::Utf8Value port( isolate, args[ 0 ].As<String>() );
	reEnablePort( *port, true );
}




void ComObject::resetCom( const v8::FunctionCallbackInfo<Value> &args ) {
	ComObject *com = ObjectWrap::Unwrap<ComObject>( args.This() );
	if( com->handle >= 0 ) {
		SackCloseComm( com->handle );
	}
	com->wantEnable = true;
	reEnablePort( com->portName, false );
}
	

void ComObject::closeCom( const v8::FunctionCallbackInfo<Value>& args ) {
	
	ComObject *com = ObjectWrap::Unwrap<ComObject>( args.This() );
	if( com->handle >= 0 )
		SackCloseComm( com->handle );
	com->handle = -1;

	{
		//lprintf( "Garbage collected" );
		struct com_interface_msgbuf* msgbuf = NewPlus( struct com_interface_msgbuf, 0 );
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
		ThreadTo( RegisterAndCreateMonitor, 0 );
		ComObject::_c = getConstructors( isolate );
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
		ThreadTo( RegisterAndCreateMonitor, 0 );
		ComObject::_c = getConstructors( isolate );
		if (ComObject::_c->ivm_post) {
			ComObject::_ivm_hosted = true;
		}
		else
			uv_async_init(ComObject::_c->loop, &ComObject::_async, asyncmsg);
	}
}


void ComObject::getProperties( Local<Name> property, const PropertyCallbackInfo<Value> &args ) {
	Isolate *isolate = args.GetIsolate();
	//ComObject *com   = ObjectWrap::Unwrap<ComObject>( args.This() );
	String::Utf8Value port( USE_ISOLATE( isolate ) args.This()
	             ->Get( isolate->GetCurrentContext(), String::NewFromUtf8Literal( isolate, "port" ) )
	             .ToLocalChecked().As<String>() );
	Local<Object> result = Object::New( isolate );
	getPortProperties( *port, isolate, result );
	args.GetReturnValue().Set( result );
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

		info->SetNativeDataProperty( context, 
		     String::NewFromUtf8Literal( isolate, "properties" ), ComObject::getProperties, nullptr, Local<Value>()
		     , PropertyAttribute::ReadOnly, SideEffectType::kHasNoSideEffect, SideEffectType::kHasSideEffect );


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
		//lprintf( "Device change message %d", wParam );
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
				   INDEX idx;
				   ComObject *com;
				   LIST_FORALL(l.ports, idx, ComObject*, com) {
					   if( StrCmp( com->portName, msg->dbcp_name ) == 0 ) {
						   if( com->wantEnable ) {
							   reEnablePort( com->portName, true );
							   com->wantEnable = false;
						   }
						   break;
					   }
				   }
				   char *portname;
				   LIST_FORALL(l.want_enable, idx, char*, portname) {
					   if( StrCmp( portname, msg->dbcp_name ) == 0 ) {
						   reEnablePort( portname, true );
						   SetLink( &l.want_enable, idx, NULL );
						   Deallocate( char *, portname );
						   break;
					   }
				   }

					dispatchRemove(msg->dbcp_name, msg->dbcp_size - sizeof(DEV_BROADCAST_HDR));
				}
						
					break;
				}
			}
				break;
			case DBT_DEVNODES_CHANGED:
				// no further information, just look at the port list?
			   //lprintf( "something changed somewhere..." );
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


uintptr_t CPROC RegisterAndCreateMonitor( PTHREAD thread )
{
  // zero init.
	static WNDCLASS wc;
	static HWND ghWndIcon;
	static ATOM ac;
	if( l.event_monitor )
		return 0; // already running
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
		l.event_monitor = thread;
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

	DEFINE_GUID( ComPortsClass, 0x4D36E978, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 );

void getPortProperties( char const*com, Isolate * isolate, Local<Object> result ) {
	   Local<Array> unnamed   = Array::New( isolate );
	   int last_unnamed       = 0; // we don't have just 'push'

	   Local<Context> context = isolate->GetCurrentContext();
	   SET( result, "unnamed", unnamed );
	   DEVINST inst;

	   GUID classGuid = ComPortsClass;
	   //= 4D36E978 - E325 - 11CE-BFC1 - 08002BE10318;
	   CONFIGRET cr;
	   // 4D36E978-E325-11CE-BFC1-08002BE10318

	   //lprintf( "Enumerating Device Setup Classes:" );

	   // cr = CM_Enumerate_Classes(classIndex, &classGuid, 0); // Enumerate device setup classes

	   // if (cr == CR_SUCCESS)
	   {
		   // Convert GUID to string for printing
		   // WCHAR guidString[ MAX_GUID_STRING_LEN ];
		   // StringFromGUID2( classGuid, guidString, MAX_GUID_STRING_LEN );
		   // lprintf( "  Class GUID: %S", guidString );

		   DEVPROPTYPE propType;
		   size_t bufferLen         = 1024;
		   POINTER propertyBuffer   = NewArray( uint8_t, bufferLen );
		   // std::vector<BYTE> propertyBuffer( 256 ); // Start with a reasonable buffer size.
		   ULONG propertyBufferSize = (ULONG)bufferLen;

		   // Retrieve the class name property (DEVPKEY_NAME).
		   cr = CM_Get_Class_Property_ExW( &ComPortsClass, &DEVPKEY_NAME, &propType, (PBYTE)propertyBuffer
		                                 , &propertyBufferSize, 0, 0 );
		   if( cr == CR_NO_SUCH_REGISTRY_KEY ) {
			   lprintf( "Bad GUID Initialization..." );
		   }
		   if( cr == CR_BUFFER_SMALL ) {
			   // Buffer was too small, resize and try again.
			   propertyBuffer = ReallocateEx( propertyBuffer, propertyBufferSize DBG_SRC );
			   cr             = CM_Get_Class_Property_ExW( &ComPortsClass, &DEVPKEY_NAME, &propType, (PBYTE)propertyBuffer
			                                             , &propertyBufferSize, 0, 0 );
		   }

		   if( cr == CR_SUCCESS ) {
			   // DEVPKEY_NAME returns a DEVPROP_TYPE_STRING type.
			   if( propType == DEVPROP_TYPE_STRING ) {
				   // const wchar_t *className = reinterpret_cast<const wchar_t *>( propertyBuffer );
				   // lprintf( "  - %S  (GUID: {%S}) ", className, guidString );
				   // TEXTSTR t_className = WcharConvert( (const wchar_t *)className );
				   // lprintf( "(%s) (%s)", t_className, "Ports (COM & LPT)" );

					   //lprintf( "So we should see what devices are there.." );
					   // GUID_DEVCLASS_PORTS
					   // this id only com port class.
					   HDEVINFO hdi = SetupDiGetClassDevs( &classGuid, NULL, NULL, DIGCF_PRESENT );

					   // SetupDiCreateDeviceInfoList(&classGuid, NULL);
					   DWORD devId  = 0;
					   SP_DEVINFO_DATA data;
					   SP_DEVICE_INTERFACE_DATA intData;
					   data.cbSize = sizeof( data );
					   // enumerate devices...
					   while( 1 ) {
						   // in loop to enum all devices.
						   // get the data.
						   if( !SetupDiEnumDeviceInfo( hdi, devId, &data ) ) {
							   DWORD dwError = GetLastError();
							   if( dwError == ERROR_NO_MORE_ITEMS )
								   break;
							   lprintf( "Err: %d", dwError );
							   break;
						   }

						   {
							   // data.InterfaceClassGuid
							   // data.Flags
							   //  is this the right device though?

							   //lprintf( "Got a device... %d", data.DevInst );

							   HKEY hKey = SetupDiOpenDevRegKey( hdi, &data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE );
							   DWORD dwError = GetLastError();
							   char namebuf[ 256 ];
							   DWORD namebuflen = 256;
							   int queryErr = RegQueryValueEx( hKey, "PortName", NULL, NULL, (LPBYTE)namebuf, &namebuflen );
							   // lprintf( "Device: %.*s", namebuflen, namebuf );

							   // not zero is mis-match.
							   if( StrCmp( com, namebuf ) ) {
								   devId++;
								   continue;
							   }

							   // this code gets all the known properties of a device, and should be moved to
							   // something that gets the port info, separete from the eanble
							   {
								   DWORD instId = 0;
								   DEVPROPKEY *keys;
								   ULONG keyCount = 0;

								   // get how many there are expected...
								   if( !CM_Get_DevNode_Property_Keys( (DEVINST)data.DevInst, &keys[ 0 ], &keyCount, 0 ) ) {
								   } else {
									   keys = NewArray( DEVPROPKEY, keyCount );
								   }
								   // get all the property names.
								   if( CM_Get_DevNode_Property_Keys( (DEVINST)data.DevInst, keys, &keyCount, 0 )
								     != CR_SUCCESS ) {
									   lprintf( "not enough keys? %d", GetLastError() );
								   } else {
									   // lprintf( "Got keys?" );
									   for( int i = 0; i < keyCount; i++ ) {
										   // lprintf( "something %d", keys[ i ].pid );
										   // WCHAR psz[ 256 ];
										   // HRESULT res1 = PSStringFromPropertyKey( (PROPERTYKEY const &)( keys[ i ] ), psz, 256
										   // ); lprintf( "KeyVal? %S", psz );

										   MaybeLocal<String> use_name;
										   bool use_index = false;

										   {
											   PWSTR name;
											   HRESULT hr = PSGetNameFromPropertyKey( (PROPERTYKEY const &)( keys[ i ] ), &name );
											   if( hr == S_OK ) {
												   use_name  = String::NewFromTwoByte( isolate, (uint16_t const *)name );
												   use_index = false;
												   //lprintf( "Key Name: %S", name );
											   } else {
												   if( hr == TYPE_E_ELEMENTNOTFOUND ) {
													   int n;
													   for( n = 0; pkey_table[ n ].name; n++ ) {
														   if( MemCmp( keys + i, &pkey_table[ n ].key, sizeof( PROPERTYKEY ) )
														     == 0 ) {
															   use_name  = String::NewFromUtf8( isolate, pkey_table[ n ].niceName );
															   use_index = false;
															   // lprintf( "Recovered Key Name: %s", pkey_table[ n ].niceName );
															   break;
														   }
													   }
													   if( !pkey_table[ n ].name ) {
														   use_index = true;
														   //lprintf( "Failed to find as a predefined key... ---------------- (val "
														   //         "only?)" );
													   }
												   } else
													   lprintf( "Key Error: %08x", hr );
											   }
											   CoTaskMemFree( name );
										   }

										   BYTE buffer[ 1024 ];
										   ULONG buflen = 1024;
										   DEVPROPTYPE propType;
										   CM_Get_DevNode_PropertyW( (DEVINST)data.DevInst, keys + i, &propType, buffer, &buflen
										                           , 0 );
										   switch( propType ) {
										   default:
											   lprintf( "Unhandled type: %d(%08x)", propType, propType );
											   break;
										   case DEVPROP_TYPE_STRING_LIST: {
											   Local<Array> list = Array::New( isolate );
											   int insertat      = 0;
											   WCHAR *start = (WCHAR *)buffer;
											   while( start[ 0 ] ) {
												   //lprintf( "StringListVal: %S", start );
												   list->Set( context, insertat++, String::NewFromTwoByte( isolate, (uint16_t const*)start ).ToLocalChecked() );
												   while( start[ 0 ] )
													   start++;
											   }

											   if( use_index ) {
												   unnamed->Set( context, last_unnamed++, list );
											   } else {
												   result->Set( context, use_name.ToLocalChecked()
												              , list );
											   }
										   } break;
										   case DEVPROP_TYPE_STRING:
											   if( use_index ) {
												   unnamed->Set( context, last_unnamed++
												               , String::NewFromTwoByte( isolate, (uint16_t const *)buffer )
												                      .ToLocalChecked() );
											   } else {
												   result->Set( context, use_name.ToLocalChecked()
												              , String::NewFromTwoByte( isolate, (uint16_t const *)buffer )
												                     .ToLocalChecked() );
											   }
											   // lprintf( "WString: %S", buffer );
											   break;
										   case DEVPROP_TYPE_FILETIME:
											   //lprintf( "FileTime: %ulld", ( (FILETIME *)buffer )[ 0 ] );
										   {
												   int64_t jstick = FiletimeToJavascriptTick( ( (FILETIME *)buffer )[0] );
												if( use_index ) {
													unnamed->Set( context, last_unnamed++
													            , Date::New( context, jstick ).ToLocalChecked() );
												} else {
													result->Set( context, use_name.ToLocalChecked()
													           , Date::New( context, jstick ).ToLocalChecked() );
												}

										   }
											   break;
										   case DEVPROP_TYPE_NTSTATUS:
										   case DEVPROP_TYPE_UINT32:
											   if( use_index ) {
												   unnamed->Set( context, last_unnamed++
												               , Integer::NewFromUnsigned( isolate, ( (uint32_t *)buffer )[ 0 ] ) );
											   } else {
												   result->Set( context, use_name.ToLocalChecked()
												              , Integer::NewFromUnsigned( isolate, ( (uint32_t *)buffer )[ 0 ] ) );
											   }
											   // lprintf( "UINT32:%d", ( (uint32_t *)buffer )[ 0 ] );
											   break;
										   case DEVPROP_TYPE_BOOLEAN:
											   if( use_index ) {
												   unnamed->Set( context, last_unnamed++
												               , Boolean::New( isolate, ( (bool *)buffer )[ 0 ] ) );
											   } else {
												   result->Set( context, use_name.ToLocalChecked()
												              , Boolean::New( isolate, ( (bool *)buffer )[ 0 ] ) );
											   }
											   // lprintf( "BOOLEAN:%s", ( (bool *)buffer )[ 0 ] ? "TRUE" : "FALSE" );
											   break;
										   case DEVPROP_TYPE_GUID: {
											   WCHAR guidString[ MAX_GUID_STRING_LEN ];
											   StringFromGUID2( classGuid, guidString, MAX_GUID_STRING_LEN );
											   //lprintf( "GUID Val: %S", guidString );
											   if( use_index ) {
												   unnamed->Set( context, last_unnamed++
												               , String::NewFromTwoByte( isolate, (uint16_t const *)guidString )
												                      .ToLocalChecked() );
											   } else {
												   result->Set( context, use_name.ToLocalChecked()
												              , String::NewFromTwoByte( isolate, (uint16_t const *)guidString )
												                     .ToLocalChecked() );
											   }
										   }

										   break;
										   case DEVPROP_TYPE_BINARY:
											   //lprintf( "Binary value..." );
											   //LogBinary( buffer, buflen );
											   break;
										   }

										   // DEVPROPTYPE propType;
										   size_t bufferLen         = 1024;
										   POINTER propertyBuffer   = NewArray( uint8_t, bufferLen );
										   // std::vector<BYTE> propertyBuffer( 256 ); // Start with a reasonable buffer size.
										   ULONG propertyBufferSize = (ULONG)bufferLen;

										   /*
										   if( !CM_Get_Class_Property_ExW( (DEVINST)data.DevInst, &keys[ 0 ], &keyCount, 0 ) ) {
										   } else {
										      keys = NewArray( DEVPROPKEY, keyCount );
										   }
										   */
										   // Retrieve the class name property (DEVPKEY_NAME).
										   cr = CM_Get_Class_Property_ExW( &ComPortsClass, NULL, &propType, (PBYTE)propertyBuffer
										                                 , &propertyBufferSize, 0, 0 );
										   if( cr == CR_NO_SUCH_REGISTRY_KEY ) {
											   lprintf( "Bad GUID Initialization..." );
										   }
										   if( cr == CR_NO_SUCH_VALUE ) {
											   lprintf( "NO such value..." );
										   }
										   if( cr == CR_BUFFER_SMALL ) {
											   // Buffer was too small, resize and try again.
											   propertyBuffer = ReallocateEx( propertyBuffer, propertyBufferSize DBG_SRC );
											   cr             = CM_Get_Class_Property_ExW( &ComPortsClass, &DEVPKEY_NAME, &propType
											                                             , (PBYTE)propertyBuffer, &propertyBufferSize, 0, 0 );
										   }
										   if( cr == ERROR_SUCCESS ) {
											   if( propType == DEVPROP_TYPE_STRING ) {
												   TEXTSTR t_className = WcharConvert( (const wchar_t *)propertyBuffer );
												   lprintf( "Property name:%s", t_className );
											   } else {
												   lprintf( "unsupported format %d", propType );
											   }
										   }
									   }
								   }
							   }
						   }
						   devId++;
					   }
					   SetupDiDestroyDeviceInfoList( hdi );
				
			   }
		   } else {
			   lprintf( "CM_Get_Class_Property failed for a class with error: %d", cr );
		   }
	   }
	   /*
	   } else if (cr == CR_NO_SUCH_VALUE) {
	       // End of enumeration
	     //  break;
	   } else if (cr == CR_INVALID_DATA) {
	       // Ignore invalid data entries and continue enumeration
	       lprintf("  Warning: Invalid data encountered for class index %lu. Skipping.", classIndex);
	   } else {
	       lprintf("  Error enumerating classes: %lu", cr);
	    //   break;
	   }
	          */

	   // CONFIGRET r1 = CM_Disable_DevNode( inst, CM_DISABLE_UI_NOT_OK );
	   // CONFIGRET r2 = CM_Enable_DevNode( inst, 0 );
   }


void reEnablePort( char const *port, bool enable ) {
	DEVINST inst;

    GUID classGuid = ComPortsClass;
	//= 4D36E978 - E325 - 11CE-BFC1 - 08002BE10318;
    ULONG classIndex = 0;
    CONFIGRET cr;
	// 4D36E978-E325-11CE-BFC1-08002BE10318
	
    //lprintf("Enumerating Device Setup Classes:");

    //do 
	{
        //cr = CM_Enumerate_Classes(classIndex, &classGuid, 0); // Enumerate device setup classes

        //if (cr == CR_SUCCESS) 
		{
			  // Convert GUID to string for printing
			  //WCHAR guidString[ MAX_GUID_STRING_LEN ];
			  //StringFromGUID2( classGuid, guidString, MAX_GUID_STRING_LEN );
			  //lprintf( "  Class GUID: %S", guidString );

			  DEVPROPTYPE propType;
			  size_t bufferLen = 1024;
			  POINTER propertyBuffer   = NewArray( uint8_t, bufferLen );
			  //std::vector<BYTE> propertyBuffer( 256 ); // Start with a reasonable buffer size.
			  ULONG propertyBufferSize = (ULONG)bufferLen;


			  // Retrieve the class name property (DEVPKEY_NAME).
			  cr = CM_Get_Class_Property_ExW( &ComPortsClass, &DEVPKEY_NAME, &propType, (PBYTE)propertyBuffer
			                                , &propertyBufferSize, 0, 0 );
			  if( cr == CR_NO_SUCH_REGISTRY_KEY ) {
				  lprintf( "Bad GUID Initialization..." );
			  }
			  if( cr == CR_BUFFER_SMALL ) {
				  // Buffer was too small, resize and try again.
				  propertyBuffer = ReallocateEx( propertyBuffer, propertyBufferSize DBG_SRC );
				  cr = CM_Get_Class_Property_ExW( &ComPortsClass, &DEVPKEY_NAME, &propType, (PBYTE)propertyBuffer
				                                , &propertyBufferSize, 0, 0 );
			  }

			  if( cr == CR_SUCCESS ) {
				  // DEVPKEY_NAME returns a DEVPROP_TYPE_STRING type.
				  if( propType == DEVPROP_TYPE_STRING ) {
					  //const wchar_t *className = reinterpret_cast<const wchar_t *>( propertyBuffer );
					  //lprintf( "  - %S  (GUID: {%S}) ", className, guidString );
					  //TEXTSTR t_className = WcharConvert( (const wchar_t *)className );
					  //lprintf( "(%s) (%s)", t_className, "Ports (COM & LPT)" );
					  if( 1 ) {// || StrCmp( t_className, "Ports (COM & LPT)" ) == 0 ) {
						  //lprintf( "So we should see what devices are there.." );
						  // GUID_DEVCLASS_PORTS
						  HDEVINFO hdi = SetupDiGetClassDevs( &classGuid, NULL, NULL, DIGCF_PRESENT );

						  // SetupDiCreateDeviceInfoList(&classGuid, NULL);
						  DWORD devId  = 0;
						  SP_DEVINFO_DATA data;
						  SP_DEVICE_INTERFACE_DATA intData;
						  data.cbSize = sizeof( data );
						  while( 1 ) {
							  if( !SetupDiEnumDeviceInfo( hdi, devId, &data ) )
								  break;
							  {
								  //lprintf( "Got a device... %d", data.DevInst );

								  HKEY hKey
								       = SetupDiOpenDevRegKey( hdi, &data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE );
								  DWORD dwError = GetLastError();
								  char namebuf[ 256 ];
								  DWORD namebuflen = 256;
								  int queryErr = RegQueryValueEx( hKey, "PortName", NULL, NULL, (LPBYTE)namebuf, &namebuflen );
								  namebuf[ namebuflen ] = 0;
								  //lprintf( "Device: %.*s", namebuflen, namebuf );

								  if( !port || !port[ 0 ] || StrCmp( port, namebuf ) == 0 ) {

									  if( !enable ) {
										  bool skip_disable = false;
										  ULONG ulStatus;
										  ULONG ulProblem;

										  // Get the status and problem code of the device
										  CONFIGRET cr = CM_Get_DevNode_Status( &ulStatus, &ulProblem, data.DevInst, 0 );

										  if( cr == CR_SUCCESS ) {
											  // Check if the device is disabled (problem code CM_PROB_DISABLED)
											  if( ulProblem == CM_PROB_DISABLED ) {
												  // was already disabled, disable won't give an event
												  // just do the enable.
												  //CONFIGRET r2 = CM_Enable_DevNode( data.DevInst, 0 );
												  skip_disable = true;
												  //lprintf( "Enable dev? %d", r2 );
												  
											  }
										  }

										  if( !skip_disable ) {
											  CONFIGRET r1 = CM_Disable_DevNode( data.DevInst, CM_DISABLE_UI_NOT_OK );
											  if( r1 == CR_REMOVE_VETOED ) {
												  lprintf( "Cannot remove the device." );
											  } else {
												  //lprintf( "Should this wait some time? %d", r1 );
											  }
										  }
									  } else {
										  // enabling an enabled node doesn't matter...
										  // no need to require that it's disabled...
										  // it's just that the disable is intended to be followed by an enable

										  CONFIGRET r2 = CM_Enable_DevNode( data.DevInst, 0 );
										  if( r2 )
											lprintf( "Device enable Failed? %d(%08x)", r2, r2 );
									  }
									  // found the match, done.
									  if( port && port[ 0 ] )
										  break;
								  }
							  }

							  devId++;
						  }
						  SetupDiDestroyDeviceInfoList( hdi );
					  }
				  }
			  } else {
				  lprintf( "CM_Get_Class_Property failed for a class with error: %d", cr );
			  }
		  }
		  /*
        } else if (cr == CR_NO_SUCH_VALUE) {
            // End of enumeration
          //  break;
        } else if (cr == CR_INVALID_DATA) {
            // Ignore invalid data entries and continue enumeration
            lprintf("  Warning: Invalid data encountered for class index %lu. Skipping.", classIndex);
        } else {
            lprintf("  Error enumerating classes: %lu", cr);
         //   break;
        }
			      */
        classIndex++;
	 };// while( TRUE );	


	//CONFIGRET r1 = CM_Disable_DevNode( inst, CM_DISABLE_UI_NOT_OK );
	//CONFIGRET r2 = CM_Enable_DevNode( inst, 0 );
	
}


void updateNames( void ) {
	int i;
	for( i = 0; pkey_table[ i ].name; i++ ) {
		char const *last = StrRChr( pkey_table[ i ].name, '_' );
		int spaces = 0;
		LOGICAL isCap    = FALSE;
		int ofs = 0;
		while( last[ ofs ] ) {
			if( last[ ofs ] >= 'A' && last[ ofs ] <= 'Z' ) {
				isCap = TRUE;
			} else if( last[ ofs ] >= 'a' && last[ ofs ] <= 'z' ) {
				if( isCap ) {
					// one space before the previous cap.
					spaces++;
					isCap = FALSE;
				}
			}
			ofs++;
		}
		char *fname = NewArray( char, ofs + spaces + 1 );
		int out     = 0;
		ofs         = 0;
		while( last[ ofs ] ) {
			if( last[ ofs ] >= 'A' && last[ ofs ] <= 'Z' ) {
				fname[ out++ ] = last[ofs];
				isCap          = TRUE;
			} else if( last[ ofs ] >= 'a' && last[ ofs ] <= 'z' ) {
				if( isCap ) {
					// one space before the previous cap.
					if( out > 1 ) {
						fname[ out ] = fname[ out - 1 ];
						fname[ out - 1 ] = ' ';
						out++;
					} 
					spaces++;
					isCap = FALSE;
				}
				fname[ out++ ] = last[ ofs ];
			}
			ofs++;
		}
		fname[ out++ ] = 0;
		pkey_table[ i ].niceName = fname;
	}
}


// Constant for the number of 100-nanosecond intervals between Jan 1, 1601 and Jan 1, 1970
// This value is 116444736000000000LL
const int64_t WINDOWS_TICK_TO_UNIX_EPOCH_OFFSET    = 116444736000000000LL;

// Constant for converting 100-nanosecond intervals to milliseconds
const int64_t WINDOWS_TICK_TO_MILLISECONDS_DIVISOR = 10000;

int64_t FiletimeToJavascriptTick( FILETIME ft ) {
	// Combine the high and low parts of the FILETIME into a 64-bit integer.
	ULARGE_INTEGER uli;
	uli.LowPart          = ft.dwLowDateTime;
	uli.HighPart         = ft.dwHighDateTime;

	// The result needs to be a signed 64-bit integer to handle the subtraction.
	int64_t windowsTicks = uli.QuadPart;

	// Adjust for the different epochs.
	int64_t unixTicks    = windowsTicks - WINDOWS_TICK_TO_UNIX_EPOCH_OFFSET;

	// Convert from 100-nanosecond intervals to milliseconds.
	int64_t jsTicks      = unixTicks / WINDOWS_TICK_TO_MILLISECONDS_DIVISOR;

	return jsTicks;
}

#else

void getPortProperties(ComObject *com, Isolate* isolate, Local<Object> result) {

}

#endif
