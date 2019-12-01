#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE

#include "../global.h"
#include "InterShell/intershell_registry.h"
#include "InterShell/intershell_export.h"

//#include 

struct optionStrings {
	Isolate *isolate;

	Eternal<String> *nameString;
	Eternal<String> *loadString;
	Eternal<String> *saveString;
	Eternal<String> *drawString;
	Eternal<String> *createString;
	Eternal<String> *mouseString;
	Eternal<String> *controlRegistrationString;
	Eternal<String> *keyString;
	Eternal<String> *destroyString;
	Eternal<String> *sizeString;
	Eternal<String> *posString;
	Eternal<String> *layoutString;
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
#define makeString(a,b) check->a##String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, b, v8::NewStringType::kNormal ).ToLocalChecked() );
		makeString( name, "name" );
		makeString( save, "save" );
		makeString( load, "load" );
		makeString( create, "create" );
		makeString( destroy, "destroy" );
		makeString( mouse, "mouse" );
		makeString( draw, "draw" );
		makeString( size, "size" );
		makeString( layout, "layout" );
		makeString( controlRegistration, "control" );
		//check->String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "" ) );
		AddLink( &strings, check );
	}
	return check;
}



static struct isLocal {
	PLIST controlTypes;
	InterShellObject *core;
	PLIST parents;
	PSI_CONTROL canvas;
	PSI_CONTROL creating_parent;
	PIS_EVENTSET events;
}isLocal;


//-----------------------------------------------------------
//   InterShell Object
//-----------------------------------------------------------
int MakeISEvent( uv_async_t *async, PLINKQUEUE *queue, enum GUI_eventType type, ... );

static void defineCreateButton( char *name );
static void defineCreateControl( char *name );
static void defineButtonPress( char *name );
static void defineOnQueryShowControl( char *name );
static void defineOnQueryControl( char *name );

static InterShellObject *findControlType( CTEXTSTR type ) {
	INDEX idx;
	InterShellObject *o;
	LIST_FORALL( isLocal.controlTypes, idx, InterShellObject*, o ) {
		if( StrCaseCmp( type, o->name ) == 0 )
			break;
	}
	return o;
}



static PSI_CONTROL getCanvas( Isolate *isolate, PMENU_BUTTON button ) {
	if( !isLocal.canvas ) {
		class constructorSet* c = getConstructors( isolate );
		isLocal.canvas = InterShell_GetButtonCanvas( button );
		if( isLocal.creating_parent != isLocal.canvas )
			DebugBreak();
		c->canvasObject.Reset( isolate, ControlObject::NewWrappedControl( isolate, isLocal.canvas ) );
		//AddLink( &isLocal.parents, )
	}
	return isLocal.canvas;
}

static void asyncmsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	Local<Context> context = isolate->GetCurrentContext();
	class constructorSet* c = getConstructors( isolate );
	HandleScope scope( isolate );
	//lprintf( "async message notice. %p", myself );
	{
		struct event *evt;

		while( evt = (struct event *)DequeLink( &isLocal.core->events ) ) {
			is_control *is = evt->data.InterShell.control;
			InterShellObject* myself = is?is->type:NULL;
			Local<Object> object;// = ProcessEvent( isolate, evt, myself );
			Local<Value> *argv = NULL;
			int argc = 0;
			Local<Function> cb;
			switch( evt->type ) {
			case Event_Intershell_Quit:
				extern void disableEventLoop( class constructorSet *c );
				disableEventLoop( c );
				uv_close( (uv_handle_t*)&isLocal.core->async, NULL );
				DeleteFromSet( IS_EVENT, isLocal.events, evt );
				return;
			case Event_Intershell_CreateControl:
			case Event_Intershell_CreateButton:
				{
					///object = Local<Object>::New( isolate, myself->surface );
					Local<Function> cons = Local<Function>::New( isolate
							, is->isButton
								? c->InterShellObject_buttonInstanceConstructor 
								: c->InterShellObject_controlInstanceConstructor
						);
					Local<Object> inst = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
					is->psvControl.Reset( isolate, inst );


					Local<Value> _argv[] = { inst  };
					argv = _argv;
					argc = 1;
					if( !myself->cbCreate.IsEmpty() ) {
						cb = Local<Function>::New( isolate, myself->cbCreate );
						Local<Value> r = cb->Call( context, inst, argc, argv ).ToLocalChecked();
						is->psvData.Reset( isolate, r->ToObject(context).ToLocalChecked() );
						evt->success = !r->IsNull() && !r->IsUndefined();
					}
					else
						evt->success = 1;
				}
				break;
			case Event_Intershell_CreateCustomControl:
			{
				///object = Local<Object>::New( isolate, myself->surface );
				Local<Function> cons = Local<Function>::New( isolate
					, c->InterShellObject_customControlInstanceConstructor
				);

				Local<Object> inst = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
				is->psvControl.Reset( isolate, inst );
				//defineOnQueryControl( is->type->name );
				getCanvas( isolate, is->button );
				SET( inst, "parent", Local<Object>::New( isolate, c->canvasObject ) );
				SET( inst, "x" , Number::New( isolate, evt->data.createCustomControl.x ) );
				SET( inst, "y" , Number::New( isolate, evt->data.createCustomControl.y ) );
				SET( inst, "w" , Number::New( isolate, evt->data.createCustomControl.w ) );
				SET( inst, "h" , Number::New( isolate, evt->data.createCustomControl.h ) );
				Local<Value> _argv[] = { inst };
				argv = _argv;
				argc = 1;
				if( !myself->cbCreate.IsEmpty() ) {
					cb = Local<Function>::New( isolate, myself->cbCreate );
					Local<Value> r = cb->Call( context, inst, argc, argv ).ToLocalChecked();
					is->psvData.Reset( isolate, r->ToObject(context).ToLocalChecked() );
					evt->success = !r->IsNull() && !r->IsUndefined();
				}
				else
					evt->success = 1;
			}
			break;
			case Event_Intershell_Control_Destroy:
				{
					Local<Value> _argv[] = { is->psvData.Get( isolate ) };
					argv = _argv;
					argc = 1;
					cb = Local<Function>::New( isolate, myself->cbDestroy );
					Local<Value> r = cb->Call( context, is->psvControl.Get( isolate ), argc, argv ).ToLocalChecked();
				}
				break;
			case Event_Intershell_Control_Draw:
				if( !is->surface.IsEmpty() )
					is->surface.Reset( isolate, ImageObject::NewImage( isolate, GetControlSurface( InterShell_GetButtonControl( is->button ) ), TRUE ) );
				{
					Local<Value> _argv[] = { is->psvData.Get( isolate ), is->surface.Get( isolate ) };
					argv = _argv;
					argc = 1;
					cb = Local<Function>::New( isolate, myself->cbDraw );
					Local<Value> r = cb->Call( context, is->psvControl.Get( isolate ), argc, argv ).ToLocalChecked();
				}
				break;
			case Event_Intershell_Control_Mouse:
				{
					static Persistent<Object> mo;
					if( mo.IsEmpty() ) {
						object = Object::New( isolate );
						mo.Reset( isolate, object );
					} else
						object = Local<Object>::New( isolate, mo );

					object->Set( context, localStringExternal( isolate, "x" ), Number::New( isolate, evt->data.mouse.x ) );
					object->Set( context, localStringExternal( isolate, "y" ), Number::New( isolate, evt->data.mouse.y ) );
					object->Set( context, localStringExternal( isolate, "b" ), Number::New( isolate, evt->data.mouse.b ) );
					{
						Local<Value> _argv[] = { is->psvData.Get( isolate ), object };
						argv = _argv;
						argc = 1;
						cb = Local<Function>::New( isolate, myself->cbMouse );
					}
					Local<Value> r = cb->Call( context, is->psvControl.Get( isolate ), argc, argv ).ToLocalChecked();
				}
				break;


			case Event_Intershell_ButtonClick:
				cb = Local<Function>::New( isolate, myself->cbClick );
				if( !cb.IsEmpty() )
				{

					if( !is->psvData.IsEmpty() ) {
						Local<Value> argv[] = { Local<Object>::New( isolate, is->psvData ) };
						Local<Value> r; r = cb->Call( context, Local<Object>::New( isolate, is->psvControl ), 1, argv ).ToLocalChecked();
						evt->success = (int)r->NumberValue(context).ToChecked();
					}
					else {
						Local<Value> r; r = cb->Call( context, Local<Object>::New( isolate, is->psvControl ), 0, NULL ).ToLocalChecked();
						evt->success = (int)r->NumberValue(context).ToChecked();
					}
				}
				break;
			//case Event_InterShell_Draw:
			//	cb = Local<Function>::New( isolate, myself->cbDraw );
			//	break;
			}

			if( evt->waiter ) {
				evt->flags.complete = TRUE;
				WakeThread( evt->waiter );
			}
		}
	}
	//lprintf( "done calling message notice." );
	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}

static void onSave( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	args.GetReturnValue().Set( True( isolate ) );
}

//-----------------------------------------------------------

static void onLoad( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	args.GetReturnValue().Set( True( isolate ) );
}

static uintptr_t startMain( PTHREAD thread )
{
	int(CPROC *Main)(int argc, TEXTCHAR **argv, int bConsole, struct volume* (CPROC *load)(CTEXTSTR filepath, CTEXTSTR userkey, CTEXTSTR devkey)
		, void (CPROC*unload)(struct volume *));
	Main = (int(CPROC*)(int, TEXTCHAR**, int, struct volume* (CPROC *load)(CTEXTSTR filepath, CTEXTSTR userkey, CTEXTSTR devkey)
		, void (CPROC*unload)(struct volume *)))GetThreadParam( thread );
	Main( 0, NULL, false, NULL, NULL );
	return 0;
}

static void start( const FunctionCallbackInfo<Value>& args ) {
	ThreadTo( startMain, (uintptr_t) LoadFunction( "intershell.core", "Main" ) );
}

typedef Local<Value> _argv[];

static void configConvertArgs( arg_list args, Local<Value> **ppargv, int *argc ) {
	int count = PARAM_COUNT( args );
	int n;
	int nBias = 0;
	POINTER buffer;
	Local<Value> *argResult = new Local<Value>[count];
	Isolate* isolate = Isolate::GetCurrent();
	Local<Array> result = Array::New( isolate );
	for( n = 0; n < count; n++ ) {
		switch( my_va_next_arg_type( args ) ) {
		case CONFIG_ARG_STRING:
			{
				PARAM( args, CTEXTSTR, string );
				argResult[n - nBias] = localStringExternal( isolate, string );
				//result->Set( n-nBias, localStringExternal( isolate, string ) );
			}
			break;
		case CONFIG_ARG_INT64:
			{
				PARAM( args, int64_t, value );
				argResult[n - nBias] = Integer::New( isolate, (int32_t)value );
				//result->Set( n-nBias, Integer::New( isolate, (int32_t)value ) );
			}
			break;
		case CONFIG_ARG_FLOAT:
			{
				PARAM( args, float, value );
				argResult[n - nBias] = Number::New( isolate, value );
				//result->Set( n-nBias, Number::New( isolate, value ) );
			}
			break;
		case CONFIG_ARG_DATA:
			{
				PARAM( args, POINTER, value );
				buffer = value;
				nBias++;
			}
			break;
		case CONFIG_ARG_DATA_SIZE:
			{
				PARAM( args, size_t, value );
				argResult[n - nBias] = ArrayBuffer::New( isolate, buffer, value );
				//result->Set( n-nBias, ArrayBuffer::New( isolate, buffer, value ) );

			}
			break;
		case CONFIG_ARG_LOGICAL:
			{
				PARAM( args, LOGICAL, value );
				if( value )
					argResult[n - nBias] = True( isolate );
				//result->Set( n-nBias, True(isolate) );
				else
					argResult[n - nBias] = False( isolate );
					//result->Set( n-nBias, False( isolate ) );
			}
			break;
		case CONFIG_ARG_FRACTION:
			{
				PARAM( args, FRACTION, value );
				//argResult[n - nBias] =
			}
			break;
		case CONFIG_ARG_COLOR:
			{
				PARAM( args, CDATA, value );
				argResult[n - nBias] = ColorObject::makeColor( isolate, value );
				//result->Set( n-nBias, ColorObject::makeColor(isolate, value) );
			}
			break;
		}
	}
	(*ppargv) = argResult;
	(*argc) = n - nBias;
}

static uintptr_t configMethod( uintptr_t psv, uintptr_t callback, arg_list args ) {
	Local<Value> *argv;
	int argc;
	Isolate* isolate = Isolate::GetCurrent();
	Persistent<Function> *pf = (Persistent<Function> *)callback;
	configConvertArgs( args, &argv, &argc );
	Local<Function> cb = pf->Get( isolate );;
	//cb->Call( context, NULL, argc, argv ).ToLocalChecked();
	delete[] argv;

	return psv;
}

void InterShellObject::Init( Local<Object> exports ) {
	if( !InterShell ) {
		//lprintf( "intershell interface not available." );
		//LoadFunction( "InterShell.core", NULL );
		//return;
	}
	Isolate* isolate = Isolate::GetCurrent();
	Local<Context> context = isolate->GetCurrentContext();
	class constructorSet* c = getConstructors( isolate );
	//Local<Object> intershellObject;

	//--------------------------------------------
	Local<FunctionTemplate> intershellTemplate;
	// Prepare constructor template
	intershellTemplate = FunctionTemplate::New( isolate, NewApplication );
	intershellTemplate->SetClassName( localStringExternal( isolate, "sack.InterShell" ) );
	intershellTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // have to be 1 internal for wrap

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( intershellTemplate, "setSave", onSave );
	NODE_SET_PROTOTYPE_METHOD( intershellTemplate, "setLoad", onLoad );
	NODE_SET_PROTOTYPE_METHOD( intershellTemplate, "start", start );

	c->InterShellObject_intershellConstructor.Reset( isolate, intershellTemplate->GetFunction(context).ToLocalChecked() );

	//Local<Function> cons = Local<Function>::New( isolate, intershellConstructor );
	//intershellObject = cons->NewInstance( 0, NULL );
	//isLocal.core = ObjectWrap::Unwrap<InterShellObject>( intershellObject );
	SET_READONLY( exports, "InterShell", intershellTemplate->GetFunction(context).ToLocalChecked() );

	//intershellTemplate->GetFunction(context).ToLocalChecked()->Set( localStringExternal( isolate, "core" ),
	//	intershellObject );

	//isLocal.canvasObject.Reset( isolate, intershellObject );

	//--------------------------------------------

	Local<FunctionTemplate> configTemplate;
	// Prepare constructor template
	configTemplate = FunctionTemplate::New( isolate, NewConfiguration );
	configTemplate->SetClassName( localStringExternal( isolate, "sack.Configuration" ) );
	configTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // have to be 1 internal for wrap

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( configTemplate, "addMethod", addConfigMethod );
	//NODE_SET_PROTOTYPE_METHOD( configTemplate, "process", processConfigFile );

	c->InterShellObject_configConstructor.Reset( isolate, configTemplate->GetFunction(context).ToLocalChecked() );
	SET_READONLY( exports, "Configuration", configTemplate->GetFunction(context).ToLocalChecked() );

	//isLocal.canvasObject.Reset( isolate, configObject );


	//--------------------------------------------
	Local<FunctionTemplate> buttonTemplate;
	// Prepare constructor template
	buttonTemplate = FunctionTemplate::New( isolate, NewButton );
	buttonTemplate->SetClassName( localStringExternal( isolate, "sack.InterShell.Button" ) );
	buttonTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // have to add 1 implicit constructor.

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( buttonTemplate, "setCreate", onCreateButton );
	NODE_SET_PROTOTYPE_METHOD( buttonTemplate, "setClick", onClickButton );
	NODE_SET_PROTOTYPE_METHOD( buttonTemplate, "setSave", onSaveButton );
	NODE_SET_PROTOTYPE_METHOD( buttonTemplate, "setLoad", onLoadButton );


	c->InterShellObject_buttonConstructor.Reset( isolate, buttonTemplate->GetFunction(context).ToLocalChecked() );
	SET_READONLY( intershellTemplate->GetFunction(context).ToLocalChecked(), "Button", buttonTemplate->GetFunction(context).ToLocalChecked() );

	//--------------------------------------------
	Local<FunctionTemplate> buttonInstanceTemplate;
	// Prepare constructor template
	buttonInstanceTemplate = FunctionTemplate::New( isolate, is_control::NewControlInstance );
	buttonInstanceTemplate->SetClassName( localStringExternal( isolate, "sack.InterShell.Button.instance" ) );
	buttonInstanceTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // needs to be 1 for wrap

	 // Prototype
	NODE_SET_PROTOTYPE_METHOD( buttonInstanceTemplate, "setTitle", setTitle );
	NODE_SET_PROTOTYPE_METHOD( buttonInstanceTemplate, "setStyle", setStyle );
	NODE_SET_PROTOTYPE_METHOD( buttonInstanceTemplate, "setTextColor", setTextColor );
	NODE_SET_PROTOTYPE_METHOD( buttonInstanceTemplate, "setBackground", setBackground );
	NODE_SET_PROTOTYPE_METHOD( buttonInstanceTemplate, "setSecondary", setSecondary );

	c->InterShellObject_buttonInstanceConstructor.Reset( isolate, buttonInstanceTemplate->GetFunction(context).ToLocalChecked() );

	//--------------------------------------------
	Local<FunctionTemplate> controlTemplate;
	// Prepare constructor template
	controlTemplate = FunctionTemplate::New( isolate, NewControl );
	controlTemplate->SetClassName( localStringExternal( isolate, "sack.InterShell.Control" ) );
	controlTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // needs to be 1 for wrap

	 // Prototype
	c->InterShellObject_controlConstructor.Reset( isolate, controlTemplate->GetFunction(context).ToLocalChecked() );

	SET_READONLY( intershellTemplate->GetFunction(context).ToLocalChecked(), "Control", controlTemplate->GetFunction(context).ToLocalChecked() );

	//--------------------------------------------

	Local<FunctionTemplate> controlInstanceTemplate;
	// Prepare constructor template
	controlInstanceTemplate = FunctionTemplate::New( isolate, is_control::NewControlInstance );
	controlInstanceTemplate->SetClassName( localStringExternal( isolate, "sack.InterShell.control.instance" ) );
	controlInstanceTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // needs to be 1 for wrap
																			 // Prototype
	NODE_SET_PROTOTYPE_METHOD( controlInstanceTemplate, "setTitle", onCreateControl );

	c->InterShellObject_controlInstanceConstructor.Reset( isolate, controlInstanceTemplate->GetFunction(context).ToLocalChecked() );


	//--------------------------------------------
	Local<FunctionTemplate> customControlTemplate;
	// Prepare constructor template
	customControlTemplate = FunctionTemplate::New( isolate, NewCustomControl );
	customControlTemplate->SetClassName( localStringExternal( isolate, "sack.InterShell.CustomControl" ) );
	customControlTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // needs to be 1 for wrap

																			 // Prototype
	NODE_SET_PROTOTYPE_METHOD( customControlTemplate, "setCreate", onCreateCustomControl );
	//NODE_SET_PROTOTYPE_METHOD( buttonTemplate, "onMouse", query );
	NODE_SET_PROTOTYPE_METHOD( customControlTemplate, "setSave", onSaveControl );
	NODE_SET_PROTOTYPE_METHOD( customControlTemplate, "setLoad", onLoadControl );

	c->InterShellObject_customControlConstructor.Reset( isolate, customControlTemplate->GetFunction(context).ToLocalChecked() );

	SET_READONLY( intershellTemplate->GetFunction(context).ToLocalChecked(), "Custom", customControlTemplate->GetFunction(context).ToLocalChecked() );

	//--------------------------------------------
	Local<FunctionTemplate> customControlInstanceTemplate;
	// Prepare constructor template
	customControlInstanceTemplate = FunctionTemplate::New( isolate, is_control::NewControlInstance );
	customControlInstanceTemplate->SetClassName( localStringExternal( isolate, "sack.InterShell.CustomControl.instance" ) );
	customControlInstanceTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // needs to be 1 for wrap

																			 // Prototype
	NODE_SET_PROTOTYPE_METHOD( customControlInstanceTemplate, "setCreate", onCreateCustomControl );
	//NODE_SET_PROTOTYPE_METHOD( buttonTemplate, "onMouse", query );
	NODE_SET_PROTOTYPE_METHOD( customControlInstanceTemplate, "setSave", onSaveControl );
	NODE_SET_PROTOTYPE_METHOD( customControlInstanceTemplate, "setLoad", onLoadControl );

	c->InterShellObject_customControlInstanceConstructor.Reset( isolate, customControlInstanceTemplate->GetFunction(context).ToLocalChecked() );

	//--------------------------------------------


}

//-----------------------------------------------------------

void is_control::NewControlInstance( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		is_control* obj;
		obj = new is_control();
		obj->self.Reset( isolate, args.This() );
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	}
	else {
		//const int argc = 2;
		//Local<Value> argv[argc] = { args[0], args.Holder() };
		//Local<Function> cons = Local<Function>::New( isolate, buttonConstructor );
		//args.GetReturnValue().Set( cons->NewInstance( argc, argv ) );
	}
}


//-----------------------------------------------------------

void InterShellObject::NewButton( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( args.IsConstructCall() ) {
		char *name;
		String::Utf8Value arg( isolate, args[0] );
		name = StrDup( *arg );
		InterShellObject* obj;
		obj = new InterShellObject( name, TRUE );

		obj->self.Reset( isolate, args.This() );
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	} else {
		const int argc = 2;
		Local<Value> argv[argc] = { args[0], args.Holder() };
		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->InterShellObject_buttonConstructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
	}
}

void InterShellObject::addConfigMethod( const FunctionCallbackInfo<Value>& args ) {
}

void InterShellObject::NewConfiguration( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		char *name;
		String::Utf8Value arg( isolate, args[0] );
		name = *arg;
		InterShellObject* obj;

		obj = new InterShellObject();
		obj->events = NULL;

		obj->self.Reset( isolate, args.This() );
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	}
	else {
		const int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];// = { args[0], args.Holder() };
		for( int n = 0; n < argc; n++ ) argv[n] = args[n];
		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->InterShellObject_controlConstructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
	}
}

void InterShellObject::NewApplication( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	class constructorSet* c = getConstructors( isolate );
	if( args.IsConstructCall() ) {
		if( !InterShell ) {
			LoadFunction( "bag.psi.dll", NULL );
			LoadFunction( "sack_widgets.dll", NULL );
			LoadFunction( "InterShell.core", NULL );
		}
		if( !isLocal.core ) {
			char *name;
			String::Utf8Value arg( isolate, args[0] );
			name = *arg;
			InterShellObject* obj;

			obj = new InterShellObject();
			obj->events = NULL;
			MemSet( &obj->async, 0, sizeof( obj->async ) );
			uv_async_init( uv_default_loop(), &obj->async, asyncmsg );
			obj->async.data = obj;

			isLocal.core = obj;
			c->canvasObject.Reset( isolate, args.This() );

			obj->self.Reset( isolate, args.This() );
			obj->Wrap( args.This() );
			args.GetReturnValue().Set( args.This() );
		}
		else {
			args.GetReturnValue().Set( c->canvasObject.Get( isolate ) );

		}
	}
	else {
		const int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];// = { args[0], args.Holder() };
		for( int n = 0; n < argc; n++ ) argv[n] = args[n];
		Local<Function> cons = Local<Function>::New( isolate, c->InterShellObject_intershellConstructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete[] argv;
	}
}


void InterShellObject::NewControl( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( args.IsConstructCall() ) {
		char *name;
		Local<Object> opts;
		Local<String> optName;
		struct optionStrings *strings;
		if( args[0]->IsString() ) {
			String::Utf8Value arg( isolate, args[0] );
			name = StrDup( *arg );
		} else if( args[0]->IsObject() ) {
			opts = args[0]->ToObject(context).ToLocalChecked();
			strings = getStrings( isolate );
			if( opts->Has( context, optName = strings->nameString->Get( isolate ) ).ToChecked() ) {
				String::Utf8Value arg( isolate, GETV( opts, optName ) );
				name = StrDup( *arg );
			}
		}
		InterShellObject* obj;
		obj = new InterShellObject( name, FALSE );

		{
			if( opts->Has( context, optName = strings->controlRegistrationString->Get( isolate ) ).ToChecked() ) {
				Local<Object> registration = opts->Get( context, optName ).ToLocalChecked()->ToObject(context).ToLocalChecked();
				RegistrationObject *regobj = ObjectWrap::Unwrap<RegistrationObject>( registration );
				obj->registrationObject.Reset( isolate, registration );
				obj->registration = regobj;
			}
			defineOnQueryControl( name );

			if( opts->Has( context, optName = strings->createString->Get( isolate ) ).ToChecked() )
				obj->cbCreate.Reset( isolate, Local<Function>::Cast( opts->Get( context, optName ).ToLocalChecked() ) );
			if( opts->Has( context, optName = strings->destroyString->Get( isolate ) ).ToChecked() )
				obj->cbDestroy.Reset( isolate, Local<Function>::Cast( opts->Get( context, optName ).ToLocalChecked() ) );
			if( opts->Has( context, optName = strings->mouseString->Get( isolate ) ).ToChecked() )
				obj->cbMouse.Reset( isolate, Local<Function>::Cast( opts->Get( context, optName ).ToLocalChecked() ) );
			if( opts->Has( context, optName = strings->drawString->Get( isolate ) ).ToChecked() )
				obj->cbDraw.Reset( isolate, Local<Function>::Cast( opts->Get( context, optName ).ToLocalChecked() ) );
		}

			Local<Object> _this = args.This();

			// this created a new registration so control events would be hooked to this.
			//Local<Function> cons = Local<Function>::New( isolate, ControlObject::registrationConstructor );
			//Local<Object> temp;
			//obj->registrationObject.Reset( isolate, temp = cons->NewInstance() );
			//obj->registration = ObjectWrap::Unwrap<RegistrationObject>( temp );
			//_this->Set( localStringExternal( isolate, "registration" ), temp );

			obj->self.Reset( isolate, args.This() );
			obj->Wrap( _this );
			args.GetReturnValue().Set( _this );
	}
	else {
		const int argc = 2;
		Local<Value> argv[argc] = { args[0], args.Holder() };
		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->InterShellObject_controlConstructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
	}
}

//-----------------------------------------------------------

void InterShellObject::NewCustomControl( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( args.IsConstructCall() ) {
		char *name;
		String::Utf8Value arg( isolate, args[0] );
		name = StrDup( *arg );
		InterShellObject* obj;

		obj = new InterShellObject( name, FALSE );
		obj->bCustom = TRUE;

		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	}
	else {
		const int argc = 2;
		Local<Value> argv[argc] = { args[0], args.Holder() };
		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->InterShellObject_customControlConstructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
	}
}

//-----------------------------------------------------------

void InterShellObject::onCreateButton( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	InterShellObject *obj = ObjectWrap::Unwrap<InterShellObject>( args.This() );

	obj->cbCreate.Reset( isolate, Local<Function>::Cast( args[0] ) );

	args.GetReturnValue().Set( True( isolate ) );
}

//-----------------------------------------------------------

void InterShellObject::setTitle( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	is_control *obj = ObjectWrap::Unwrap<is_control>( args.This() );
	String::Utf8Value sName( isolate, args[0]->ToString(context).ToLocalChecked() );
	char *title = StrDup( *sName );
	InterShell_SetButtonText( obj->button, title );
	Deallocate( char*, title );
	
	args.GetReturnValue().Set( True( isolate ) );
}
void InterShellObject::setStyle( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	is_control *obj = ObjectWrap::Unwrap<is_control>( args.This() );
	String::Utf8Value sName( isolate, args[0]->ToString(context).ToLocalChecked() );
	char *style = StrDup( *sName );
	InterShell_SetButtonStyle( obj->button, style );
	Deallocate( char*, style );

	args.GetReturnValue().Set( True( isolate ) );
}
void InterShellObject::setTextColor( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	is_control *obj = ObjectWrap::Unwrap<is_control>( args.This() );
	ColorObject *color = ObjectWrap::Unwrap<ColorObject>( args[0]->ToObject(context).ToLocalChecked() );
	InterShell_SetButtonColor( obj->button, color->color, 0 );

	args.GetReturnValue().Set( True( isolate ) );
}
void InterShellObject::setBackground( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	is_control *obj = ObjectWrap::Unwrap< is_control>( args.This() );
	ColorObject *color = ObjectWrap::Unwrap<ColorObject>( args[0]->ToObject(context).ToLocalChecked() );
	InterShell_SetButtonColor( obj->button, color->color, 0 );

	args.GetReturnValue().Set( True( isolate ) );
}
void InterShellObject::setSecondary( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	is_control *obj = ObjectWrap::Unwrap< is_control>( args.This() );
	ColorObject *color = ObjectWrap::Unwrap<ColorObject>( args[0]->ToObject(context).ToLocalChecked() );
	InterShell_SetButtonColor( obj->button, color->color, 0 );

	args.GetReturnValue().Set( True( isolate ) );
}

void InterShellObject::onClickButton( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	InterShellObject *obj = ObjectWrap::Unwrap<InterShellObject>( args.This() );
	defineButtonPress( obj->name );
	if( args[0]->IsFunction() ) {
		Local<Function> arg0 = Local<Function>::Cast( args[0] );
		obj->cbClick.Reset( isolate, arg0 );
		args.GetReturnValue().Set( True( isolate ) );
	}
	else {
		args.GetReturnValue().Set( False( isolate ) );
	}
}

//-----------------------------------------------------------

void InterShellObject::onSaveButton( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	args.GetReturnValue().Set( True( isolate ) );
}

//-----------------------------------------------------------

void InterShellObject::onLoadButton( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	args.GetReturnValue().Set( True( isolate ) );
}

//-----------------------------------------------------------

void InterShellObject::onCreateControl( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	InterShellObject *obj = ObjectWrap::Unwrap<InterShellObject>( args.This() );

	obj->cbCreate.Reset( isolate, Local<Function>::Cast( args[0] ) );

	args.GetReturnValue().Set( True( isolate ) );
}

//-----------------------------------------------------------

void InterShellObject::onCreateCustomControl( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	InterShellObject *obj = ObjectWrap::Unwrap<InterShellObject>( args.This() );

	obj->cbCreate.Reset( isolate, Local<Function>::Cast( args[0] ) );

	args.GetReturnValue().Set( True( isolate ) );
}

//-----------------------------------------------------------

void InterShellObject::onSaveControl( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	args.GetReturnValue().Set( True( isolate ) );
}

//-----------------------------------------------------------

void InterShellObject::onLoadControl( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	args.GetReturnValue().Set( True( isolate ) );
}

//-----------------------------------------------------------

InterShellObject::InterShellObject(  )
{
	bCustom = FALSE;
}

InterShellObject::InterShellObject( char *name, LOGICAL bButton )
{
	this->name = name;
	if( !bButton ) {
		bCustom = TRUE; // custom control will later set it.
		defineCreateControl( name );
	}
	else {
		bCustom = FALSE; // custom control will later set it.
		defineCreateButton( name );
	}
	AddLink( &isLocal.controlTypes, this );
}

InterShellObject::~InterShellObject() {
}

//-----------------------------------------------------------
//-----------------------------------------------------------

static uintptr_t cbCreateControl( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h ) {
	CTEXTSTR controlType = InterShell_GetCurrentlyCreatingButtonType();
	InterShellObject *io = findControlType( controlType );
	is_control *c = NewArray( is_control, 1 );
	PMENU_BUTTON button = InterShell_GetCurrentlyCreatingButton();
	isLocal.creating_parent = parent;
	c->psvData.Empty();
	c->psvControl.Empty();
	c->button = button;
	c->isButton = FALSE;
	c->caption = NULL;
	c->type = io;
	g.nextControlCreatePosition.x = x;
	g.nextControlCreatePosition.y = y;
	g.nextControlCreatePosition.w = w;
	g.nextControlCreatePosition.h = h;
	g.nextControlCreatePosition.control = c;
	if( !controlType ) {
		lprintf( "how did we get here without a valid name?" );
		DebugBreak();
	}
	if( !io->bCustom ) {
		c->pc = MakeNamedCaptionedControl( parent, io->name, x, y, w, h, -1, c->caption );
		MakeISEvent( &isLocal.core->async, &isLocal.core->events, Event_Intershell_CreateControl, c );
	} else {
		c->pc = MakeNamedCaptionedControl( parent, io->registration->r.name, x, y, w, h, -1, c->caption );
		MakeISEvent( &isLocal.core->async, &isLocal.core->events, Event_Intershell_CreateCustomControl, c, x, y, w, h );
		//c->pc = g.nextControlCreatePosition.resultControl;
	}
	return (uintptr_t)c;
}

static void defineCreateControl( char *name ) {
	//DefineRegistryMethod(TASK_PREFIX,CreateControl,WIDE( "control" ),name,WIDE( "control_create" ),uintptr_t,(PSI_CONTROL,int32_t,int32_t,uint32_t,uint32_t))

	TEXTCHAR buf[256];
	//lprintf( "Define Create Control %s", name );
	snprintf( buf, 256, "intershell/control/%s", name );
	SimpleRegisterMethod( buf, cbCreateControl
								, "uintptr_t", "control_create", "(PSI_CONTROL,int32_t,int32_t,uint32_t,uint32_t)" );
}

static void cbDestroyControl( uintptr_t psvControl ) {
	MakeISEvent( &isLocal.core->async, &isLocal.core->events, Event_Intershell_Control_Destroy );
}

static void defineDestroyControl( char *name ) {
	//DefineRegistryMethod(TASK_PREFIX,CreateControl,WIDE( "control" ),name,WIDE( "control_create" ),uintptr_t,(PSI_CONTROL,int32_t,int32_t,uint32_t,uint32_t))

	TEXTCHAR buf[256];
	//lprintf( "Define Destroy Control %s", name );
	snprintf( buf, 256, "intershell/control/%s", name );
	SimpleRegisterMethod( buf, cbDestroyControl
		, "void", "control_destroy", "(uintptr_t)" );
}

static void cbMouseControl( uintptr_t psvControl ) {
	MakeISEvent( &isLocal.core->async, &isLocal.core->events, Event_Intershell_Control_Destroy );
}

static void defineMouseControl( char *name ) {
	//DefineRegistryMethod(TASK_PREFIX,CreateControl,WIDE( "control" ),name,WIDE( "control_create" ),uintptr_t,(PSI_CONTROL,int32_t,int32_t,uint32_t,uint32_t))

	TEXTCHAR buf[256];
	//lprintf( "Define Mouse Control %s", name );
	snprintf( buf, 256, "intershell/control/%s", name );
	SimpleRegisterMethod( buf, cbMouseControl
		, "int", "control_mouse", "(uintptr_t)" );
}


static LOGICAL cbQueryShowControl( uintptr_t psvInit) {
	is_control *c = (is_control *)psvInit;
	 lprintf( "Need to pass query to JS" );
	 return TRUE;
}

static void defineOnQueryShowControl( char *name ) {
	//TASK_PREFIX, QueryShowControl, WIDE( "control" ), name, WIDE( "query can show" ), LOGICAL, (uintptr_t))
	TEXTCHAR buf[256];
	lprintf( "Define Create Control %s", name );
	snprintf( buf, 256, "intershell/control/%s", name );
	SimpleRegisterMethod( buf, cbQueryShowControl
		, "LOGICAL", "query can show", "(uintptr_t)" );
}


static PSI_CONTROL cbQueryControl( uintptr_t psvInit ) {
	is_control *c = (is_control *)psvInit;
	return c->pc;
}

static void defineOnQueryControl( char *name ) {
	TEXTCHAR buf[256];
	//DefineRegistryMethod( TASK_PREFIX, GetControl, WIDE( "control" ), name, WIDE( "get_control" ), PSI_CONTROL, (uintptr_t) )	TEXTCHAR buf[256];
	lprintf( "Define Query Control %s", name );
	snprintf( buf, 256, "intershell/control/%s", name );
	SimpleRegisterMethod( buf, cbQueryControl
		, "PSI_CONTROL", "get_control", "(uintptr_t)" );
}



static uintptr_t cbCreateButton( PMENU_BUTTON button ) {
	CTEXTSTR controlType = InterShell_GetCurrentlyCreatingButtonType();
	InterShellObject *io = findControlType( controlType );
	is_control *c = NewArray( is_control, 1 );
	c->psvData.Empty();
	c->psvControl.Empty();
	c->isButton = TRUE;
	c->button = button;
	c->pc = NULL;
	c->caption = NULL;
	c->type = io;
	MakeISEvent( &isLocal.core->async, &isLocal.core->events, Event_Intershell_CreateButton, c );
	return (uintptr_t)c;
}

static void defineCreateButton( char *name ) {
	TEXTCHAR buf[256];
	lprintf( "Define Create Button %s", name );
	snprintf( buf, 256, "intershell/control/%s", name );
	SimpleRegisterMethod( buf, cbCreateButton
								, "uintptr_t", "button_create", "(PMENU_BUTTON)" );
}

static void cbButtonClick( uintptr_t self ) {
	MakeISEvent( &isLocal.core->async, &isLocal.core->events, Event_Intershell_ButtonClick
				, self );
}

static void defineButtonPress( char *name ) {
//#define OnKeyPressEvent(name)  \
//		DefineRegistryMethod(WIDE( "sack/widgets" ),KeyPressHandler,WIDE( "keypad" ),WIDE( "press handler/" )name, WIDE( "on_keypress_event" ),void,(uintptr_t))

	TEXTCHAR buf[256];
	snprintf( buf, 256, "sack/widgets/keypad/press handler/%s", name );
	SimpleRegisterMethod( buf, cbButtonClick
								, "void", "on_keypress_event", "(uintptr_t)" );
}


int MakeISEvent( uv_async_t *async, PLINKQUEUE *queue, enum GUI_eventType type, ... ) {
	event *e = GetFromSet( IS_EVENT, &isLocal.events );
	va_list args;
	va_start( args, type );
	e->type = type;
	e->flags.complete = 0;
	e->waiter = MakeThread();
	switch( type ) {
	case Event_Intershell_Quit:
		e->data.InterShell.control = NULL;
		e->flags.complete = 1;
		break;
	case Event_Intershell_ButtonClick: {
		is_control *c = va_arg( args, is_control * );
		e->data.InterShell.control = c;
		break;
	}
	case Event_Intershell_CreateButton: {
		is_control *c = va_arg( args, is_control * );
		e->data.InterShell.control = c;
		break;
	}
	case Event_Intershell_CreateControl: {
		is_control *c = va_arg( args, is_control * );
		e->data.InterShell.control = c;
		break;
	}
	case Event_Intershell_CreateCustomControl: {
		is_control *c = va_arg( args, is_control * );
		e->data.createCustomControl.control = c;
		e->data.createCustomControl.x = va_arg( args, int32_t );
		e->data.createCustomControl.y = va_arg( args, int32_t );
		e->data.createCustomControl.w = va_arg( args, uint32_t );
		e->data.createCustomControl.h = va_arg( args, uint32_t );
		break;
	}
	case Event_Render_Key:
		e->data.key.code = va_arg( args, uint32_t );
		break;
	}

	//e->value = 0;
	EnqueLink( queue, e );
	uv_async_send( async );
	if( !e->flags.complete ) {
		while( !e->flags.complete ) WakeableSleep( 1000 );
		{
			int success = (int)e->success;
			DeleteFromSet( IS_EVENT, isLocal.events, e );
			return success;
		}
	}
	return 1;
}

static void OnApplicationQuit( "Intershell Core" )(void) {
	if( isLocal.core )
		MakeISEvent( &isLocal.core->async, &isLocal.core->events, Event_Intershell_Quit );
}
