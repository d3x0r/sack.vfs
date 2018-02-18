
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

static struct psiLocal {
	PLIST registrations;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE events;
	PTHREAD nodeThread;
	ControlObject *pendingCreate;
	Local<Object> newControl;
	LOGICAL eventLoopRegistered;
} psiLocal;

Persistent<FunctionTemplate> ControlObject::controlTemplate;
Persistent<Function> ControlObject::constructor;
Persistent<Function> ControlObject::constructor2;
Persistent<Function> ControlObject::registrationConstructor;
Persistent<Function> PopupObject::constructor;
Persistent<Function> ListboxItemObject::constructor;
Persistent<Function> MenuItemObject::constructor;

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
			case Event_Control_Draw: {
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
		}
	}
	//lprintf( "done calling message notice." );
}


int MakePSIEvent( ControlObject *control, enum eventType type, ... ) {
	event e;
	va_list args;
	va_start( args, type );
	e.type = type;
	e.control = control;
	//e.registration = control->reg
	switch( type ) {
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
	}

	e.waiter = MakeThread();
	e.flags.complete = 0; 
	e.success = 0;
	EnqueLink( &psiLocal.events, &e );
	uv_async_send( &psiLocal.async );

	while( !e.flags.complete ) WakeableSleep( 1000 );

	return e.success;
}


void enableEventLoop( void ) {
	if( !psiLocal.eventLoopRegistered ) {
		psiLocal.eventLoopRegistered = TRUE;
		MemSet( &psiLocal.async, 0, sizeof( &psiLocal.async ) );
		uv_async_init( uv_default_loop(), &psiLocal.async, asyncmsg );
	}
}

void disableEventLoop( void ) {
	if( psiLocal.eventLoopRegistered ) {
		psiLocal.eventLoopRegistered = FALSE;
		MakePSIEvent( NULL, Event_Control_Close_Loop );
	}
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

void ControlObject::Init( Handle<Object> _exports ) {

		Isolate* isolate = Isolate::GetCurrent();
		Local<FunctionTemplate> psiTemplate;
		Local<FunctionTemplate> psiTemplate2;
		Local<FunctionTemplate> psiTemplatePopups;
		Local<FunctionTemplate> regTemplate;
		Local<FunctionTemplate> listItemTemplate;
		Local<FunctionTemplate> menuItemTemplate;
		Handle<Object> exports = Object::New( isolate );
		_exports->Set( String::NewFromUtf8( isolate, "PSI" ), exports );
		// Prepare constructor template
		psiTemplate = FunctionTemplate::New( isolate, New );
		psiTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Frame" ) );
		psiTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 internal field for wrap

		// Prepare constructor template
		psiTemplate2 = FunctionTemplate::New( isolate, NewControl );
		controlTemplate.Reset( isolate, psiTemplate2 );
		psiTemplate2->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Control" ) );
		psiTemplate2->InstanceTemplate()->SetInternalFieldCount( 1 );// 1 internal field for wrap

		// Prepare constructor template
		psiTemplatePopups = FunctionTemplate::New( isolate, PopupObject::NewPopup );
		controlTemplate.Reset( isolate, psiTemplatePopups );
		psiTemplatePopups->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Popup" ) );
		psiTemplatePopups->InstanceTemplate()->SetInternalFieldCount( 1 );// 1 internal field for wrap

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

		// Prototype
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "Control", ControlObject::NewControl );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "Frame", ControlObject::createFrame );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "show", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "hide", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "reveal", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "close", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "edit", ControlObject::edit );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "save", ControlObject::save );


		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "Control", ControlObject::NewControl );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "createFrame", ControlObject::createFrame );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "createControl", ControlObject::NewControl );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "show", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "hide", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "reveal", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "redraw", ControlObject::redraw );



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
			, FunctionTemplate::New( isolate, ControlObject::setCoordinate, Integer::New( isolate, 10 ) )
			, DontDelete );
		psiTemplate2->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "position" )
			, FunctionTemplate::New( isolate, ControlObject::getCoordinate, Integer::New( isolate, 12 ) )
			, FunctionTemplate::New( isolate, ControlObject::setCoordinate, Integer::New( isolate, 11 ) )
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
	else
		args.GetReturnValue().Set( ColorObject::makeColor( isolate, GetControlColor( NULL, colorIndex ) ) );
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


	Local<FunctionTemplate> controlTpl = controlTemplate.Get( isolate );
	CDATA newColor;
	Local<Object> color = args[0]->ToObject();
	if( ColorObject::isColor( isolate, color ) ) {
		newColor = ColorObject::getColor( color );
	}
	else {
		newColor = (uint32_t)args[0]->NumberValue();
	}

	if( controlTpl->HasInstance( object ) ) {
		ControlObject *c = ObjectWrap::Unwrap<ControlObject>( object );
		SetControlColor( c->control, colorIndex, newColor );
	}
	else
		SetControlColor( NULL, colorIndex, newColor );
}



ControlObject::ControlObject( ControlObject *over, const char *type, const char *title, int x, int y, int w, int h ) {
	memcpy( this, &_blankObject, sizeof( *this ) );
	image = NULL;
	frame = over;
	psiLocal.pendingCreate = this;
	if( !title )
		control = MakeNamedControl( over->control, type, x, y, w, h, 0 );
	else
		control = MakeNamedCaptionedControl( over->control, type, x, y, w, h, 0, title );
}

ControlObject::ControlObject( const char *title, int x, int y, int w, int h, int border, ControlObject *over ) {
	memcpy( this, &_blankObject, sizeof( *this ) );
	image = NULL;
	frame = over;
	control = ::CreateFrame( title, x, y, w, h, border, over ? over->control : (PSI_CONTROL)NULL );
}

ControlObject::ControlObject( const char *type, ControlObject *parent, int32_t x, int32_t y, uint32_t w, uint32_t h ) {
	memcpy( this, &_blankObject, sizeof( *this ) );
	image = NULL;
	psiLocal.pendingCreate = this;
	control = ::MakeNamedControl( parent->control, type, x, y, w, h, -1 );
}


ControlObject::~ControlObject() {

}
ControlObject::ControlObject() {
	memcpy( this, &_blankObject, sizeof( *this ) );
	image = NULL;
	frame = NULL;
	control = NULL;
}

/* this is constructor of sack.PSI.Frame */
void ControlObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	enableEventLoop();
	if( args.IsConstructCall() ) {
		if( args.Length() == 0 ) {
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
		args.GetReturnValue().Set( cons->NewInstance( argc, argv ) );
		delete argv;
	}
}


Local<Object> ControlObject::NewWrappedControl( Isolate* isolate, PSI_CONTROL pc ) {
	// Invoked as plain function `MyObject(...)`, turn into construct call.

	Local<Function> cons = Local<Function>::New( isolate, constructor2 );

	Local<Object> c = cons->NewInstance( 0, 0 );
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( c );
	me->control = pc;
	return c;
}


void ControlObject::writeConsole( const FunctionCallbackInfo<Value>& args) {
	ControlObject *c = ObjectWrap::Unwrap<ControlObject>( args.This() );
	if( args.Length() > 0 )
	{
			String::Utf8Value fName( args[0]->ToString() );
			pcprintf( c->control, "%s", (const char*)*fName );
	}
}

static void consoleInputEvent( uintptr_t arg, PTEXT text ) {
	MakePSIEvent( (ControlObject*)arg, Event_Control_ConsoleInput, text );
}

void ControlObject::setConsoleRead( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	//Local<FunctionTemplate> tpl = ControlObject::controlTemplate.Get( isolate );
	ControlObject *c = NULL;
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
	MakePSIEvent( (ControlObject*)object, Event_Control_ButtonClick );
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

static void ProvideKnownCallbacks( Isolate *isolate, Local<Object>c, ControlObject *obj ) {
	CTEXTSTR type = GetControlTypeName( obj->control );
	if( StrCmp( type, "PSI Console" ) == 0 ) {
		c->Set( String::NewFromUtf8( isolate, "write" ), Function::New( isolate, ControlObject::writeConsole ) );
		c->Set( String::NewFromUtf8( isolate, "oninput" ), Function::New( isolate, ControlObject::setConsoleRead ) );
	} else if( StrCmp( type, NORMAL_BUTTON_NAME ) == 0 ) {
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

	} else if( StrCmp( type, LISTBOX_CONTROL_NAME ) == 0 ) {
		c->Set( String::NewFromUtf8( isolate, "addItem" ), Function::New( isolate, ControlObject::addListboxItem ) );
		c->Set( String::NewFromUtf8( isolate, "removeItem" ), Function::New( isolate, ListboxItemObject::removeListboxItem ) );
		c->Set( String::NewFromUtf8( isolate, "onSelect" ), Function::New( isolate, ControlObject::setListboxOnSelect ) );
		c->Set( String::NewFromUtf8( isolate, "onDoubleClick" ), Function::New( isolate, ControlObject::setListboxOnDouble ) );
	} else if( StrCmp( type, SHEET_CONTROL_NAME ) == 0 ) {
		c->Set( String::NewFromUtf8( isolate, "addPage" ), Function::New( isolate, ControlObject::addSheetsPage ) );

	} else if( StrCmp( type, PROGRESS_BAR_CONTROL_NAME ) == 0 ) {
		c->SetAccessorProperty( String::NewFromUtf8( isolate, "range" )
			, Local<Function>()
			, Function::New( isolate, ControlObject::setProgressBarRange )
			, DontDelete );
		c->SetAccessorProperty( String::NewFromUtf8( isolate, "progress" )
			, Local<Function>()
			, Function::New( isolate, ControlObject::setProgressBarProgress )
			, DontDelete );
		c->SetAccessorProperty( String::NewFromUtf8( isolate, "colors" )
			, Local<Function>()
			, Function::New( isolate, ControlObject::setProgressBarColors )
			, DontDelete );
		c->SetAccessorProperty( String::NewFromUtf8( isolate, "text" )
			, Local<Function>()
			, Function::New( isolate, ControlObject::setProgressBarTextEnable )
			, DontDelete );


	} else if( StrCmp( type, "Basic Clock Widget" ) == 0 ) {
		c->Set( String::NewFromUtf8( isolate, "analog" ), Function::New( isolate, ControlObject::makeAnalog ) );
		
	}
}

/* this is constructor of sack.PSI.Control (also invoked as frame.Control */
void ControlObject::NewControl( const FunctionCallbackInfo<Value>& args ) {
	int argc = args.Length();
	Isolate* isolate = args.GetIsolate();
	if( argc == 0 ) {
		if( args.IsConstructCall() )
			args.GetReturnValue().Set( args.This() );
		else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Required parameter 'controlType' missing." ) ) );
		return;
	}

	char *type = NULL;
	char *title = NULL;
	int x = 0, y = 0, w = 1024, h = 768, border = 0;
	int argOffset = 0;
	ControlObject *parent = NULL;


		Local<Function> cons = Local<Function>::New( isolate, constructor2 );
		Local<Object> newControl = cons->NewInstance( 0, NULL );
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
				ControlObject* obj = new ControlObject( type, container, x, y, w, h );
				ControlObject::wrapSelf( isolate, obj, newControl );

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
		ProvideKnownCallbacks( isolate, newControl, obj );

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
		args.GetReturnValue().Set( cons->NewInstance( argc, argv ) );
		delete argv;
	}
}

void ControlObject::redraw( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );

	SmudgeCommon( me->control );

}

void ControlObject::show( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );

	DisplayFrame( me->control );
}

void ControlObject::edit( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	EditFrame( me->control, 1 );
}

void ControlObject::save( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	String::Utf8Value name( args[0]->ToString() );
	SaveXMLFrame( me->control, *name );
}

//-------------------------------------------------------

void ControlObject::wrapSelf( Isolate* isolate, ControlObject *_this, Local<Object> into ) {
	_this->Wrap( into );
	_this->state.Reset( isolate, into );
	SetupControlColors( isolate, into );
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
	CDATA c2 = ColorObject::getColor( args[0]->ToObject() );
	ProgressBar_SetColors( me->control, c1, c2 );
}
void ControlObject::setProgressBarTextEnable( const FunctionCallbackInfo<Value>&  args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	ProgressBar_EnableText( me->control, args[0]->Int32Value() );
}

//-------------------------------------------------------
// LISTBOX
//-----------------------------------------------------------

//InsertListItem
void ControlObject::addListboxItem( const FunctionCallbackInfo<Value>&  args ) {
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	String::Utf8Value text( args[0]->ToString() );

	Isolate* isolate = args.GetIsolate();
	Local<Function> cons = Local<Function>::New( isolate, ListboxItemObject::constructor );
	Local<Object> lio = cons->NewInstance( 0, NULL );
	ListboxItemObject *pli = ObjectWrap::Unwrap<ListboxItemObject>( lio );

	pli->pli = AddListItem( me->control, *text );
	SetItemData( pli->pli, (uintptr_t)pli );
	pli->control = me;
	args.GetReturnValue().Set( lio );

}

static void DoubleClickHandler( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM hli ){
	ControlObject *me = (ControlObject *)psvUser;
	MakePSIEvent( me, Event_Listbox_DoubleClick, (MenuItemObject*)GetItemData( hli ) );
}

void ControlObject::setListboxOnDouble( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	me->listboxOnDouble.Reset( isolate, Handle<Function>::Cast( args[0] ) );
	SetDoubleClickHandler( me->control, DoubleClickHandler, (uintptr_t)me );
		
}

static void SelChangeHandler( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM hli ) {
	ControlObject *me = (ControlObject *)psvUser;
	MakePSIEvent( me, Event_Listbox_Selected, ((MenuItemObject*)GetItemData( hli )) );
}

void ControlObject::setListboxOnSelect( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	ControlObject *me = ObjectWrap::Unwrap<ControlObject>( args.This() );
	me->listboxOnSelect.Reset( isolate, Handle<Function>::Cast( args[0] ) );
	SetSelChangeHandler( me->control, SelChangeHandler, (uintptr_t)me );
}

void ListboxItemObject::removeListboxItem( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	ListboxItemObject *me = ObjectWrap::Unwrap<ListboxItemObject>( args.This() );
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
		args.GetReturnValue().Set( cons->NewInstance( 0, NULL ) );
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
		args.GetReturnValue().Set( cons->NewInstance( 0, NULL ) );
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
		args.GetReturnValue().Set( cons->NewInstance( 0, NULL ) );
	}
}

void PopupObject::addPopupItem( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() > 0 )
	{
		Local<Function> cons = Local<Function>::New( isolate, MenuItemObject::constructor );
		Local<Object> menuItemObject = cons->NewInstance( NULL, 0 );
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
				MakePSIEvent( NULL, Event_Menu_Item_Selected, mio );
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

	return retval->ToInt32()->Value();
}

static int CPROC onCreate( PSI_CONTROL pc ) {
	Isolate* isolate = Isolate::GetCurrent();
	CTEXTSTR name = GetControlTypeName( pc );
	RegistrationObject *registration = findRegistration( name );
	psiLocal.pendingCreate->registration = registration;
	ControlObject **me = ControlData( ControlObject **, pc );
	me[0] = psiLocal.pendingCreate;

	Local<Function> cb = Local<Function>::New( isolate, registration->cbInitEvent );
	ControlObject::wrapSelf( isolate, me[0], psiLocal.newControl );

	Local<Value> retval = cb->Call( psiLocal.newControl, 0, NULL );

	return retval->ToInt32()->Value();
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
		args.GetReturnValue().Set( cons->NewInstance( argc, argv ) );
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
	lprintf( "Define Create Control %s", obj->r.name );
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

	return MakePSIEvent( me[0], Event_Control_Draw, obj, me );
}

void RegistrationObject::setDraw( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RegistrationObject *obj = ObjectWrap::Unwrap<RegistrationObject>( args.This() );

	TEXTCHAR buf[256];
	lprintf( "Define Create Control %s", obj->r.name );
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

	return MakePSIEvent( me[0], Event_Control_Mouse, x, y, b );
}

void RegistrationObject::setMouse( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RegistrationObject *obj = ObjectWrap::Unwrap<RegistrationObject>( args.This() );

	TEXTCHAR buf[256];
	lprintf( "Define Create Control %s", obj->r.name );
	snprintf( buf, 256, "psi/control/%s/rtti", obj->r.name );
	SimpleRegisterMethod( buf, cbMouse
							  , "int", "mouse", "(PSI_CONTROL,int32_t,int32_t,uint32_t)" );

	obj->cbMouseEvent.Reset( isolate, Handle<Function>::Cast( args[0] ) );

}



static int CPROC cbKey( PSI_CONTROL pc, uint32_t key ) {
	CTEXTSTR name = GetControlTypeName( pc );
	RegistrationObject *obj = findRegistration( name );
	ControlObject **me = ControlData( ControlObject **, pc );

	return MakePSIEvent( me[0], Event_Control_Key, key );
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
	memcpy( this, &_blankRegistration, sizeof( *this ) );
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

