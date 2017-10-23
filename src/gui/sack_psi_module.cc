
#include "../global.h"

#include <psi.h>
#include <psi/console.h>

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
Persistent<Function> ControlObject::constructor3;

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
		check->nameString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "name" ) );
		check->widthString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "width" ) );
		check->heightString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "height" ) );
		check->borderString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "border" ) );
		check->createString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "create" ) );
		check->mouseString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "mouse" ) );
		check->drawString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "draw" ) );
		check->keyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "key" ) );
		check->destroyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "destroy" ) );
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
	}

	e.waiter = MakeThread();
	e.flags.complete = 0; 
	e.success = 0;
	EnqueLink( &psiLocal.events, &e );
	uv_async_send( &psiLocal.async );

	while( !e.flags.complete ) WakeableSleep( 1000 );

	return e.success;
}


static void enableEventLoop( void ) {
	if( !psiLocal.eventLoopRegistered ) {
		psiLocal.eventLoopRegistered = TRUE;
		MemSet( &psiLocal.async, 0, sizeof( &psiLocal.async ) );
		uv_async_init( uv_default_loop(), &psiLocal.async, asyncmsg );
	}
}

void ControlObject::Init( Handle<Object> exports ) {

		Isolate* isolate = Isolate::GetCurrent();
		Local<FunctionTemplate> psiTemplate;
		Local<FunctionTemplate> psiTemplate2;
		Local<FunctionTemplate> psiTemplate3;

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
		psiTemplate3 = FunctionTemplate::New( isolate, RegistrationObject::NewRegistration );
		psiTemplate3->SetClassName( String::NewFromUtf8( isolate, "sack.PSI.Registration" ) );
		psiTemplate3->InstanceTemplate()->SetInternalFieldCount( 1 );// 1 internal field for wrap

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
		SET_READONLY( controlObject, "types", controlTypes );

		Local<Object> controlColors = Object::New( isolate );

		//controlColors->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8( isolate, name ), data, ReadOnlyProperty )
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "highlight" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, HIGHLIGHT ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "normal" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, NORMAL ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "shade" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, SHADE ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "shadow" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, SHADOW ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "textColor" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, TEXTCOLOR ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "caption" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, CAPTION ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "captionText" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, CAPTIONTEXTCOLOR ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "inactiveCaption" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, INACTIVECAPTION ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "InactiveCaptionText" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, INACTIVECAPTIONTEXTCOLOR ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "selectBack" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, SELECT_BACK ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "selectText" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, SELECT_TEXT ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "editBackground" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, EDIT_BACKGROUND ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "editText" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, EDIT_TEXT ) );
		controlColors->SetNativeDataProperty( isolate->GetCallingContext()
			, String::NewFromUtf8( isolate, "scrollBarBackground" )
			, ControlObject::getControlColor, ControlObject::setControlColor, Integer::New( isolate, SCROLLBAR_BACK ) );

		SET_READONLY( controlObject, "color", controlColors );

		//psiTemplate2->Set( isolate, "color", controlColors );
		//psiTemplate->Set( isolate, "color", controlColors );

		NODE_SET_PROTOTYPE_METHOD( psiTemplate3, "setCreate", RegistrationObject::setCreate );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate3, "setDraw", RegistrationObject::setDraw );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate3, "setMouse", RegistrationObject::setMouse );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate3, "setKey", RegistrationObject::setKey );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate3, "setTouch", RegistrationObject::setTouch );

		// Prototype
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "Control", ControlObject::NewControl );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "Frame", ControlObject::createFrame );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "show", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "hide", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "reveal", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate, "close", ControlObject::show );


		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "Control", ControlObject::NewControl );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "createFrame", ControlObject::createFrame );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "createControl", ControlObject::NewControl );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "show", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "hide", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "reveal", ControlObject::show );
		NODE_SET_PROTOTYPE_METHOD( psiTemplate2, "redraw", ControlObject::redraw );

		constructor.Reset( isolate, psiTemplate->GetFunction() );
		exports->Set( String::NewFromUtf8( isolate, "Frame" ),
			psiTemplate->GetFunction() );

		constructor2.Reset( isolate, psiTemplate2->GetFunction() );
		//exports->Set( String::NewFromUtf8( isolate, "Control" ),
		//				 psiTemplate2->GetFunction() );

		constructor3.Reset( isolate, psiTemplate3->GetFunction() );
		exports->Set( String::NewFromUtf8( isolate, "Registration" ),
			psiTemplate3->GetFunction() );
}


void ControlObject::getControlColor( v8::Local<v8::Name> field,
		const PropertyCallbackInfo<v8::Value>& args ) {
	int colorIndex = (int)args.Data()->IntegerValue();
	Isolate* isolate = args.GetIsolate();
	Local<FunctionTemplate> controlTpl = controlTemplate.Get( isolate );
	Local<Object> object = args.This();
	if( controlTpl->HasInstance( object ) ) {
		ControlObject *c = ObjectWrap::Unwrap<ControlObject>( object );
		args.GetReturnValue().Set( ColorObject::makeColor( isolate, GetControlColor( c->control, colorIndex ) ) );
	}
	else
		args.GetReturnValue().Set( ColorObject::makeColor( isolate, GetControlColor( NULL, colorIndex ) ) );
}
void ControlObject::setControlColor( v8::Local<v8::Name> field,
	Local<Value> value,
	const PropertyCallbackInfo<void>& args ) {
	int colorIndex = (int)args.Data()->IntegerValue();
	Isolate* isolate = args.GetIsolate();
	Local<FunctionTemplate> controlTpl = controlTemplate.Get( isolate );
	Local<Object> object = args.This();
	CDATA newColor;

	Local<Object> color = value->ToObject();
	if( ColorObject::isColor( isolate, color ) ) {
		newColor = ColorObject::getColor( color );
	}
	else {
		newColor = (uint32_t)value->NumberValue();
	}

	if( controlTpl->HasInstance( object ) ) {
		ControlObject *c = ObjectWrap::Unwrap<ControlObject>( object );
		SetControlColor( c->control, colorIndex, newColor );
	}
	else
		SetControlColor( NULL, colorIndex, newColor );

	//Isolate* isolate = args.GetIsolate();

}


ControlObject::ControlObject( ControlObject *over, const char *type, const char *title, int x, int y, int w, int h ) {
	image = NULL;
	frame = over;
	psiLocal.pendingCreate = this;
	if( !title )
		control = MakeNamedControl( over->control, type, x, y, w, h, 0 );
	else
		control = MakeNamedCaptionedControl( over->control, type, x, y, w, h, 0, title );
}

ControlObject::ControlObject( const char *title, int x, int y, int w, int h, int border, ControlObject *over ) {
	image = NULL;
	frame = over;
	control = ::CreateFrame( title, x, y, w, h, border, over ? over->control : (PSI_CONTROL)NULL );
}

ControlObject::ControlObject( const char *type, ControlObject *parent, int32_t x, int32_t y, uint32_t w, uint32_t h ) {
	image = NULL;
	psiLocal.pendingCreate = this;
	control = ::MakeNamedControl( parent->control, type, x, y, w, h, -1 );
}


ControlObject::~ControlObject() {

}
ControlObject::ControlObject() {
	image = NULL;
	frame = NULL;
	control = NULL;
}


void ControlObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	enableEventLoop();
	if( args.IsConstructCall() ) {
		if( args.Length() == 0 ) {
			args.GetReturnValue().Set( args.This() );
			return;
		}

		char *type;
		char *title = "Node Application";
		char *tmpTitle = NULL;
		int x = 0, y = 0, w = 1024, h = 768, border = 0;
		int arg_ofs = 0;
		ControlObject *parent = NULL;

		int argc = args.Length();
		if( argc > 0 ) {
			String::Utf8Value fName( args[0]->ToString() );
			type = StrDup( *fName );
			arg_ofs++;
		}
		else {
			ControlObject* obj = new ControlObject( );
			ControlObject::wrapSelf( isolate, obj, args.This() );
			args.GetReturnValue().Set( args.This() );
			return;
		}
		if( argc > (arg_ofs+0) ) {
			if( args[arg_ofs + 0]->IsString() ) {
				String::Utf8Value fName( args[arg_ofs + 0]->ToString() );
				if( tmpTitle )
					Deallocate( char*, title );
				tmpTitle = title = StrDup( *fName );
				arg_ofs++;
			}
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

void ControlObject::setConsoleRead( const FunctionCallbackInfo<Value>& args ) {
	ControlObject *c = ObjectWrap::Unwrap<ControlObject>( args.This() );
	if( args.Length() > 0 )
	{
		Isolate* isolate = args.GetIsolate();
		c->cbConsoleRead.Reset( isolate, Handle<Function>::Cast( args[0] ) );
		//args[0]->ToFunction
		//PSIConsoleInputEvent( c->control,
	}
}

static void ProvideKnownCallbacks( Isolate *isolate, Local<Object>c, ControlObject *obj ) {
	CTEXTSTR type = GetControlTypeName( obj->control );
	if( StrCmp( type, "PSI Console" ) == 0 ) {
		c->Set( String::NewFromUtf8( isolate, "write" ), Function::New( isolate, ControlObject::writeConsole ) );
		c->Set( String::NewFromUtf8( isolate, "setRead" ), Function::New( isolate, ControlObject::setConsoleRead ) );
	}
	else if( StrCmp( type, NORMAL_BUTTON_NAME ) == 0 ) {

	}
	else if( StrCmp( type, IMAGE_BUTTON_NAME ) == 0 ) {

	}
	else if( StrCmp( type, CUSTOM_BUTTON_NAME ) == 0 ) {

	}
	else if( StrCmp( type, RADIO_BUTTON_NAME ) == 0 ) {

	}
	else if( StrCmp( type, SCROLLBAR_CONTROL_NAME ) == 0 ) {

	}
}

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

				ProvideKnownCallbacks( isolate, args.This(), obj );

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

			Local<Function> cons = Local<Function>::New( isolate, ControlObject::constructor3 );
			args.GetReturnValue().Set( cons->NewInstance( argc, argv ) );
			delete argv;
		}
	}

RegistrationObject::~RegistrationObject() {
}


static RegistrationObject *findRegistration( CTEXTSTR name ) {
	RegistrationObject *obj;
	INDEX idx;
	LIST_FORALL( psiLocal.registrations, idx, RegistrationObject *, obj ) {
		if( StrCmp( obj->r.name, name ) == 0 )
			break;
	}
	return obj;
}

//-------------------------------------------------------

void ControlObject::wrapSelf( Isolate* isolate, ControlObject *_this, Local<Object> into ) {
	_this->Wrap( into );
	_this->state.Reset( isolate, into );
}

//-------------------------------------------------------

void ControlObject::releaseSelf( ControlObject *_this ) {
	_this->state.Reset();
}

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
	MemSet( &r, 0, sizeof( r ) );
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


