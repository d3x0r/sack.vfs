
#include "../global.h"

#include <psi.h>
#include <psi/console.h>
#include <psi/clock.h>

static RegistrationObject *findRegistration( CTEXTSTR name );

static RegistrationObject _blankRegistration;
static ControlObject _blankObject;
static ListboxItemObject _blankListbox;
static MenuItemObject _blankMenuItem;
static PopupObject _blankPopup;
static VoidObject _blankVoid;

static struct psiLocal {
	PLIST registrations;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE events;
	PTHREAD nodeThread;
	ControlObject *pendingCreate;
	Local<Object> newControl;
	RegistrationObject *pendingRegistration;
	int eventLoopEnables;
	LOGICAL eventLoopRegistered;
	PLIST controls;
	LOGICAL internalCreate;
	IS_EVENTSET event_pool;
	int bindingDataId;
} psiLocal;

Persistent<FunctionTemplate> ControlObject::frameTemplate;
Persistent<FunctionTemplate> ControlObject::controlTemplate;
Persistent<Function> ControlObject::constructor;
Persistent<Function> ControlObject::constructor2;
Persistent<Function> ControlObject::registrationConstructor;
Persistent<Function> PopupObject::constructor;
Persistent<Function> ListboxItemObject::constructor;
Persistent<Function> MenuItemObject::constructor;
Persistent<Function> VoidObject::constructor;
Persistent<Function> VoidObject::constructor2;

struct optionStrings {
	Isolate *isolate;

	Eternal<String> *nameString;
	Eternal<String> *widthString;
	Eternal<String> *heightString;
	Eternal<String> *borderString;
	Eternal<String> *createString;
	Eternal<String> *drawString;
	Eternal<String> *mouseString;
	Eternal<String> *keyString;
	Eternal<String> *destroyString;
	Eternal<String> *xString;
	Eternal<String> *yString;
	Eternal<String> *wString;
	Eternal<String> *hString;
	Eternal<String> *sizeString;
	Eternal<String> *posString;
	Eternal<String> *layoutString;
	Eternal<String> *indexString;
	Eternal<String> *objectString;
	Eternal<String> *imageString;
	Eternal<String> *anchorsString;
	Eternal<String> *definesColorsString;
};


static struct optionStrings *getStrings( Isolate *isolate ) {
	static PLIST strings;
	INDEX idx;
	struct optionStrings * check;
	LIST_FORALL( strings, idx, struct optionStrings *, check ) {
		if( check->isolate == isolate )
			break;
	}
	if( !check ) {
		check = NewArray( struct optionStrings, 1 );
		check->isolate = isolate;
#define makeString(a,b) check->a##String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, b ) );
		check->nameString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "name" ) );
		check->widthString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "width" ) );
		check->heightString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "height" ) );
		check->borderString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "border" ) );
		check->createString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "create" ) );
		check->mouseString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "mouse" ) );
		check->drawString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "draw" ) );
		check->keyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "key" ) );
		check->destroyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "destroy" ) );
		makeString( x, "x" );
		makeString( y, "y" );
		makeString( w, "width" );
		makeString( h, "height" );
		makeString( pos, "position" );
		makeString( size, "size" );
		makeString( layout, "layout" );
		makeString( index, "index" );
		makeString( object, "object" );
		makeString( image, "image" );
		makeString( anchors, "anchors" );
		makeString( definesColors, "definesColors" );

		//check->String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "" ) );
		AddLink( &strings, check );
	}
	return check;
}


static void asyncmsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	
	RenderObject* myself = (RenderObject*)handle->data;

	HandleScope scope(isolate);
	//lprintf( "async message notice. %p", myself );
	{
		struct event *evt;

		while( evt = (struct event *)DequeLink( &psiLocal.events ) ) {

			//Local<Value> object = ProcessEvent( isolate, evt, myself );
			//Local<Value> argv[] = { Local<Object>::New( isolate, evt->data.control.control->state ) };
			Local<Function> cb;
			Local<Value> r;
			switch( evt->type ){
			case Event_Frame_Ok:
			{
				cb = Local<Function>::New( isolate, evt->control->cbFrameEventOkay );
				cb->Call( evt->control->state.Get( isolate ), 0, NULL );
				break;
			}
			case Event_Frame_Cancel:
			{
				cb = Local<Function>::New( isolate, evt->control->cbFrameEventCancel );
				cb->Call( evt->control->state.Get( isolate ), 0, NULL );
				break;
			}
			case Event_Frame_Abort:
			{
				cb = Local<Function>::New( isolate, evt->control->cbFrameEventAbort );
				cb->Call( evt->control->state.Get( isolate ), 0, NULL );
				break;
			}
			case Event_Control_Create:
			{
				Isolate* isolate = Isolate::GetCurrent();
				if( psiLocal.pendingRegistration ) {

					Local<Object> object = ControlObject::NewWrappedControl( isolate, evt->data.pc );
					ControlObject *control = ControlObject::Unwrap<ControlObject>( object );
					control->registration = psiLocal.pendingRegistration;
					ControlObject **me = ControlData( ControlObject **, evt->data.pc );
					if( me )
						me[0] = control;

					Local<Function> cb = Local<Function>::New( isolate, psiLocal.pendingRegistration->cbInitEvent );
					// controls get wrapped sooner... 
					// ControlObject::wrapSelf( isolate, me[0], psiLocal.newControl );
					
					Local<Value> retval = cb->Call( object, 0, NULL );

					evt->success = retval->ToInt32( USE_ISOLATE_VOID( isolate ) )->Value();
				}
				else {
					Local<Object> object = ControlObject::NewWrappedControl( isolate, evt->data.pc );
					ControlObject *control = ControlObject::Unwrap<ControlObject>( object );
					if( control->registration ) {
						Local<Function> cb = Local<Function>::New( isolate, control->registration->cbInitEvent );
						Local<Value> retval = cb->Call( psiLocal.newControl, 0, NULL );
					}
					AddLink( &psiLocal.controls, control );
					evt->control = control;
				}

			}
				break;
			case Event_Control_Destroy:
				ControlObject::releaseSelf( evt->control );
				break;
			case Event_Control_Draw: {
				if( evt->control ) {
					if( !evt->control->image ) {
						evt->control->image = ImageObject::MakeNewImage( isolate, GetControlSurface( evt->control->control ), TRUE );
					}
					Local<Value> argv[1] = { evt->control->image->_this.Get( isolate ) };
					cb = Local<Function>::New( isolate, evt->control->registration->cbDrawEvent );
					r = cb->Call( evt->control->state.Get( isolate ), 1, argv );
					if( !r.IsEmpty() )
						evt->success = (int)r->IntegerValue();
					else
						evt->success = 1;
				}
				else evt->success = 0;
				break;
			}
			case Event_Control_Mouse: {
				//evt->data.controlMouse.target
				Local<Object> jsEvent = Object::New( isolate );
				jsEvent->Set( String::NewFromUtf8( isolate, "x" ), Integer::New( isolate, evt->data.mouse.x ) );
				jsEvent->Set( String::NewFromUtf8( isolate, "y" ), Integer::New( isolate, evt->data.mouse.y ) );
				jsEvent->Set( String::NewFromUtf8( isolate, "b" ), Integer::New( isolate, evt->data.mouse.b ) );
				Local<Value> argv[1] = {jsEvent };
				cb = Local<Function>::New( isolate, evt->control->registration->cbMouseEvent );
				r = cb->Call( evt->control->state.Get( isolate ), 1, argv );
				if( !r.IsEmpty() )
					evt->success = (int)r->IntegerValue();
				else
					evt->success = 1;
				break;
			}
			case Event_Control_ButtonClick:
				cb = Local<Function>::New( isolate, evt->control->customEvents[0] );
				r = cb->Call( evt->control->state.Get( isolate ), 0, NULL );

				break;
			case Event_Control_ConsoleInput: {
				cb = Local<Function>::New( isolate, evt->control->customEvents[0] );
				Local<Value> argv[1] = { String::NewFromUtf8( isolate, GetText( evt->data.console.text ) ) };
				r = cb->Call( evt->control->state.Get( isolate ), 1, argv );
				break;
			}
			case Event_Control_Resize: {
				cb = Local<Function>::New( isolate, evt->control->cbSizeEvent );
				Local<Value> argv[3] = { Number::New(isolate, evt->data.size.w )
					, Number::New( isolate, evt->data.size.h )
					, Boolean::New( isolate, evt->data.move.start ) };
				r = cb->Call( evt->control->state.Get( isolate ), 2, argv );
				break;
			}
			case Event_Control_Move: {
				cb = Local<Function>::New( isolate, evt->control->cbMoveEvent );
				Local<Value> argv[3] = { Number::New( isolate, evt->data.move.x )
					, Number::New( isolate, evt->data.move.y )
					, Boolean::New( isolate, evt->data.move.start ) };
				r = cb->Call( evt->control->state.Get( isolate ), 2, argv );
				break;
			}
			case Event_Control_Close_Loop:
				uv_close( (uv_handle_t*)&psiLocal.async, NULL );
				break;
			case Event_Listbox_Selected:
				cb = Local<Function>::New( isolate, evt->control->listboxOnSelect );
				//Local<Value> argv[1] = { String::NewFromUtf8( isolate, GetText( evt->data.console.text ) ) };
				{
					ListboxItemObject *lio = (ListboxItemObject *)evt->data.listbox.pli;
					r = cb->Call( lio->_this.Get(isolate), 0, NULL );
				}
				break;
			case Event_Listbox_DoubleClick:
				cb = Local<Function>::New( isolate, evt->control->listboxOnDouble );
				{
					ListboxItemObject *lio = (ListboxItemObject *)evt->data.listbox.pli;
					r = cb->Call( lio->_this.Get( isolate ), 0, NULL );
				}
				break;
			case Event_Menu_Item_Selected:
				MenuItemObject *mio = (MenuItemObject *)evt->data.popup.pmi;
				cb = Local<Function>::New( isolate, mio->cbSelected );
				r = cb->Call( mio->_this.Get(isolate), 0, NULL );
				break;
			}
			if( evt->waiter ) {
				evt->flags.complete = TRUE;
				WakeThread( evt->waiter );
			}
			DeleteFromSet( IS_EVENT, &psiLocal.event_pool, evt );
		}
	}
	//lprintf( "done calling message notice." );
}


void enableEventLoop( void ) {
	if( !psiLocal.eventLoopRegistered ) {
		psiLocal.eventLoopRegistered = TRUE;
		MemSet( &psiLocal.async, 0, sizeof( &psiLocal.async ) );
		uv_async_init( uv_default_loop(), &psiLocal.async, asyncmsg );
	}
	psiLocal.eventLoopEnables++;
}


static uintptr_t MakePSIEvent( ControlObject *control, bool block, enum eventType type, ... ) {
	event *pe;
#define e (*pe)
	va_list args;
	va_start( args, type );
	if( type != Event_Control_Close_Loop )
		enableEventLoop();
	pe = GetFromSet( IS_EVENT, &psiLocal.event_pool );
	e.type = type;
	e.control = control;
	//e.registration = control->reg
	switch( type ) {
	case Event_Control_Close_Loop:
		break;
	case Event_Control_Create:
		e.data.pc = va_arg( args, PSI_CONTROL );
		break;
	case Event_Control_Mouse:
		e.data.mouse.x = va_arg( args, int32_t );
		e.data.mouse.y = va_arg( args, int32_t );
		e.data.mouse.b = va_arg( args, uint32_t );
		break;
	case Event_Control_Draw:

		break;
	case Event_Control_Key:
		e.data.key.code = va_arg( args, uint32_t );
		break;
	case Event_Control_ConsoleInput:
		e.data.console.text = va_arg( args, PTEXT );
		break;
	case Event_Listbox_Selected:
	case Event_Listbox_DoubleClick:
		e.data.listbox.pli = va_arg( args, uintptr_t );
		break;
	case Event_Menu_Item_Selected:
		e.data.popup.pmi = va_arg( args, uintptr_t );
		break;
	case Event_Control_Move:
		if( control->cbMoveEvent.IsEmpty() )
			return false;
		e.data.move.x = va_arg( args, int32_t );
		e.data.move.y = va_arg( args, int32_t );
		e.data.move.start = va_arg( args, LOGICAL );
		break;
	case Event_Control_Resize:
		if( control->cbSizeEvent.IsEmpty() )
			return false;
		e.data.size.w = va_arg( args, uint32_t );
		e.data.size.h = va_arg( args, uint32_t );
		e.data.size.start = va_arg( args, LOGICAL );
		break;
	}

	e.waiter = MakeThread();
	e.flags.complete = 0; 
	e.success = 0;
	EnqueLink( &psiLocal.events, &e );
	uv_async_send( &psiLocal.async );
	if( block )
		while( !e.flags.complete ) WakeableSleep( 1000 );

	return e.success;
#undef e
}

void disableEventLoop( void ) {
	if( psiLocal.eventLoopRegistered ) {
		if( !(--psiLocal.eventLoopEnables) ) {
			psiLocal.eventLoopRegistered = FALSE;
			MakePSIEvent( NULL, false, Event_Control_Close_Loop );
		}
	}
}


VoidObject::VoidObject( uintptr_t data ) {
	memcpy( this, &_blankVoid, sizeof( *this ) );
	this->data = data;
}
void VoidObject::Init( Isolate *isolate ) {
	Local<FunctionTemplate> voidTemplate;
	voidTemplate = FunctionTemplate::New( isolate, New );
	voidTemplate->SetClassName( String::NewFromUtf8( isolate, "void*" ) );
	voidTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 internal field for wrap
	constructor.Reset( isolate, voidTemplate->GetFunction() );
}
void VoidObject::New( const FunctionCallbackInfo<Value>& args ) {
	if( args.IsConstructCall() ) {
		Isolate* isolate = args.GetIsolate();
		VoidObject* obj = new VoidObject();
		VoidObject::wrapSelf( isolate, obj, args.This() );
		args.GetReturnValue().Set( args.This() );
	}
	else {
		// void object is always called with constructor....
	}
}
void VoidObject::wrapSelf( Isolate* isolate, VoidObject *_this, Local<Object> into ) {
	_this->Wrap( into );
	_this->_this.Reset( isolate, into );
}
void VoidObject::releaseSelf( VoidObject *_this ) {
	_this->_this.Reset();
}



static void newBorder( const FunctionCallbackInfo<Value>& args ) {
	if( args.Length() == 1 && args[0]->IsObject() ) {
		Local<Object> config = Handle<Object>::Cast( args[0] );
		Isolate* isolate = args.GetIsolate();
		Local<Function> cons = Local<Function>::New( isolate, VoidObject::constructor );
		Local<Object> borderObj = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
		VoidObject *v = VoidObject::Unwrap<VoidObject>( borderObj );
		Image image;
		int32_t width, height;
		int32_t anchors;
		int definesColors;
		struct optionStrings *strings = getStrings( isolate );
		Local<String> optString;
		if( config->Has( optString = strings->imageString->Get( isolate ) ) ) {
			String::Utf8Value filename( config->Get( optString ) );
			image = LoadImageFile( *filename );
		}
		if( config->Has( optString = strings->wString->Get( isolate ) ) ) {
			width = (int)config->Get( optString )->IntegerValue();
		}
		if( config->Has( optString = strings->hString->Get( isolate ) ) ) {
			height = (int)config->Get( optString )->IntegerValue();
		}
		if( config->Has( optString = strings->anchorsString->Get( isolate ) ) ) {
			anchors = (int)config->Get( optString )->IntegerValue();
		}
		if( config->Has( optString = strings->definesColorsString->Get( isolate ) ) ) {
			definesColors = config->Get( optString )->BooleanValue();
		}
		v->data = (uintptr_t)PSI_CreateBorder( image, width, height, anchors, definesColors );
		args.GetReturnValue().Set( borderObj );
	}
	if( args.Length() == 2 && args[0]->IsFunction() && args[1]->IsFunction() ) {
		//PSI_SetCustomBorder( 
		//args.GetReturnValue().Set( borderObj );
	}
}


static int CPROC CustomDefaultInit( PSI_CONTROL pc ) {
	if( psiLocal.pendingCreate ) return 1; // internal create in progress... it will result with its own object later.
	Isolate* isolate = Isolate::GetCurrent();
	if( !isolate ) {
		MakePSIEvent( NULL, false, Event_Control_Create, pc );
	} else {
		Local<Object> object = ControlObject::NewWrappedControl( isolate, pc );
		ControlObject *control = ControlObject::Unwrap<ControlObject>( object );
		AddLink( &psiLocal.controls, control );
	}
	return 1;
}

//--------------------------------------------------------------------------
static int CPROC CustomDefaultDestroy( PSI_CONTROL pc ) {
	ControlObject *control;
	INDEX idx;
	LIST_FORALL( psiLocal.controls, idx, ControlObject *, control ) {
		if( control->control == pc ) {
			SetLink( &psiLocal.controls, idx, NULL );
			//DeleteControlColors( control->state.Get() );
			Isolate* isolate = Isolate::GetCurrent();
			if( isolate )
				ControlObject::releaseSelf( control );
			else
				MakePSIEvent( control, false, Event_Control_Destroy );
			break;
		}
	}
	return 1;
}


void SetupControlColors( Isolate *isolate, Local<Object> object ) {
	Local<Object> controlColors = Object::New( isolate );
	struct optionStrings *strings = getStrings( isolate );

#define setupAccessor( name, define ) {   \
	Local<Object> arg = Object::New( isolate );\
	arg->Set( strings->indexString->Get( isolate ), Integer::New( isolate, define ) );\
	arg->Set( strings->objectString->Get( isolate ), object );\
	controlColors->SetAccessorProperty( String::NewFromUtf8( isolate, name )\
		, Function::New( isolate, ControlObject::getControlColor, arg ), Function::New( isolate, ControlObject::setControlColor, arg ), DontDelete );\
	}

	//controlColors->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8( isolate, name ), data, ReadOnlyProperty )
	setupAccessor( "highlight", HIGHLIGHT );
	setupAccessor( "normal" , NORMAL);
	setupAccessor( "shade" , SHADE);
	setupAccessor( "shadow" , SHADOW);
	setupAccessor( "textColor" , TEXTCOLOR);
	setupAccessor( "caption", CAPTION);
	setupAccessor( "captionText", CAPTIONTEXTCOLOR);
	setupAccessor( "inactiveCaption", INACTIVECAPTION);
	setupAccessor( "InactiveCaptionText", INACTIVECAPTIONTEXTCOLOR);
	setupAccessor( "selectBack", SELECT_BACK);
	setupAccessor( "selectText", SELECT_TEXT);
	setupAccessor( "editBackground", EDIT_BACKGROUND);
	setupAccessor( "editText", EDIT_TEXT);
	setupAccessor( "scrollBarBackground", SCROLLBAR_BACK);

	SET_READONLY( object, "color", controlColors );

}



void MakeControlColors( Isolate *isolate, Local<FunctionTemplate> tpl ) {
	//Local<Object> controlColors = Object::New( isolate );
	//struct optionStrings *strings = getStrings( isolate );

#define makeAccessor(a,b,c,d,e) a->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, b ) \
		, FunctionTemplate::New( isolate, c, Integer::New( isolate, e ) ) \
			, FunctionTemplate::New( isolate, d, Integer::New( isolate, e ) ) \
			, DontDelete )
#define makeColorAccessor(a,b,c) makeAccessor( a, b, ControlObject::getControlColor2, ControlObject::setControlColor2, c )

	//controlColors->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8( isolate, name ), data, ReadOnlyProperty )
	makeColorAccessor( tpl, "highlight", HIGHLIGHT );
	makeColorAccessor( tpl, "normal", NORMAL );
	makeColorAccessor( tpl, "shade", SHADE );
	makeColorAccessor( tpl, "shadow", SHADOW );
	makeColorAccessor( tpl, "textColor", TEXTCOLOR );
	makeColorAccessor( tpl, "caption", CAPTION );
	makeColorAccessor( tpl, "captionText", CAPTIONTEXTCOLOR );
	makeColorAccessor( tpl, "inactiveCaption", INACTIVECAPTION );
	makeColorAccessor( tpl, "InactiveCaptionText", INACTIVECAPTIONTEXTCOLOR );
	makeColorAccessor( tpl, "selectBack", SELECT_BACK );
	makeColorAccessor( tpl, "selectText", SELECT_TEXT );
	makeColorAccessor( tpl, "editBackground", EDIT_BACKGROUND );
	makeColorAccessor( tpl, "editText", EDIT_TEXT );
	makeColorAccessor( tpl, "scrollBarBackground", SCROLLBAR_BACK );
#undef makeColorAccessor
#undef makeAccessor
}

void AddControlColors( Isolate *isolate, Local<Object> control, ControlObject *c ) {

	Local<Function> cons = Local<Function>::New( isolate, VoidObject::constructor2 );
	Local<Object> colors = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
	VoidObject *v = VoidObject::Unwrap<VoidObject>( colors );
	v->data = (uintptr_t)c;
	SET_READONLY( control, "color", colors );
}

void DeleteControlColors( Isolate *isolate, Local<Object> control, ControlObject *c ) {
	Local<Object> colors = control->Get( String::NewFromUtf8( isolate, "color" ) )->ToObject();

	VoidObject *v = VoidObject::Unwrap<VoidObject>( colors );
	VoidObject::releaseSelf( v );
}

void ControlObject::Init( Handle<Object> _exports ) {
	psiLocal.bindingDataId = PSI_AddBindingData( "Node" );

		Isolate* isolate = Isolate::GetCurrent();
		Local<FunctionTemplate> psiTemplate;
		Local<FunctionTemplate> psiTemplate2;
		Local<FunctionTemplate> psiTemplatePopups;
		Local<FunctionTemplate> borderTemplate;
		Local<FunctionTemplate> regTemplate;
		Local<FunctionTemplate> listItemTemplate;
		Local<FunctionTemplate> menuItemTemplate;
		Local<FunctionTemplate> voidTemplate;
		Local<FunctionTemplate> colorTemplate;
		Handle<Object> exports = Object::New( isolate );

		VoidObject::Init( isolate );
		VulkanObject::Init( isolate, _exports );

		SimpleRegisterMethod( WIDE( "psi/control/rtti/extra init" )
			, CustomDefaultInit, WIDE( "int" ), WIDE( "sack-gui init" ), WIDE( "(PCOMMON)" ) );
		SimpleRegisterMethod( WIDE( "psi/control/rtti/extra destroy" )
			, CustomDefaultDestroy, WIDE( "int" ), WIDE( "sack-gui destroy" ), WIDE( "(PCOMMON)" ) );

		SetControlImageInterface( g.pii );
		SetControlInterface( g.pdi );

		_exports->Set( String::NewFromUtf8( isolate, "PSI" ), exports );
		// Prepare constructor template
		psiTemplate = FunctionTemplate::New( isolate, New );
		frameTemplate.Reset( isolate, psiTemplate );
		psiTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Frame" ) );
		psiTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 internal field for wrap

																	 // Prepare constructor template
		colorTemplate = FunctionTemplate::New( isolate, VoidObject::New );
		colorTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Frame.Colors" ) );
		colorTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 internal field for wrap
		MakeControlColors( isolate, colorTemplate );
		VoidObject::constructor2.Reset( isolate, colorTemplate->GetFunction() );


				// Prepare constructor template
		psiTemplate2 = FunctionTemplate::New( isolate, NewControl );
		controlTemplate.Reset( isolate, psiTemplate2 );
		psiTemplate2->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Control" ) );
		psiTemplate2->InstanceTemplate()->SetInternalFieldCount( 1 );// 1 internal field for wrap

		// Prepare constructor template
		psiTemplatePopups = FunctionTemplate::New( isolate, PopupObject::NewPopup );
		psiTemplatePopups->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Popup" ) );
		psiTemplatePopups->InstanceTemplate()->SetInternalFieldCount( 1 );// 1 internal field for wrap

		// Prepare constructor template
		borderTemplate = FunctionTemplate::New( isolate, newBorder );
		borderTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Frame.Border" ) );
		borderTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );// 1 internal field for wrap


		// Prepare constructor template
		regTemplate = FunctionTemplate::New( isolate, RegistrationObject::NewRegistration );
		regTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Registration" ) );
		regTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );// 1 internal field for wrap


		// Prepare constructor template
		listItemTemplate = FunctionTemplate::New( isolate, ListboxItemObject::New );
		listItemTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Listbox.Item" ) );
		listItemTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 internal field for wrap
		ListboxItemObject::constructor.Reset( isolate, listItemTemplate->GetFunction() );

		// Prepare constructor template
		menuItemTemplate = FunctionTemplate::New( isolate, MenuItemObject::New );
		menuItemTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Menu.Item" ) );
		menuItemTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 internal field for wrap
		MenuItemObject::constructor.Reset( isolate, menuItemTemplate->GetFunction() );

		Local<Object> buttonEnum = Object::New( isolate );
		SET_READONLY( buttonEnum, "left", Integer::New( isolate, 1 ) );
		SET_READONLY( buttonEnum, "right", Integer::New( isolate, 2 ) );
		SET_READONLY( buttonEnum, "middle", Integer::New( isolate, 16 ) );
		SET_READONLY( buttonEnum, "scroll_down", Integer::New( isolate, 256 ) );
		SET_READONLY( buttonEnum, "scroll_up", Integer::New( isolate, 512 ) );
		SET_READONLY( exports, "button", buttonEnum );

		Local<Object> controlObject = Object::New( isolate );
		SET_READONLY( exports, "control", controlObject );

		Local<Object> borderEnum = Object::New( isolate );
		SET_READONLY( borderEnum, "normal", Integer::New( isolate, BORDER_NORMAL ) );
		SET_READONLY( borderEnum, "none", Integer::New( isolate, BORDER_NONE ) );
		SET_READONLY( borderEnum, "thin", Integer::New( isolate, BORDER_THIN ) );
		SET_READONLY( borderEnum, "thinner", Integer::New( isolate, BORDER_THINNER ) );
		SET_READONLY( borderEnum, "dent", Integer::New( isolate, BORDER_DENT ) );
		SET_READONLY( borderEnum, "thinDent", Integer::New( isolate, BORDER_THIN_DENT ) );
		SET_READONLY( borderEnum, "thickDent", Integer::New( isolate, BORDER_THICK_DENT ) );
		SET_READONLY( borderEnum, "user", Integer::New( isolate, BORDER_USER_PROC ) );
		SET_READONLY( borderEnum, "invert", Integer::New( isolate, BORDER_INVERT ) );
		SET_READONLY( borderEnum, "invertThinner", Integer::New( isolate, BORDER_INVERT_THINNER ) );
		SET_READONLY( borderEnum, "invertThin", Integer::New( isolate, BORDER_INVERT_THIN ) );
		SET_READONLY( borderEnum, "bump", Integer::New( isolate, BORDER_BUMP ) );
		SET_READONLY( borderEnum, "caption", Integer::New( isolate, BORDER_CAPTION ) );
		SET_READONLY( borderEnum, "noCaption", Integer::New( isolate, BORDER_NOCAPTION ) );
		SET_READONLY( borderEnum, "noMove", Integer::New( isolate, BORDER_NOMOVE ) );
		SET_READONLY( borderEnum, "close", Integer::New( isolate, BORDER_CLOSE ) );
		SET_READONLY( borderEnum, "resizable", Integer::New( isolate, BORDER_RESIZABLE ) );
		SET_READONLY( borderEnum, "within", Integer::New( isolate, BORDER_WITHIN ) );
		SET_READONLY( borderEnum, "wantMouse", Integer::New( isolate, BORDER_WANTMOUSE ) );
		SET_READONLY( borderEnum, "exclusive", Integer::New( isolate, BORDER_EXCLUSIVE ) );
		//SET_READONLY( borderEnum, "noMove", Integer::New( isolate, BORDER_NOMOVE ) );
		SET_READONLY( borderEnum, "fixed", Integer::New( isolate, BORDER_FIXED ) );
		SET_READONLY( borderEnum, "noExtraInit", Integer::New( isolate, BORDER_NO_EXTRA_INIT ) );
		SET_READONLY( borderEnum, "captionCloseButton", Integer::New( isolate, BORDER_CAPTION_CLOSE_BUTTON ) );
		SET_READONLY( borderEnum, "noCaptionCloseButton", Integer::New( isolate, BORDER_CAPTION_NO_CLOSE_BUTTON ) );
		SET_READONLY( borderEnum, "closeIsDone", Integer::New( isolate, BORDER_CAPTION_CLOSE_IS_DONE ) );

		SET_READONLY( controlObject, "border", borderEnum );

		Local<Object> borderAnchorEnum = Object::New( isolate );
		SET_READONLY( borderAnchorEnum, "topMin", Integer::New( isolate, BORDER_ANCHOR_TOP_MIN ) );
		SET_READONLY( borderAnchorEnum, "topCenter", Integer::New( isolate, BORDER_ANCHOR_TOP_CENTER ) );
		SET_READONLY( borderAnchorEnum, "topMax", Integer::New( isolate, BORDER_ANCHOR_TOP_MAX ) );

		SET_READONLY( borderAnchorEnum, "leftMin", Integer::New( isolate, BORDER_ANCHOR_LEFT_MIN ) );
		SET_READONLY( borderAnchorEnum, "leftCenter", Integer::New( isolate, BORDER_ANCHOR_LEFT_CENTER ) );
		SET_READONLY( borderAnchorEnum, "leftMax", Integer::New( isolate, BORDER_ANCHOR_LEFT_MAX ) );

		SET_READONLY( borderAnchorEnum, "rightMin", Integer::New( isolate, BORDER_ANCHOR_RIGHT_MIN ) );
		SET_READONLY( borderAnchorEnum, "rightCenter", Integer::New( isolate, BORDER_ANCHOR_RIGHT_CENTER ) );
		SET_READONLY( borderAnchorEnum, "rightMax", Integer::New( isolate, BORDER_ANCHOR_RIGHT_MAX ) );

		SET_READONLY( borderAnchorEnum, "bottomMin", Integer::New( isolate, BORDER_ANCHOR_BOTTOM_MIN ) );
		SET_READONLY( borderAnchorEnum, "bottomCenter", Integer::New( isolate, BORDER_ANCHOR_BOTTOM_CENTER ) );
		SET_READONLY( borderAnchorEnum, "bottomMax", Integer::New( isolate, BORDER_ANCHOR_BOTTOM_MAX ) );

		SET_READONLY( controlObject, "borderAnchor", borderAnchorEnum );

		Local<Array> controlTypes = Array::New( isolate );
		controlTypes->Set( 0, String::NewFromUtf8( isolate, "Frame" ) );
		controlTypes->Set( 1, String::NewFromUtf8( isolate, "Undefined" ) );
		controlTypes->Set( 2, String::NewFromUtf8( isolate, "SubFrame" ) );
		controlTypes->Set( 3, String::NewFromUtf8( isolate, "TextControl" ) );
		controlTypes->Set( 4, String::NewFromUtf8( isolate, "Button" ) );
		controlTypes->Set( 5, String::NewFromUtf8( isolate, "CustomDrawnButton" ) );
		controlTypes->Set( 6, String::NewFromUtf8( isolate, "ImageButton" ) );
		controlTypes->Set( 7, String::NewFromUtf8( isolate, "CheckButton" ) );
		controlTypes->Set( 8, String::NewFromUtf8( isolate, "EditControl" ) );
		controlTypes->Set( 9, String::NewFromUtf8( isolate, "Slider" ) );
		controlTypes->Set( 10, String::NewFromUtf8( isolate, "ListBox" ) );
		controlTypes->Set( 11, String::NewFromUtf8( isolate, "ScrollBar" ) );
		controlTypes->Set( 12, String::NewFromUtf8( isolate, "Gridbox" ) );
		controlTypes->Set( 13, String::NewFromUtf8( isolate, "Console" ) );
		controlTypes->Set( 14, String::NewFromUtf8( isolate, "SheetControl" ) );
		controlTypes->Set( 15, String::NewFromUtf8( isolate, "Combo Box" ) );
		controlTypes->Set( 16, String::NewFromUtf8( isolate, "Basic Clock Widget" ) );
		controlTypes->Set( 17, String::NewFromUtf8( isolate, "PSI Console" ) );

		SET_READONLY( controlObject, "types", controlTypes );

		Local<Object> controlColors = Object::New( isolate );

#define makeAccessor(a,b,c,d,e) a->SetAccessorProperty( String::NewFromUtf8( isolate, b ) \
		, Function::New( isolate, c, Integer::New( isolate, e ) ) \
			, Function::New( isolate, d, Integer::New( isolate, e ) ) \
			, DontDelete )
#define makeColorAccessor(a,b,c) makeAccessor( a, b, ControlObject::getControlColor, ControlObject::setControlColor, c )

		makeColorAccessor( controlColors, "highlight", HIGHLIGHT );
		makeColorAccessor( controlColors, "normal", NORMAL );

		makeColorAccessor( controlColors, "shade", SHADE );
		makeColorAccessor( controlColors, "shadow", SHADOW );
		makeColorAccessor( controlColors, "textColor", TEXTCOLOR );
		makeColorAccessor( controlColors, "caption", CAPTION );
		makeColorAccessor( controlColors, "captionText", CAPTIONTEXTCOLOR );
		makeColorAccessor( controlColors, "inactiveCaption", INACTIVECAPTION );
		makeColorAccessor( controlColors, "InactiveCaptionText" , INACTIVECAPTIONTEXTCOLOR );
		makeColorAccessor( controlColors, "selectBack", SELECT_BACK );
		makeColorAccessor( controlColors, "selectText", SELECT_TEXT );
		makeColorAccessor( controlColors, "editBackground", EDIT_BACKGROUND );
		makeColorAccessor( controlColors, "editText" , EDIT_TEXT );
		makeColorAccessor( controlColors, "scrollBarBackground", SCROLLBAR_BACK );

		SET_READONLY( controlObject, "color", controlColors );

		//psiTemplate2->Set( isolate, "color", controlColors );
		//psiTemplate->Set( isolate, "color", controlColors );

		NODE_SET_PROTOTYPE_METHOD( regTemplate, "setCreate", RegistrationObject::setCreate );
		NODE_SET_PROTOTYPE_METHOD( regTemplate, "setDraw", RegistrationObject::setDraw );
		NODE_SET_PROTOTYPE_METHOD( regTemplate, "setMouse", RegistrationObject::setMouse );
		NODE_SET_PROTOTYPE_METHOD( regTemplate, "setKey", RegistrationObject::setKey );
		NODE_SET_PROTOTYPE_METHOD( regTemplate, "setTouch", RegistrationObject::setTouch );
		//NODE_SET_PROTOTYPE_METHOD( regTemplate, "setMove", RegistrationObject::setMove );
		//NODE_SET_PROTOTYPE_METHOD( regTemplate, "setSize", RegistrationObject::setSize );

		// Prototype
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "Control", ControlObject::NewControl );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "Frame", ControlObject::createFrame );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "focus", ControlObject::focus );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "show", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "hide", ControlObject::hide );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "reveal", ControlObject::reveal );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "close", ControlObject::close );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "get", ControlObject::get );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "edit", ControlObject::edit );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "save", ControlObject::save );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "on", ControlObject::on );

		psiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "border" )
			, FunctionTemplate::New( isolate, ControlObject::getFrameBorder )
			, FunctionTemplate::New( isolate, ControlObject::setFrameBorder )
			, DontDelete );
		psiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "font" )
			, FunctionTemplate::New( isolate, ControlObject::getControlFont )
			, FunctionTemplate::New( isolate, ControlObject::setControlFont )
			, DontDelete );
		psiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "size" )
			, FunctionTemplate::New( isolate, ControlObject::getCoordinate, Integer::New( isolate, 12 ) )
			, FunctionTemplate::New( isolate, ControlObject::setCoordinate, Integer::New( isolate, 11 ) )
			, DontDelete );
		psiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "position" )
			, FunctionTemplate::New( isolate, ControlObject::getCoordinate, Integer::New( isolate, 12 ) )
			, FunctionTemplate::New( isolate, ControlObject::setCoordinate, Integer::New( isolate, 10 ) )
			, DontDelete );
		psiTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "layout" )
			, FunctionTemplate::New( isolate, ControlObject::getCoordinate, Integer::New( isolate, 12 ) )
			, FunctionTemplate::New( isolate, ControlObject::setCoordinate, Integer::New( isolate, 12 ) )
			, DontDelete );


		Local<Function> frameConstructor = psiTemplate->GetFunction();
		SET_READONLY_METHOD( frameConstructor, "Border", newBorder );
		SET_READONLY_METHOD( frameConstructor, "load", ControlObject::load );


		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "Control", ControlObject::NewControl );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "createFrame", ControlObject::createFrame );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "createControl", ControlObject::NewControl );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "focus", ControlObject::focus );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "show", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "hide", ControlObject::hide );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "get", ControlObject::get );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "reveal", ControlObject::reveal );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "redraw", ControlObject::redraw );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "close", ControlObject::close );

		psiTemplate2->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "font" )
			, FunctionTemplate::New( isolate, ControlObject::getControlFont )
			, FunctionTemplate::New( isolate, ControlObject::setControlFont )
			, DontDelete );


		/*
		psiTemplate2->PrototypeTemplate()->SetAccessor( String::NewFromUtf8( isolate, "width" )
			, ControlObject::getCoordinate, ControlObject::setCoordinate, Integer::New( isolate, 0 ) );
		psiTemplate2->PrototypeTemplate()->SetAccessor( String::NewFromUtf8( isolate, "height" )
			, ControlObject::getCoordinate, ControlObject::setCoordinate, Integer::New( isolate, 1 ) );
		psiTemplate2->PrototypeTemplate()->SetAccessor( String::NewFromUtf8( isolate, "x" )
			, ControlObject::getCoordinate, ControlObject::setCoordinate, Integer::New( isolate, 2 ) );
		psiTemplate2->PrototypeTemplate()->SetAccessor( String::NewFromUtf8( isolate, "y" )
			, ControlObject::getCoordinate, ControlObject::setCoordinate, Integer::New( isolate, 3 ) );
		*/
		psiTemplate2->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "size" )
			, FunctionTemplate::New( isolate, ControlObject::getCoordinate, Integer::New( isolate, 12 ) )
			, FunctionTemplate::New( isolate, ControlObject::setCoordinate, Integer::New( isolate, 11 ) )
			, DontDelete );
		psiTemplate2->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "position" )
			, FunctionTemplate::New( isolate, ControlObject::getCoordinate, Integer::New( isolate, 12 ) )
			, FunctionTemplate::New( isolate, ControlObject::setCoordinate, Integer::New( isolate, 10 ) )
			, DontDelete );
		psiTemplate2->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "layout" )
			, FunctionTemplate::New( isolate, ControlObject::getCoordinate, Integer::New( isolate, 12 ) )
			, FunctionTemplate::New( isolate, ControlObject::setCoordinate, Integer::New( isolate, 12 ) )
			, DontDelete );

		//psiTemplate2->PrototypeTemplate()->SetAccessor( String::NewFromUtf8( isolate, "text" )
		//	, ControlObject::getControlText, ControlObject::setControlText );
		psiTemplate2->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "text" )
			, FunctionTemplate::New( isolate, ControlObject::getControlText )
			, FunctionTemplate::New( isolate, ControlObject::setControlText )
			, DontDelete );

		NODE_SET_PROTOTYPE_METHOD( psiTemplatePopups, "add", PopupObject::addPopupItem );
		NODE_SET_PROTOTYPE_METHOD( psiTemplatePopups, "Item", PopupObject::addPopupItem );
		NODE_SET_PROTOTYPE_METHOD( psiTemplatePopups, "track", PopupObject::trackPopup );
		Local<Function> popupObject = psiTemplatePopups->GetFunction();
		PopupObject::constructor.Reset( isolate, popupObject );
		SET_READONLY( exports, "Popup", popupObject );
		Local<Object> itemTypes = Object::New( isolate );
		SET_READONLY( itemTypes, "string", Integer::New( isolate, MF_STRING ) );
		SET_READONLY( itemTypes, "separator", Integer::New( isolate, MF_SEPARATOR ) );
		SET_READONLY( itemTypes, "popup", Integer::New( isolate, MF_POPUP ) );
		SET_READONLY( itemTypes, "checked", Integer::New( isolate, MF_CHECKED ) );
		SET_READONLY( itemTypes, "disabled", Integer::New( isolate, MF_DISABLED ) );
		SET_READONLY( itemTypes, "grayed", Integer::New( isolate, MF_GRAYED ) );
		SET_READONLY( itemTypes, "customDraw", Integer::New( isolate, MF_OWNERDRAW ) );
		SET_READONLY( popupObject, "itemType", itemTypes );
		//SET_READONLY_METHOD( popupObject, "add", PopupObject::addPopupItem );

		constructor.Reset( isolate, psiTemplate->GetFunction() );
		SET_READONLY( exports, "Frame",	psiTemplate->GetFunction() );

		constructor2.Reset( isolate, psiTemplate2->GetFunction() );
		//exports->Set( String::NewFromUtf8( isolate, "Control" ),
		//				 psiTemplate2->GetFunction() );

		registrationConstructor.Reset( isolate, regTemplate->GetFunction() );
		SET_READONLY( exports, "Registration", regTemplate->GetFunction() );
}

void ControlObject::getControlColor( const FunctionCallbackInfo<Value>& args ) {

	Isolate* isolate = args.GetIsolate();
	int colorIndex;
	Local<Value> data = args.Data();
	struct optionStrings *strings = getStrings( isolate );
	Local<Object> object;
	if( data->IsObject() ) {
		Local<Object> params = data->ToObject();
		colorIndex = (int)params->Get( strings->indexString->Get( isolate ) )->IntegerValue();
		object = params->Get( strings->objectString->Get( isolate ) )->ToObject();
	}
	else {
		colorIndex = (int)data->IntegerValue();
		object = args.This();
	}
	args.Data()->IntegerValue();
	Local<FunctionTemplate> controlTpl = controlTemplate.Get( isolate );
	if( controlTpl->HasInstance( object ) ) {

		ControlObject *c = ObjectWrap::Unwrap<ControlObject>( object );
		args.GetReturnValue().Set( ColorObject::makeColor( isolate, GetControlColor( c->control, colorIndex ) ) );
	}
	else {
		Local<FunctionTemplate> controlTpl = frameTemplate.Get( isolate );
		if( controlTpl->HasInstance( object ) ) {

			ControlObject *c = ObjectWrap::Unwrap<ControlObject>( object );
			args.GetReturnValue().Set( ColorObject::makeColor( isolate, GetControlColor( c->control, colorIndex ) ) );
		}
		else
			args.GetReturnValue().Set( ColorObject::makeColor( isolate, GetControlColor( NULL, colorIndex ) ) );
	}
}

void ControlObject::setControlColor( const FunctionCallbackInfo<Value>& args ) {
	int colorIndex = (int)args.Data()->IntegerValue();
	Isolate* isolate = args.GetIsolate();
	Local<Value> data = args.Data();
	struct optionStrings *strings = getStrings( isolate );
	Local<Object> object;

	if( data->IsObject() ) {
		Local<Object> params = data->ToObject();
		colorIndex = (int)params->Get( strings->indexString->Get( isolate ) )->IntegerValue();
		object = params->Get( strings->objectString->Get( isolate ) )->ToObject();
	}
	else {
		colorIndex = (int)data->IntegerValue();
		object = args.This();
	}


	CDATA newColor;
	Local<Object> color = args[0]->ToObject();
	if( ColorObject::isColor( isolate, color ) ) {
		newColor = ColorObject::getColor( color );
	}
	else {
		newColor = (uint32_t)args[0]->NumberValue();
	}

	Local<FunctionTemplate> controlTpl = controlTemplate.Get( isolate );
	if( controlTpl->HasInstance( object ) ) {
		ControlObject *c = ObjectWrap::Unwrap<ControlObject>( object );
		SetControlColor( c->control, colorIndex, newColor );
	}
	else {
		controlTpl = frameTemplate.Get( isolate );
		if( controlTpl->HasInstance( object ) ) {
			ControlObject *c = ObjectWrap::Unwrap<ControlObject>( object );
			SetControlColor( c->control, colorIndex, newColor );
		} else
			SetControlColor( NULL, colorIndex, newColor );
	}
}


void ControlObject::getControlColor2( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Value> data = args.Data();
	int colorIndex = (int)data->IntegerValue();
	VoidObject *v = ObjectWrap::Unwrap<VoidObject>( args.This() );
	ControlObject *c = (ControlObject*)v->data;
	args.GetReturnValue().Set( ColorObject::makeColor( isolate, GetControlColor( c->control, colorIndex ) ) );
}

void ControlObject::setControlColor2( const FunctionCallbackInfo<Value>& args ) {

	Isolate* isolate = args.GetIsolate();
	Local<Value> data = args.Data();
	int colorIndex = (int)data->IntegerValue();
	VoidObject *v = ObjectWrap::Unwrap<VoidObject>( args.This() );
	ControlObject *c = (ControlObject*)v->data;
	args.GetReturnValue().Set( ColorObject::makeColor( isolate, GetControlColor( c->control, colorIndex ) ) );

	CDATA newColor;
	Local<Object> color = args[0]->ToObject();
	if( ColorObject::isColor( isolate, color ) ) {
		newColor = ColorObject::getColor( color );
	} else {
		newColor = (uint32_t)args[0]->NumberValue();
	}

	SetControlColor( c->control, colorIndex, newColor );
}


ControlObject::ControlObject( ControlObject *over, const char *type, const char *title, int x, int y, int w, int h ) {
	registration = NULL;
	image = NULL;
	frame = over;
	psiLocal.pendingCreate = this;
	if( !title )
		control = MakeNamedControl( over->control, type, x, y, w, h, 0 );
	else
		control = MakeNamedCaptionedControl( over->control, type, x, y, w, h, 0, title );
	psiLocal.pendingCreate = NULL;
}

ControlObject::ControlObject( const char *title, int x, int y, int w, int h, int border, ControlObject *over ) {
	registration = NULL;
	image = NULL;
	frame = over;
	psiLocal.pendingCreate = this;
	control = ::CreateFrame( title, x, y, w, h, border, over ? over->control : (PSI_CONTROL)NULL );
	PSI_SetBindingData( control, psiLocal.bindingDataId, (uintptr_t)this );
	psiLocal.pendingCreate = NULL;
}

ControlObject::ControlObject( const char *type, ControlObject *parent, int32_t x, int32_t y, uint32_t w, uint32_t h ) {
	registration = NULL;
	image = NULL;
	psiLocal.pendingCreate = this;
	control = ::MakeNamedControl( parent->control, type, x, y, w, h, -1 );
	psiLocal.pendingCreate = NULL;
}


ControlObject::~ControlObject() {

}
ControlObject::ControlObject( PSI_CONTROL control ) {
	//memcpy( this, &_blankObject, sizeof( *this ) );
	registration = NULL;
	image = NULL;
	frame = NULL;
	this->control = control;
}

/* this is constructor of sack.PSI.Frame */
void ControlObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	enableEventLoop();
	if( args.IsConstructCall() ) {
		if( args.Length() == 0 ) {
			ControlObject* obj = new ControlObject();
			ControlObject::wrapSelf( isolate, obj, args.This() );
			args.GetReturnValue().Set( args.This() );
			return;
		}

		char *title = "Node Application";
		char *tmpTitle = NULL;
		int x = 0, y = 0, w = 1024, h = 768, border = 0;
		int arg_ofs = 0;
		ControlObject *parent = NULL;

		int argc = args.Length();
		if( argc > 0 ) {
			String::Utf8Value fName( args[0]->ToString() );
			tmpTitle = title = StrDup( *fName );
			arg_ofs++;
		}
		else {
			ControlObject* obj = new ControlObject( );
			ControlObject::wrapSelf( isolate, obj, args.This() );
			args.GetReturnValue().Set( args.This() );
			return;
		}
		if( argc > (arg_ofs + 0) ) {
			x = (int)args[arg_ofs + 0]->NumberValue();
		}
		if( argc > (arg_ofs + 1) ) {
			y = (int)args[arg_ofs + 1]->NumberValue();
		}
		if( argc > (arg_ofs + 2) ) {
			w = (int)args[arg_ofs + 2]->NumberValue();
		}
		if( argc > (arg_ofs + 3) ) {
			h = (int)args[arg_ofs + 3]->NumberValue();
		}

		if( argc > (arg_ofs + 4) ) {
			border = (int)args[arg_ofs+4]->NumberValue();
		}
		/*
		if( argc > 6 ) {
			parent = (int)args[5]->NumberValue();
			}
		*/
		// Invoked as constructor: `new MyObject(...)`
		
		ControlObject* obj = new ControlObject( title, x, y, w, h, border, NULL );
		ControlObject::wrapSelf( isolate, obj, args.This() );
		args.GetReturnValue().Set( args.This() );
		if( tmpTitle )
			Deallocate( char*, title );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete argv;
	}
}

static void ProvideKnownCallbacks( Isolate *isolate, Local<Object>c, ControlObject *obj ) {
	CTEXTSTR type = GetControlTypeName( obj->control );
	if( StrCmp( type, CONTROL_FRAME_NAME ) == 0 ) {
		SetCommonButtons( obj->control, &obj->done, &obj->okay );

	} else if( StrCmp( type, "PSI Console" ) == 0 ) {
		c->Set( String::NewFromUtf8( isolate, "write" ), Function::New( isolate, ControlObject::writeConsole ) );
		c->Set( String::NewFromUtf8( isolate, "send" ), Function::New( isolate, ControlObject::writeConsole ) );
		c->SetAccessorProperty( String::NewFromUtf8( isolate, "echo" )
			, Function::New( isolate, ControlObject::getConsoleEcho )
			, Function::New( isolate, ControlObject::setConsoleEcho )
			, DontDelete );
		c->Set( String::NewFromUtf8( isolate, "oninput" ), Function::New( isolate, ControlObject::setConsoleRead ) );
	} else if( StrCmp( type, NORMAL_BUTTON_NAME ) == 0 ) {
		int ID = GetControlID( obj->control );
		if( ID == BTN_OKAY || ID == BTN_CANCEL ) {
			SetCommonButtons( obj->control, &obj->done, &obj->okay );
		}
		c->Set( String::NewFromUtf8( isolate, "on" ), Function::New( isolate, ControlObject::setButtonEvent ) );
		c->Set( String::NewFromUtf8( isolate, "click" ), Function::New( isolate, ControlObject::setButtonClick ) );
	} else if( StrCmp( type, IMAGE_BUTTON_NAME ) == 0 ) {
		c->Set( String::NewFromUtf8( isolate, "on" ), Function::New( isolate, ControlObject::setButtonEvent ) );
		c->Set( String::NewFromUtf8( isolate, "click" ), Function::New( isolate, ControlObject::setButtonClick ) );
	} else if( StrCmp( type, CUSTOM_BUTTON_NAME ) == 0 ) {
		c->Set( String::NewFromUtf8( isolate, "on" ), Function::New( isolate, ControlObject::setButtonEvent ) );
		c->Set( String::NewFromUtf8( isolate, "click" ), Function::New( isolate, ControlObject::setButtonClick ) );
	} else if( StrCmp( type, RADIO_BUTTON_NAME ) == 0 ) {

	} else if( StrCmp( type, SCROLLBAR_CONTROL_NAME ) == 0 ) {
	} else if( StrCmp( type, EDIT_FIELD_NAME ) == 0 ) {
		c->SetAccessorProperty( String::NewFromUtf8( isolate, "password" )
			, Function::New( isolate, ControlObject::getPassword )
			, Function::New( isolate, ControlObject::setPassword )
			, DontDelete );

	} else if( StrCmp( type, LISTBOX_CONTROL_NAME ) == 0 ) {
		c->Set( String::NewFromUtf8( isolate, "setTabs" ), Function::New( isolate, ControlObject::setListboxTabs ) );
		c->Set( String::NewFromUtf8( isolate, "addItem" ), Function::New( isolate, ControlObject::addListboxItem ) );
		c->SetAccessorProperty( String::NewFromUtf8( isolate, "header" )
			, Function::New( isolate, ControlObject::getListboxHeader )
			, Function::New( isolate, ControlObject::setListboxHeader )
			, DontDelete );
		c->Set( String::NewFromUtf8( isolate, "measure" ), Function::New( isolate, ControlObject::measureListItem ) );
		c->Set( String::NewFromUtf8( isolate, "hScroll" ), Function::New( isolate, ControlObject::setListboxHScroll ) );
		c->Set( String::NewFromUtf8( isolate, "removeItem" ), Function::New( isolate, ListboxItemObject::removeListboxItem ) );
		c->Set( String::NewFromUtf8( isolate, "onSelect" ), Function::New( isolate, ControlObject::setListboxOnSelect ) );
		c->Set( String::NewFromUtf8( isolate, "onDoubleClick" ), Function::New( isolate, ControlObject::setListboxOnDouble ) );
	} else if( StrCmp( type, SHEET_CONTROL_NAME ) == 0 ) {
		c->Set( String::NewFromUtf8( isolate, "addPage" ), Function::New( isolate, ControlObject::addSheetsPage ) );

	} else if( StrCmp( type, PROGRESS_BAR_CONTROL_NAME ) == 0 ) {
		c->SetAccessorProperty( String::NewFromUtf8( isolate, "range" )
			, Function::New( isolate, ControlObject::setProgressBarRange )
			, Function::New( isolate, ControlObject::setProgressBarRange )
			, DontDelete );
		c->SetAccessorProperty( String::NewFromUtf8( isolate, "progress" )
			, Function::New( isolate, ControlObject::setProgressBarProgress )
			, Function::New( isolate, ControlObject::setProgressBarProgress )
			, DontDelete );
		c->Set( String::NewFromUtf8( isolate, "colors" ), Function::New( isolate, ControlObject::setProgressBarColors ) );
		c->SetAccessorProperty( String::NewFromUtf8( isolate, "text" )
			, Function::New( isolate, ControlObject::setProgressBarTextEnable )
			, Function::New( isolate, ControlObject::setProgressBarTextEnable )
			, DontDelete );
	} else if( StrCmp( type, "Basic Clock Widget" ) == 0 ) {
		c->Set( String::NewFromUtf8( isolate, "analog" ), Function::New( isolate, ControlObject::makeAnalog ) );
	}
}



Local<Object> ControlObject::NewWrappedControl( Isolate* isolate, PSI_CONTROL pc ) {
	// Invoked as plain function `MyObject(...)`, turn into construct call.
	ControlObject *control;
	INDEX idx;
	LIST_FORALL( psiLocal.controls, idx, ControlObject *, control ) {
		if( control->control == pc ) {
			return control->state.Get( isolate );
		}
	}

	CTEXTSTR type = GetControlTypeName( pc );

	Local<Function> cons = Local<Function>::New( isolate, StrCmp( type, "Frame" ) == 0 ?constructor : constructor2 );
	Local<Object> c = cons->NewInstance( isolate->GetCurrentContext(), 0, 0 ).ToLocalChecked();
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( c );
	me->control = pc;
	if( pc )
		ProvideKnownCallbacks( isolate, c, me );
	return c;
}


void ControlObject::writeConsole( const FunctionCallbackInfo<Value>& args) {
	ControlObject *c = ObjectWrap::Unwrap<ControlObject>( args.This() );
	if( args.Length() > 0 )
	{
		if( args[0]->IsObject() ) 			{
			PTEXT text = isTextObject( args.GetIsolate(), args[0] );
			if( text ) {
				PSIConsoleDirectOutput( c->control, SegDuplicate( text ) );
				return;
			}
		}
		String::Utf8Value fName( args[0]->ToString() );
		pcprintf( c->control, "%s", (const char*)*fName );
	}
}

static void consoleInputEvent( uintptr_t arg, PTEXT text ) {
	MakePSIEvent( (ControlObject*)arg, true, Event_Control_ConsoleInput, text=BuildLine(text) );
	LineRelease( text );
}

void ControlObject::setConsoleRead( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	ControlObject *c = NULL;
	//Local<FunctionTemplate> tpl = ControlObject::controlTemplate.Get( isolate );
	//if( tpl->HasInstance( args.This() ) )
	c = ObjectWrap::Unwrap<ControlObject>( args.This() );
	//else if( tpl->HasInstance( args.Holder() ) )
	// c = ObjectWrap::Unwrap<ControlObject>( args.Holder() );

	if( args.Length() > 0 )
	{
		c->customEvents[0].Reset( isolate, Handle<Function>::Cast( args[0] ) );
		//args[0]->ToFunction
		PSIConsoleInputEvent( c->control, consoleInputEvent, (uintptr_t)c );
	}
}

void  ControlObject::addSheetsPage( const FunctionCallbackInfo<Value>& args ) {
	//String::Utf8Value title( args[0]->ToString() );
	ControlObject *c = ObjectWrap::Unwrap<ControlObject>( args.This() );
	ControlObject *page = ObjectWrap::Unwrap<ControlObject>( args[0]->ToObject() );
	AddSheet( c->control, page->control );
}

static void buttonClicked( uintptr_t object, PSI_CONTROL ) {
	MakePSIEvent( (ControlObject*)object, true, Event_Control_ButtonClick );
}

void ControlObject::setButtonClick( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *c = ObjectWrap::Unwrap<ControlObject>( args.This() );
	if( args.Length() > 0 )
	{
		Isolate* isolate = args.GetIsolate();
		::SetButtonPushMethod( c->control, buttonClicked, (uintptr_t)c );
		c->customEvents[0].Reset( isolate, Handle<Function>::Cast( args[0] ) );
	}
}

void ControlObject::setButtonEvent( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *c = ObjectWrap::Unwrap<ControlObject>( args.This() );
	if( args.Length() > 0 )
	{
		Isolate* isolate = args.GetIsolate();
		String::Utf8Value event( args[0] );
		c->customEvents[0].Reset( isolate, Handle<Function>::Cast( args[1] ) );
		::SetButtonPushMethod( c->control, buttonClicked, (uintptr_t)c );
	}
}

void ControlObject::makeAnalog( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *c = ObjectWrap::Unwrap<ControlObject>( args.This() );
	if( args.Length() == 0 )
		MakeClockAnalog( c->control );
}

/* this is constructor of sack.PSI.Control (also invoked as frame.Control */
void ControlObject::NewControl( const FunctionCallbackInfo<Value>& args ) {
	int argc = args.Length();
	Isolate* isolate = args.GetIsolate();
	if( argc == 0 ) {
		if( args.IsConstructCall() ) {
			ControlObject *control = new ControlObject();
			control->wrapSelf( isolate, control, args.This() );
			args.GetReturnValue().Set( args.This() );
		}  else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Required parameter 'controlType' missing." ) ) );
		return;
	}

	char *type = NULL;
	char *title = NULL;
	int x = 0, y = 0, w = 1024, h = 768, border = 0;
	int argOffset = 0;
	ControlObject *parent = NULL;


		Local<Function> cons = Local<Function>::New( isolate, constructor2 );
		Local<Object> newControl = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
		ControlObject *container = ObjectWrap::Unwrap<ControlObject>( args.Holder() );

		if( argc > 0 ) {
			String::Utf8Value fName( args[0]->ToString() );
			type = StrDup( *fName );
		}

		{
			if( argc > 1 && args[1]->IsString() ) {
				String::Utf8Value fName( args[1]->ToString() );
				title = StrDup( *fName );
				argOffset = 1;
			}
			else {
				//ControlObject *parent = ObjectWrap::Unwrap<ControlObject>( args[1]->ToObject() );
				if( argc > 4 ) {
					x = (int)args[1]->NumberValue();
					y = (int)args[2]->NumberValue();
					w = (int)args[3]->NumberValue();
					h = (int)args[4]->NumberValue();
				}
				else {
					x = g.nextControlCreatePosition.x;
					y = g.nextControlCreatePosition.y;
					w = g.nextControlCreatePosition.w;
					h = g.nextControlCreatePosition.h;
				}
				psiLocal.newControl = newControl;
				ControlObject* obj = ObjectWrap::Unwrap<ControlObject>( newControl );
				psiLocal.pendingCreate = obj;
				PSI_CONTROL pc = MakeNamedControl( container->control, type, x, y, w, h, 0 );

				obj->control = pc;
				ProvideKnownCallbacks( isolate, newControl, obj );

				//g.nextControlCreatePosition.control->pc = obj->control;
				//g.nextControlCreatePosition.resultControl = obj->control;

				//obj->Wrap( newControl );
				args.GetReturnValue().Set( newControl );
				return;
			}
		}
		if( argc > (1+argOffset) ) {
			x = (int)args[1+argOffset]->NumberValue();
		}
		if( argc > (2+argOffset) ) {
			y = (int)args[2+argOffset]->NumberValue();
		}
		if( argc > (3+argOffset) ) {
			w = (int)args[3+argOffset]->NumberValue();
		}
		if( argc > (4+argOffset) ) {
			h = (int)args[4+argOffset]->NumberValue();
		}

		// Invoked as constructor: `new MyObject(...)`
		psiLocal.newControl = newControl;
		ControlObject* obj = new ControlObject( container, type, title, x, y, w, h );
		ControlObject::wrapSelf( isolate, obj, newControl );

		args.GetReturnValue().Set( newControl );

		Deallocate( char*, type );
		if( title )
			Deallocate( char*, title );
}

void ControlObject::createFrame( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {

		char *title = "Node Application";
		int x = 0, y = 0, w = 1024, h = 768, border = 0;
		ControlObject *parent = NULL;

		int argc = args.Length();
		if( argc > 0 ) {
			String::Utf8Value fName( args[0]->ToString() );
			title = StrDup( *fName );
		}
		if( argc > 1 ) {
			x = (int)args[1]->NumberValue();
		}
		if( argc > 2 ) {
			y = (int)args[2]->NumberValue();
		}
		if( argc > 3 ) {
			w = (int)args[3]->NumberValue();
		}
		if( argc > 4 ) {
			h = (int)args[4]->NumberValue();
		}
		if( argc > 5 ) {
			border = (int)args[5]->NumberValue();
		}
     /*
		if( argc > 6 ) {
			parent = (int)args[5]->NumberValue();
			}
       */
		// Invoked as constructor: `new MyObject(...)`
		ControlObject* obj = new ControlObject( title, x, y, w, h, border, NULL );
		ControlObject::wrapSelf( isolate, obj, args.This() );
		args.GetReturnValue().Set( args.This() );

		Deallocate( char*, title );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
		argv[n] = args[n];

		Local<Function> cons = Local<Function>::New( isolate, constructor2 );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete argv;
	}
}

void ControlObject::redraw( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	SmudgeCommon( me->control );
}

void ControlObject::getConsoleEcho( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	Isolate *isolate = args.GetIsolate();
	args.GetReturnValue().Set( PSIConsoleGetLocalEcho( me->control ) ? True( isolate ) : False( isolate ) );
}
void ControlObject::setConsoleEcho( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	PSIConsoleSetLocalEcho( me->control, args[0]->BooleanValue() );
}


void ControlObject::getPassword( const FunctionCallbackInfo<Value>& args ) {
}
void ControlObject::setPassword( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	SetEditControlPassword( me->control, args[0]->BooleanValue() );
}

void ControlObject::focus( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	SetCommonFocus( me->control );
}

static uintptr_t waitDialog( PTHREAD thread ) {
	ControlObject *waiter = (ControlObject*)GetThreadParam( thread );
	CommonWait( waiter->control );
	if( waiter->done ) {
		if( waiter->okay ) {
			if( !waiter->cbFrameEventOkay.IsEmpty() )
				MakePSIEvent( waiter, true, Event_Frame_Ok );
		} else {
			if( !waiter->cbFrameEventCancel.IsEmpty() )
				MakePSIEvent( waiter, true, Event_Frame_Cancel );
		}
	} else {
		if( waiter->okay ) {
			MakePSIEvent( waiter, true, Event_Frame_Ok );
		} else {
			if( !waiter->cbFrameEventAbort.IsEmpty() )
				MakePSIEvent( waiter, true, Event_Frame_Abort );
			else if( !waiter->cbFrameEventCancel.IsEmpty() )
				MakePSIEvent( waiter, true, Event_Frame_Cancel );
		}
	}
	waiter->waiter = NULL;
	return 0;
}

void ControlObject::show( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	DisplayFrame( me->control );
	me->okay = 0;
	me->done = 0;
	me->waiter = ThreadTo( waitDialog, (uintptr_t)me );
}

void ControlObject::close( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	DestroyControl( me->control );
}

void ControlObject::hide( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	HideControl( me->control );
}

void ControlObject::reveal( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	RevealCommon( me->control );
}

void ControlObject::edit( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	EditFrame( me->control, 1 );
}
void ControlObject::get( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	String::Utf8Value name( args[0]->ToString() );
	PSI_CONTROL pc = GetControlByName( me->control, *name );
	ControlObject *control;
	INDEX idx;
	LIST_FORALL( psiLocal.controls, idx, ControlObject *, control ) {
		if( control->control == pc ) {
			args.GetReturnValue().Set( control->state.Get( isolate ) );
			return ;
		}
	}
	//args.GetReturnValue().Set( NewWrappedControl( isolate, pc ) );
}

static void OnMoveCommon( CONTROL_FRAME_NAME )( PSI_CONTROL pc, LOGICAL startOrMoved ) {
	ControlObject *control = (ControlObject *)PSI_GetBindingData( pc, psiLocal.bindingDataId );
	if( control ) {
		int32_t x, y;
		GetFramePosition( pc, &x, &y );
		GetPhysicalCoordinate( pc, &x, &y, TRUE );
		MakePSIEvent( control, false, Event_Control_Move, x, y, startOrMoved );
	}
}

static void OnSizeCommon( CONTROL_FRAME_NAME )(PSI_CONTROL pc, LOGICAL startOrMoved) {
	ControlObject *control = (ControlObject *)PSI_GetBindingData( pc, psiLocal.bindingDataId );
	if( control ) {
		uint32_t w, h;
		GetFrameSize( pc, &w, &h );
		MakePSIEvent( control, false, Event_Control_Resize, w, h, startOrMoved );
	}
}


void ControlObject::on( const FunctionCallbackInfo<Value>& args ) {
	if( args.Length() >= 2 ) {
		Isolate* isolate = args.GetIsolate();
		ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
		String::Utf8Value name( args[0]->ToString() );
		Local<Function> cb = Handle<Function>::Cast( args[1] );
		if( strcmp( *name, "ok" ) == 0 ) {
			me->cbFrameEventOkay.Reset( isolate, cb );
		}
		if( strcmp( *name, "cancel" ) == 0 ) {
			me->cbFrameEventCancel.Reset( isolate, cb );
		}
		if( strcmp( *name, "cancel" ) == 0 ) {
			me->cbFrameEventCancel.Reset( isolate, cb );
		}
		if( strcmp( *name, "unshow" ) == 0 ) {
			//me->cbFrameEventAbort.Reset( isolate, cb );
		}
		if( strcmp( *name, "move" ) == 0 ) {
			me->cbMoveEvent.Reset( isolate, cb );
		}
		if( strcmp( *name, "size" ) == 0 ) {
			me->cbSizeEvent.Reset( isolate, cb );
		}
	}
}

void ControlObject::getControlFont( const FunctionCallbackInfo<Value>& args ) {

}
void ControlObject::setControlFont( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	FontObject *font = ObjectWrap::Unwrap<FontObject>( args[0]->ToObject() );
	SetCommonFont( me->control, font->font );
}


void ControlObject::getFrameBorder( const FunctionCallbackInfo<Value>& args ) {

}
void ControlObject::setFrameBorder( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	if( args[0]->IsObject() ) {
		VoidObject *border = ObjectWrap::Unwrap<VoidObject>( args[0]->ToObject() );
		PSI_SetFrameBorder( me->control, (PFrameBorder)border->data );
	}
	else
		PSI_SetFrameBorder( me->control, NULL );
}

void ControlObject::save( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	String::Utf8Value name( args[0]->ToString() );
	SaveXMLFrame( me->control, *name );
}

void ControlObject::load( const FunctionCallbackInfo<Value>& args ) {
	//ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	String::Utf8Value name( args[0]->ToString() );
	PSI_CONTROL pc = LoadXMLFrame( *name );
	Local<Object> blah = NewWrappedControl( args.GetIsolate(), pc );
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( blah );
	PSI_SetBindingData( pc, psiLocal.bindingDataId, (uintptr_t)me );
	SetCommonButtons( pc, &me->done, &me->okay );

	args.GetReturnValue().Set( blah );
}

//-------------------------------------------------------

void ControlObject::wrapSelf( Isolate* isolate, ControlObject *_this, Local<Object> into ) {
	_this->Wrap( into );
	_this->state.Reset( isolate, into );
	//SetupControlColors( isolate, into );
	AddControlColors( isolate, into, _this );
	if( _this->control )
		ProvideKnownCallbacks( isolate, into, _this );
}

//-------------------------------------------------------

void ControlObject::releaseSelf( ControlObject *_this ) {
	_this->state.Reset();
}


void ControlObject::getControlText( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	static char buf[1024];
	GetControlText( me->control, buf, 1024 );
	args.GetReturnValue().Set( String::NewFromUtf8( isolate, buf ) );
}
void ControlObject::setControlText( const FunctionCallbackInfo<Value>& args ) {
	String::Utf8Value text( args[0] );
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	SetControlText( me->control, *text );
}


void ControlObject::getCoordinate( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	int coord = (int)args.Data()->IntegerValue();
	Local<Object> o = Object::New( isolate );
	struct optionStrings *strings = getStrings( isolate );
	int32_t x, y;
	uint32_t w, h;
	GetFramePosition( me->control, &x, &y );
	GetFrameSize( me->control, &w, &h );
	switch( coord ) {
	case 10:
		o->Set( strings->xString->Get( isolate ), Integer::New( isolate, x ) );
		o->Set( strings->yString->Get( isolate ), Integer::New( isolate, y ) );
		break;
	case 11:
		o->Set( strings->wString->Get( isolate ), Integer::New( isolate, w ) );
		o->Set( strings->hString->Get( isolate ), Integer::New( isolate, h ) );
		break;
	case 12:
		o->Set( strings->xString->Get( isolate ), Integer::New( isolate, x ) );
		o->Set( strings->yString->Get( isolate ), Integer::New( isolate, y ) );
		o->Set( strings->wString->Get( isolate ), Integer::New( isolate, w ) );
		o->Set( strings->hString->Get( isolate ), Integer::New( isolate, h ) );
		break;
	}
	args.GetReturnValue().Set( o );
}

void ControlObject::setCoordinate( const FunctionCallbackInfo<Value>&  args ) {

	Isolate* isolate = args.GetIsolate();
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	int coord = (int)args.Data()->IntegerValue();
	Local<Object> o = args[0]->ToObject();
	if( !o.IsEmpty() ) {
		struct optionStrings *strings = getStrings( isolate );
		int32_t x, y;
		uint32_t w, h;
		switch( coord ) {
		case 10:
			x = (int32_t)o->Get( strings->xString->Get( isolate ) )->IntegerValue();
			y = (int32_t)o->Get( strings->yString->Get( isolate ) )->IntegerValue();
			MoveCommon( me->control, x, y );
			break;
		case 11:
			w = (uint32_t)o->Get( strings->wString->Get( isolate ) )->IntegerValue();
			h = (uint32_t)o->Get( strings->hString->Get( isolate ) )->IntegerValue();
			SizeCommon( me->control,w, h );
			break;
		case 12:
			x = (int32_t)o->Get( strings->xString->Get( isolate ) )->IntegerValue();
			y = (int32_t)o->Get( strings->yString->Get( isolate ) )->IntegerValue();
			w = (uint32_t)o->Get( strings->wString->Get( isolate ) )->IntegerValue();
			h = (uint32_t)o->Get( strings->hString->Get( isolate ) )->IntegerValue();
			MoveSizeCommon( me->control, x, y, w, h );
			break;
		}
	}

}

//-------------------------------------------------------

void ControlObject::setProgressBarRange( const FunctionCallbackInfo<Value>&  args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	ProgressBar_SetRange( me->control, args[0]->Int32Value() );
}
void ControlObject::setProgressBarProgress( const FunctionCallbackInfo<Value>&  args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	ProgressBar_SetProgress( me->control, args[0]->Int32Value() );
}
void ControlObject::setProgressBarColors( const FunctionCallbackInfo<Value>&  args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	CDATA c1 = ColorObject::getColor( args[0]->ToObject() );
	CDATA c2 = ColorObject::getColor( args[1]->ToObject() );
	ProgressBar_SetColors( me->control, c1, c2 );
}
void ControlObject::setProgressBarTextEnable( const FunctionCallbackInfo<Value>&  args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	ProgressBar_EnableText( me->control, args[0]->Int32Value() );
}

//-------------------------------------------------------
// LISTBOX
//-----------------------------------------------------------
void ControlObject::setListboxTabs( const FunctionCallbackInfo<Value>&  args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	if( args.Length() && args[0]->IsArray() ) {
		Local<Array> list = Handle<Array>::Cast( args[0] );
		int n;
		int l = list->Length();
		int *vals = NewArray( int, l );
		for( n = 0; n < l; n++ ) {
			vals[n] = (int)list->Get( n )->IntegerValue();
		}
		SetListBoxTabStops( me->control, l, vals );
		Deallocate( int*, vals );
	}
}
//InsertListItem
void ControlObject::addListboxItem( const FunctionCallbackInfo<Value>&  args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	String::Utf8Value text( args[0]->ToString() );

	Isolate* isolate = args.GetIsolate();
	Local<Function> cons = Local<Function>::New( isolate, ListboxItemObject::constructor );
	Local<Object> lio = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
	ListboxItemObject *pli = ObjectWrap::Unwrap<ListboxItemObject>( lio );

	pli->pli = AddListItem( me->control, *text );
	SetItemData( pli->pli, (uintptr_t)pli );
	pli->control = me;
	args.GetReturnValue().Set( lio );

}


void ControlObject::measureListItem( const FunctionCallbackInfo<Value>&  args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	String::Utf8Value text( args[0]->ToString() );

	Isolate* isolate = args.GetIsolate();
	int width = MeasureListboxItem( me->control, *text );
	args.GetReturnValue().Set( Int32::New( isolate, width ) );

}
void ControlObject::setListboxHScroll( const FunctionCallbackInfo<Value>&  args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	//String::Utf8Value text( args[0]->ToString() );

	Isolate* isolate = args.GetIsolate();
	SetListboxHorizontalScroll( me->control, args[0]->BooleanValue(), args[1]->Int32Value() );
}


void ControlObject::getListboxHeader( const FunctionCallbackInfo<Value>&  args ) {
}

void ControlObject::setListboxHeader( const FunctionCallbackInfo<Value>&  args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	String::Utf8Value text( args[0]->ToString() );

	Isolate* isolate = args.GetIsolate();
	SetListboxHeader( me->control, *text );
}

static void DoubleClickHandler( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM hli ){
	ControlObject *me = (ControlObject *)psvUser;
	MakePSIEvent( me, true, Event_Listbox_DoubleClick, (MenuItemObject*)GetItemData( hli ) );
}

void ControlObject::setListboxOnDouble( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	me->listboxOnDouble.Reset( isolate, Handle<Function>::Cast( args[0] ) );
	SetDoubleClickHandler( me->control, DoubleClickHandler, (uintptr_t)me );
		
}

static void SelChangeHandler( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM hli ) {
	ControlObject *me = (ControlObject *)psvUser;
	MakePSIEvent( me, true, Event_Listbox_Selected, ((MenuItemObject*)GetItemData( hli )) );
}

void ControlObject::setListboxOnSelect( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	me->listboxOnSelect.Reset( isolate, Handle<Function>::Cast( args[0] ) );
	SetSelChangeHandler( me->control, SelChangeHandler, (uintptr_t)me );
}



void ListboxItemObject::removeListboxItem( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	//ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	ListboxItemObject *me = ObjectWrap::Unwrap<ListboxItemObject>( args[0]->ToObject() );
	DeleteListItem( me->control->control, me->pli );
	me->_this.Reset();
}

ListboxItemObject::ListboxItemObject() {
	memcpy( this, &_blankListbox, sizeof( *this ) );
}
ListboxItemObject::~ListboxItemObject() {

}

void ListboxItemObject::wrapSelf( Isolate* isolate, ListboxItemObject *_this, Local<Object> into ) {
	_this->Wrap( into );
	_this->_this.Reset( isolate, into );	
}

void ListboxItemObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	//enableEventLoop();
	if( args.IsConstructCall() ) {
		ListboxItemObject* obj = new ListboxItemObject( );
		ListboxItemObject::wrapSelf( isolate, obj, args.This() );
		args.GetReturnValue().Set( args.This() );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );
	}
}

//-------------------------------------------------------
// MENU ITEMS 
//-----------------------------------------------------------

void MenuItemObject::removeItem( const FunctionCallbackInfo<Value>&  args ) {
	MenuItemObject *mio = ObjectWrap::Unwrap<MenuItemObject>( args.This() );
	DeletePopupItem( mio->popup->popup, mio->uid, 0 );
}

MenuItemObject::MenuItemObject() {
	memcpy( this, &_blankMenuItem, sizeof( *this ) );
}
MenuItemObject::~MenuItemObject() {

}

void MenuItemObject::wrapSelf( Isolate* isolate, MenuItemObject *_this, Local<Object> into ) {
	_this->Wrap( into );
	_this->_this.Reset( isolate, into );	
}

void MenuItemObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	//enableEventLoop();
	if( args.IsConstructCall() ) {
		MenuItemObject* obj = new MenuItemObject( );
		MenuItemObject::wrapSelf( isolate, obj, args.This() );
		args.GetReturnValue().Set( args.This() );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );
	}
}


//-------------------------------------------------------

PopupObject::PopupObject() {
	memcpy( this, &_blankPopup, sizeof( *this ) );
}
PopupObject::~PopupObject() {

}

void PopupObject::wrapSelf( Isolate* isolate, PopupObject *_this, Local<Object> into ) {
	_this->Wrap( into );
	_this->_this.Reset( isolate, into );	
}

void PopupObject::NewPopup( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	enableEventLoop();
	if( args.IsConstructCall() ) {

		PopupObject* obj = new PopupObject( );
		PopupObject::wrapSelf( isolate, obj, args.This() );
		obj->popup = CreatePopup();
		args.GetReturnValue().Set( args.This() );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );
	}
}

void PopupObject::addPopupItem( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() > 0 )
	{
		Local<Function> cons = Local<Function>::New( isolate, MenuItemObject::constructor );
		Local<Object> menuItemObject = cons->NewInstance( isolate->GetCurrentContext(), NULL, 0 ).ToLocalChecked();
		MenuItemObject *mio = ObjectWrap::Unwrap<MenuItemObject>( menuItemObject );

		PopupObject *popup = ObjectWrap::Unwrap<PopupObject>( args.This() );
		String::Utf8Value *text = NULL;
		if( args.Length() > 0 )
			text = new String::Utf8Value( args[1]->ToString() );
		int32_t type = args[0]->Int32Value();
		PMENUITEM pmi;
		//lprintf( "item text:%s", *text[0] );
		if( type & MF_POPUP && ( args.Length() > 2 ) ) {
			PopupObject *subMenu = ObjectWrap::Unwrap<PopupObject>( args[2]->ToObject() );
			pmi = AppendPopupItem( popup->popup, type, (uintptr_t)subMenu->popup, *text[0] );
		} else {
			pmi = AppendPopupItem( popup->popup, type, mio->uid = popup->itemId++, *text[0] );
			if( args.Length() > 2 )
				mio->cbSelected.Reset( isolate, Handle<Function>::Cast( args[2] ) );
		}

		AddLink( &popup->menuItems, mio );
		mio->pmi = pmi;
		mio->popup = popup;
		args.GetReturnValue().Set( menuItemObject );

	} else {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Required parameters: (type, text, onSelectCallback)" ) ) );

	}
	//AppendPopupItem( popup->popup, MF_STRING, args[2]->Int32Value(), *text );
}

static uintptr_t TrackPopupThread( PTHREAD thread ) {
	PopupObject *popup = (PopupObject *)GetThreadParam( thread );
	uintptr_t result = TrackPopup( popup->popup, popup->parent->control );
	{
		DWORD dwError = GetLastError();
		if( dwError )
			lprintf( "Track Popup Error?%d", dwError );	
	}
	INDEX idx;
	MenuItemObject *mio;
	MSG msg;
	PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );
	LIST_FORALL( popup->menuItems, idx, MenuItemObject *, mio ) {
		if( mio->uid == result ) {
			if( !mio->cbSelected.IsEmpty() )
				MakePSIEvent( NULL, true, Event_Menu_Item_Selected, mio );
		}
	}
	disableEventLoop();
	return 0;
}

void PopupObject::trackPopup( const FunctionCallbackInfo<Value>& args ) {
	PopupObject *popup = ObjectWrap::Unwrap<PopupObject>( args.This() );
	enableEventLoop();
	if( args.Length() > 0 ) {
		ControlObject *object = ObjectWrap::Unwrap<ControlObject>( args[0]->ToObject() );
		popup->parent = object;
		ThreadTo( TrackPopupThread, (uintptr_t)popup );
	} else {
		popup->parent = NULL;
		ThreadTo( TrackPopupThread, (uintptr_t)popup );
	}
}

/*
void PopupObject::closePopup( const FunctionCallbackInfo<Value>& args ) {

	PopupObject *popup = ObjectWrap::Unwrap<PopupObject>( args.This() );
	ThreadTo( TrackPopupThread, (uintptr_t)popup );
}
*/

//-------------------------------------------------------

static int CPROC onLoad( PSI_CONTROL pc, PTEXT params ) {
	Isolate* isolate = Isolate::GetCurrent();
	CTEXTSTR name = GetControlTypeName( pc );
	RegistrationObject *registration = findRegistration( name );
	ControlObject **me = ControlData( ControlObject **, pc );

	Local<Function> cb = Local<Function>::New( isolate, registration->cbLoadEvent );
	Local<Value> retval = cb->Call( psiLocal.newControl, 0, NULL );

	return retval->ToInt32(USE_ISOLATE_VOID(isolate))->Value();
}

static int CPROC onCreate( PSI_CONTROL pc ) {
	Isolate* isolate = Isolate::GetCurrent();
	CTEXTSTR name = GetControlTypeName( pc );
	RegistrationObject *registration = findRegistration( name );
	if( !psiLocal.pendingCreate ) {
		psiLocal.pendingRegistration = registration;
		int result = (int)MakePSIEvent( NULL, true, Event_Control_Create, pc );
		psiLocal.pendingRegistration = NULL;
		return result;
	}
	else {
		psiLocal.pendingCreate->registration = registration;
	
		ControlObject **me = ControlData( ControlObject **, pc );
		me[0] = psiLocal.pendingCreate;

		Local<Function> cb = Local<Function>::New( isolate, registration->cbInitEvent );
		// controls get wrapped sooner... 
		// ControlObject::wrapSelf( isolate, me[0], psiLocal.newControl );

		Local<Value> retval = cb->Call( psiLocal.newControl, 0, NULL );

		return retval->ToInt32( USE_ISOLATE_VOID( isolate ) )->Value();
	}

}

RegistrationObject::RegistrationObject() {
}

RegistrationObject::RegistrationObject( Isolate *isolate, const char *name ) {
	struct registrationOpts opts;
	memset( &opts, 0, sizeof( struct registrationOpts ) );
	opts.name = name;
	InitRegistration( isolate, &opts );
}
RegistrationObject::RegistrationObject( Isolate *isolate, struct registrationOpts *opts ) {
	InitRegistration( isolate, opts );
}


void RegistrationObject::NewRegistration( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		int argc = args.Length();
		char *title = NULL;
		if( argc > 0 ) {
			int arg = 0;
			struct registrationOpts regOpts;
			memset( &regOpts, 0, sizeof( struct registrationOpts ) );
			regOpts.width = 32;
			regOpts.height = 32;
			regOpts.default_border = BORDER_NORMAL;

			if( args[arg]->IsString() ) {
				arg++;
				String::Utf8Value name( args[0]->ToString() );
				regOpts.name = StrDup( *name );
			}
			if( args[arg]->IsObject() ) {
				Local<Object> opts = args[arg]->ToObject();
				Local<String> optName;
				struct optionStrings *strings = getStrings( isolate );
				if( opts->Has( optName = strings->nameString->Get( isolate ) ) ) {
					String::Utf8Value name( opts->Get( optName )->ToString() );
					regOpts.name = StrDup( *name );
				}
				if( opts->Has( optName = strings->widthString->Get( isolate ) ) ) {
					regOpts.width = (int)opts->Get( optName )->IntegerValue();
				}
				if( opts->Has( optName = strings->heightString->Get( isolate ) ) ) {
					regOpts.height = (int)opts->Get( optName )->IntegerValue();
				}
				if( opts->Has( optName = strings->borderString->Get( isolate ) ) ) {
					regOpts.default_border = (int)opts->Get( optName )->IntegerValue();
				}
				if( opts->Has( optName = strings->createString->Get( isolate ) ) ) {
					regOpts.cbInitEvent = Handle<Function>::Cast( opts->Get( optName ) );
				}
				if( opts->Has( optName = strings->drawString->Get( isolate ) ) ) {
					regOpts.cbDrawEvent = Handle<Function>::Cast( opts->Get( optName ) );
				}
				if( opts->Has( optName = strings->mouseString->Get( isolate ) ) ) {
					regOpts.cbMouseEvent = Handle<Function>::Cast( opts->Get( optName ) );
				}
				if( opts->Has( optName = strings->keyString->Get( isolate ) ) ) {
					regOpts.cbKeyEvent = Handle<Function>::Cast( opts->Get( optName ) );
				}
				if( opts->Has( optName = strings->destroyString->Get( isolate ) ) ) {
					regOpts.cbDestroyEvent = Handle<Function>::Cast( opts->Get( optName ) );
				}
			}

			// Invoked as constructor: `new MyObject(...)`
			RegistrationObject* obj = new RegistrationObject( isolate, &regOpts );
			obj->_this.Reset( isolate, args.This() );
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

		Local<Function> cons = Local<Function>::New( isolate, ControlObject::registrationConstructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete argv;
	}
}

RegistrationObject::~RegistrationObject() {
}


RegistrationObject *findRegistration( CTEXTSTR name ) {
	RegistrationObject *obj;
	INDEX idx;
	LIST_FORALL( psiLocal.registrations, idx, RegistrationObject *, obj ) {
		if( StrCmp( obj->r.name, name ) == 0 )
			break;
	}
	return obj;
}


void RegistrationObject::setCreate( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RegistrationObject *obj = ObjectWrap::Unwrap<RegistrationObject>( args.This() );
	TEXTCHAR buf[256];
	//lprintf( "Define Create Control %s", obj->r.name );
	snprintf( buf, 256, "psi/control/%s/rtti", obj->r.name );
	SimpleRegisterMethod( buf,  onCreate
							  , "int", "init", "(PSI_CONTROL)" );
	obj->cbInitEvent.Reset( isolate, Handle<Function>::Cast( args[0] ) );
}

//-------------------------------------------------------

static int CPROC onDraw( PSI_CONTROL pc ) {
	CTEXTSTR name = GetControlTypeName( pc );
	RegistrationObject *obj = findRegistration( name );
	ControlObject **me = ControlData( ControlObject **, pc );

	return (int)MakePSIEvent( me[0], true, Event_Control_Draw, obj, me );
}

void RegistrationObject::setDraw( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RegistrationObject *obj = ObjectWrap::Unwrap<RegistrationObject>( args.This() );

	TEXTCHAR buf[256];
	//lprintf( "Define Control Draw %s", obj->r.name );
	snprintf( buf, 256, "psi/control/%s/rtti", obj->r.name );
	SimpleRegisterMethod( buf,  onDraw
							  , "int", "draw", "(PSI_CONTROL)" );

	obj->cbDrawEvent.Reset( isolate, Handle<Function>::Cast( args[0] ) );

}

//-------------------------------------------------------


static int CPROC cbMouse( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b ) {
	CTEXTSTR name = GetControlTypeName( pc );
	RegistrationObject *obj = findRegistration( name );
	ControlObject **me = ControlData( ControlObject **, pc );

	return (int)MakePSIEvent( me[0], true, Event_Control_Mouse, x, y, b );
}

void RegistrationObject::setMouse( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RegistrationObject *obj = ObjectWrap::Unwrap<RegistrationObject>( args.This() );

	TEXTCHAR buf[256];
	//lprintf( "Define Control Mouse %s", obj->r.name );
	snprintf( buf, 256, "psi/control/%s/rtti", obj->r.name );
	SimpleRegisterMethod( buf, cbMouse
							  , "int", "mouse", "(PSI_CONTROL,int32_t,int32_t,uint32_t)" );

	obj->cbMouseEvent.Reset( isolate, Handle<Function>::Cast( args[0] ) );

}



static int CPROC cbKey( PSI_CONTROL pc, uint32_t key ) {
	CTEXTSTR name = GetControlTypeName( pc );
	RegistrationObject *obj = findRegistration( name );
	ControlObject **me = ControlData( ControlObject **, pc );

	return (int)MakePSIEvent( me[0], true, Event_Control_Key, key );
}

void RegistrationObject::setKey( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
}
void RegistrationObject::setTouch( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
}

static int onTouch( PSI_CONTROL pc, PINPUT_POINT pPoints, int nPoints ) {
	return 0;
}

void RegistrationObject::InitRegistration( Isolate *isolate, struct registrationOpts *opts ) {
	//memcpy( this, &_blankRegistration, sizeof( *this ) );
	//MemSet( &r, 0, sizeof( r ) );
	r.name = opts->name;
#define setMethod( a, b, c )  if( !opts->a.IsEmpty() ) { \
		r.b = c; \
		this->a.Reset( isolate, opts->a ); \
	}

	setMethod( cbInitEvent, init, onCreate );
	setMethod( cbLoadEvent, load, onLoad );
	setMethod( cbMouseEvent, mouse, cbMouse );
	setMethod( cbKeyEvent, key, cbKey );
	setMethod( cbDrawEvent, draw, onDraw );

	r.stuff.stuff.width = opts->width;
	r.stuff.stuff.height = opts->height;
	r.stuff.extra = sizeof( uintptr_t );
	r.stuff.default_border = opts->default_border;
	AddLink( &psiLocal.registrations, this );
	DoRegisterControl( &r );

	char buf[256];
	if( !opts->cbTouchEvent.IsEmpty() ) {
		snprintf( buf, 256, "psi/control/%s/rtti", r.name );
		SimpleRegisterMethod( buf, onTouch
			, "int", "touch_event", "(PSI_CONTROL,PINPUT_POINT,int)" );
	}


}

