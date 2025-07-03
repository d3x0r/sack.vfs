

#include "../global.h"


class itemWrapper : public node::ObjectWrap {
 public:
	Persistent<Function> cb;
	Isolate *isolate;
	PLINKQUEUE events;
	INDEX id;
	LOGICAL checked = FALSE;
	CTEXTSTR text   = NULL;
	static Local<Object> New( Isolate *isolate, LOGICAL isConstructCall, Local<Object> _this );

	static void jsNew( const v8::FunctionCallbackInfo<Value> &args );
	Persistent<Object> jsObject;
	static void remove( const v8::FunctionCallbackInfo<Value> &args ) { lprintf( "Is there a delete?" );
	}

	static void setChecked2( const FunctionCallbackInfo<Value> &args ) {
		itemWrapper *wrapper = ObjectWrap::Unwrap<itemWrapper>( args.This() );
		if( args[0]->IsBoolean() ) {
			wrapper->checked = args[ 0 ].As<Boolean>()->BooleanValue( args.GetIsolate() );
			CheckSystrayMenuItem( wrapper->id, wrapper->checked );
			// MakeSystrayEvent( wrapper->isolate, Event_Systray_MenuFunction, wrapper );
		} else {
			args.GetIsolate()->ThrowException(
			     Exception::TypeError( String::NewFromUtf8Literal( args.GetIsolate(), "Checked must be a boolean" ) ) );
		}
	}
	static void getChecked2( const FunctionCallbackInfo<Value> &args ) {
		itemWrapper *wrapper = ObjectWrap::Unwrap<itemWrapper>( args.This() );
		if( !wrapper )
			args.GetReturnValue().Set( String::NewFromUtf8Literal( args.GetIsolate(), "invalid this passed to getter" ) );
		else
			args.GetReturnValue().Set( Boolean::New( args.GetIsolate(), wrapper->checked ) );
	}

	static void getText( const FunctionCallbackInfo<Value> &args ) {
		itemWrapper *wrapper = ObjectWrap::Unwrap<itemWrapper>( args.This() );
		if( !wrapper )
			args.GetReturnValue().Set( String::NewFromUtf8Literal( args.GetIsolate(), "invalid this passed to getter" ) );
		else
			args.GetReturnValue().Set( String::NewFromUtf8( wrapper->isolate, (const char*)wrapper->text ).ToLocalChecked() );
	}

	static void setText( const FunctionCallbackInfo<Value> &args ) {
		itemWrapper *wrapper = ObjectWrap::Unwrap<itemWrapper>( args.This() );
		if( args[ 0 ]->IsString() ) {
			String::Utf8Value text( USE_ISOLATE( wrapper->isolate ) args[ 0 ] );
			if( wrapper->text )
				Release( wrapper->text );
			wrapper->text = StrDup( *text );
			SetSystrayMenuItemText( wrapper->id, *text );
			// MakeSystrayEvent( wrapper->isolate, Event_Systray_MenuFunction, wrapper );
		} else {
			args.GetIsolate()->ThrowException(
			     Exception::TypeError( String::NewFromUtf8Literal( args.GetIsolate(), "Text must be a string" ) ) );
		}
	}

	static void setChecked( Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void> &args ) { 
		itemWrapper *wrapper = ObjectWrap::Unwrap<itemWrapper>( args.This() );
		if( value->IsBoolean() ) {
			wrapper->checked = value.As<Boolean>()->BooleanValue( args.GetIsolate() );
			CheckSystrayMenuItem( wrapper->id, wrapper->checked );
			//MakeSystrayEvent( wrapper->isolate, Event_Systray_MenuFunction, wrapper );
		} else {
			args.GetIsolate()->ThrowException(
			     Exception::TypeError( String::NewFromUtf8Literal( args.GetIsolate(), "Checked must be a boolean" ) ) );
		}
	}
	static void getChecked( Local<Name> property, const PropertyCallbackInfo<Value> &args ) {
		itemWrapper *wrapper = ObjectWrap::Unwrap<itemWrapper>( args.This() );
		if( !wrapper ) 
			args.GetReturnValue().Set( String::NewFromUtf8Literal( args.GetIsolate(), "invalid this passed to getter" ) );
		else
			args.GetReturnValue().Set( Boolean::New( args.GetIsolate(), wrapper->checked ) );
	}
};

class clickWrapper {
 public:
	Persistent<Function> cb;
	Isolate *isolate;
	PLINKQUEUE events;
};

enum systrayEvents {
	Event_Systray_DoubleClick,
	Event_Systray_MenuFunction,
	Event_Systray_Close_Loop,
	Event_Systray_,
};

struct systrayEvent {
	systrayEvents type;
	uintptr_t param;
};

typedef struct systrayEvent SS_EVENT;
#define MAXSS_EVENTSPERSET 64
DeclareSet( SS_EVENT );


static struct sytrayLocal {
	LOGICAL set;
	PTEXT icon;
	PLIST clicks;
	PLIST items;
	LOGICAL eventLoopRegistered;
	int eventLoopEnables;
	//uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback

	SS_EVENTSET event_pool;

} systrayLocal;

static uintptr_t MakeSystrayEvent( Isolate* isolate, enum systrayEvents type, ... );


static void asyncmsg( uv_async_t* handle ) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	systrayEvent* evt = NULL;
	INDEX idx;
	clickWrapper * wrapper; 
	itemWrapper *cbWrapper = NULL;
	do {
		evt = NULL;
		LIST_FORALL( systrayLocal.clicks, idx, clickWrapper *, wrapper ) {
			if( wrapper->isolate == isolate ) {
				evt = (struct systrayEvent *)DequeLink( &wrapper->events );
				if( evt )
					break;
			}
		}
		if( !evt ) {
			LIST_FORALL( systrayLocal.items, idx, itemWrapper *, cbWrapper ) {
				if( cbWrapper->isolate == isolate ) {
					evt = (struct systrayEvent *)DequeLink( &cbWrapper->events );
					if( evt )
						break;
				}
			}
		}
		if( !evt )
			break;
	
		switch( evt->type ) {
		case Event_Systray_DoubleClick:
			if( wrapper ) {
				if( !wrapper->cb.IsEmpty() ) {
					Local<Function> f = wrapper->cb.Get( isolate );
					f->Call( context, context->Global(), 0, NULL );
				}
			}
			break;
		case Event_Systray_MenuFunction:
			if( cbWrapper ) {
				if( !cbWrapper->cb.IsEmpty() ) {
					Local<Function> f = cbWrapper->cb.Get( isolate );
					f->Call( context, context->Global(), 0, NULL );
				}
			}
			break;
		}
		DeleteFromSet( SS_EVENT, &systrayLocal.event_pool, evt );
	}
	while( evt );

	{
		class constructorSet* c = getConstructors( isolate );
		if( !c->ThreadObject_idleProc.IsEmpty() ) {
			Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
			cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
		}
	}
}

static void enableEventLoop( Isolate *isolate ) {
	class constructorSet *c = getConstructors( isolate );
	if( !c->systrayAsync.data ){
		c->systrayAsync.data = (POINTER)1;
		uv_async_init( uv_default_loop(), &c->systrayAsync, asyncmsg );
		uv_unref( (uv_handle_t *)&c->systrayAsync );
	}
}

#if 0
static void disableEventLoop( void ) {
	if( systrayLocal.eventLoopRegistered ) {
		if( !(--systrayLocal.eventLoopEnables) ) {
			systrayLocal.eventLoopRegistered = FALSE;
			MakeSystrayEvent( Event_Systray_Close_Loop );
		}
	}
}
#endif

static uintptr_t MakeSystrayEvent( Isolate *isolate, enum systrayEvents type, ... ) {
	systrayEvent* pe;
#define e (*pe)
	va_list args;
	va_start( args, type );
	if( type != Event_Systray_Close_Loop )
		enableEventLoop( isolate );
	pe = GetFromSet( SS_EVENT, &systrayLocal.event_pool );
	e.type = type;
	//e.registration = control->reg
	pe->param = va_arg( args, uintptr_t );
	switch( type ) {
	case Event_Systray_DoubleClick:
		EnqueLink( &((clickWrapper *)pe->param)->events, pe );
		break;
	case Event_Systray_Close_Loop:

		break;
	case Event_Systray_MenuFunction:
		EnqueLink( &( (itemWrapper *)pe->param )->events, pe );
		break;
	}
	constructorSet *c = getConstructors( isolate );
	uv_async_send( &c->systrayAsync );
	return 0;
}


static void CPROC menuCallback( uintptr_t p) {
	MakeSystrayEvent( ((itemWrapper*)p)->isolate, Event_Systray_MenuFunction, p );
}

static void CPROC doubleClickCallback( uintptr_t psv ) {
	clickWrapper *wrapper = (clickWrapper *)psv; 
	MakeSystrayEvent( wrapper->isolate, Event_Systray_DoubleClick, psv );
}

void setSystrayIcon( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() > 0 ) {
		if( args.Length() > 1 ) {
			class constructorSet *c = getConstructors( isolate );
			Local<Function> cons    = c->systrayItemConstructor.Get( isolate );
			Local<Object> item      = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
			itemWrapper *wrapper    = node::ObjectWrap::Unwrap<itemWrapper>( item );
			Local<Function> cb      = Local<Function>::Cast( args[ 1 ] );
			wrapper->isolate               = isolate;
			wrapper->events         = CreateLinkQueue();
			wrapper->cb.Reset( isolate, cb );
			AddLink( &systrayLocal.clicks, wrapper );
			SetIconDoubleClick_v2( doubleClickCallback, (uintptr_t)wrapper );
		}
		String::Utf8Value fName( USE_ISOLATE( isolate ) args[ 0 ] );
		if( systrayLocal.set ) {
			ChangeIcon( *fName );
		} else {
			enableEventLoop( isolate );
			RegisterIcon( *fName );
		}
		systrayLocal.set = 1;
	}
}


void setSystrayIconMenu( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() > 1 ) {
		Local<Function> cb = Local<Function>::Cast( args[1] );
		class constructorSet *c = getConstructors( isolate );
		Local<Function> cons                     = c->systrayItemConstructor.Get( isolate );
		Local<Object> item = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
		itemWrapper * wrapper = node::ObjectWrap::Unwrap<itemWrapper>( item );

		wrapper->isolate               = isolate;
		wrapper->cb.Reset( isolate, cb );
		AddLink( &systrayLocal.items, wrapper );
		if( args[ 0 ]->IsString() ) {
			String::Utf8Value fName( USE_ISOLATE( isolate ) args[ 0 ] );
			wrapper->id = AddSystrayMenuFunction_v2( *fName, menuCallback, (uintptr_t)wrapper );
		}
		args.GetReturnValue().Set( item );
	}
}

void InitSystray( Isolate* isolate, Local<Object> _exports ) {
	//Local<Context> context = isolate->GetCurrentContext();

	Local<Object> systrayObject = Object::New( isolate );
	SET_READONLY_METHOD( systrayObject, "set", setSystrayIcon );
	SET_READONLY_METHOD( systrayObject, "on", setSystrayIconMenu );

	SET_READONLY( _exports, "Systray", systrayObject );

	Local<FunctionTemplate> itemTemplate;

	itemTemplate = FunctionTemplate::New( isolate, itemWrapper::jsNew );
	itemTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.systray.item" ) );
	itemTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( itemTemplate, "remove", itemWrapper::remove );
	itemTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "checked" )
	                                                      , FunctionTemplate::New( isolate, itemWrapper::getChecked2 )
	                                                      , FunctionTemplate::New( isolate, itemWrapper::setChecked2 ) );
	itemTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "text" )
	                                                      , FunctionTemplate::New( isolate, itemWrapper::getText )
	                                                      , FunctionTemplate::New( isolate, itemWrapper::setText ) );
#if 0
#if ( NODE_MAJOR_VERSION >= 22 )
	itemTemplate->PrototypeTemplate()->SetNativeDataProperty(
	     String::NewFromUtf8Literal( isolate, "checked" ), itemWrapper::getChecked, itemWrapper::setChecked
	     , Local<Value>()
	     , PropertyAttribute::None, SideEffectType::kHasNoSideEffect, SideEffectType::kHasSideEffect );
#elif ( NODE_MAJOR_VERSION >= 18 )
	itemTemplate->PrototypeTemplate()->SetNativeDataProperty(
	     String::NewFromUtf8Literal( isolate, "checked" ), itemWrapper::getChecked, itemWrapper::setChecked
	     , Local<Value>()
	     , PropertyAttribute::None, AccessControl::DEFAULT, SideEffectType::kHasNoSideEffect
	     , SideEffectType::kHasSideEffect );
#else
#endif
#endif
	class constructorSet *c = getConstructors( isolate );
	Local<Function> itemFunc = itemTemplate->GetFunction( isolate->GetCurrentContext() ).ToLocalChecked();
	c->systrayItemConstructor.Reset( isolate, itemFunc );
//	SET( exports, "ComPort", ComFunc );
}


void itemWrapper::jsNew( const v8::FunctionCallbackInfo<Value> &args ) {

	itemWrapper::New( args.GetIsolate(), args.IsConstructCall(), args.This() );
	args.GetReturnValue().Set( args.This() );
}

Local<Object> itemWrapper::New( Isolate *isolate, LOGICAL isConstructCall, Local<Object> _this ) {
	class constructorSet *c = getConstructors( isolate );
	if( isConstructCall ) {
		// Invoked as constructor: `new MyObject(...)`
		itemWrapper *obj = new itemWrapper( );
		obj->Wrap( _this );
		obj->jsObject.Reset( isolate, _this );
		return _this;
	} else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Value> *argv = new Local<Value>[ 0 ];
		Local<Function> cons       = Local<Function>::New( isolate, c->comConstructor );
		Local<Object> newInst = cons->NewInstance( isolate->GetCurrentContext(), 0, argv ).ToLocalChecked();
		delete[] argv;
		return newInst;
	}
}
