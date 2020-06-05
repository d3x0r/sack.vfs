
#include "../global.h"



struct optionStrings {
	Isolate* isolate;

	Eternal<String>* nameString;
	Eternal<String>* widthString;
	Eternal<String>* heightString;
	Eternal<String>* borderString;
	Eternal<String>* createString;
	Eternal<String>* drawString;
	Eternal<String>* mouseString;
	Eternal<String>* keyString;
	Eternal<String>* destroyString;
	Eternal<String>* xString;
	Eternal<String>* yString;
	Eternal<String>* wString;
	Eternal<String>* hString;
	Eternal<String>* sizeString;
	Eternal<String>* posString;
	Eternal<String>* layoutString;
	Eternal<String>* indexString;
	Eternal<String>* objectString;
	Eternal<String>* imageString;
	Eternal<String>* anchorsString;
	Eternal<String>* definesColorsString;
};


static struct optionStrings* getStrings( Isolate* isolate ) {
	static PLIST strings;
	INDEX idx;
	struct optionStrings* check;
	LIST_FORALL( strings, idx, struct optionStrings*, check ) {
		if( check->isolate == isolate )
			break;
	}
	if( !check ) {
		check = NewArray( struct optionStrings, 1 );
		check->isolate = isolate;
#define makeString(a,b) check->a##String = new Eternal<String>( isolate, localStringExternal( isolate, b ) );
		check->nameString = new Eternal<String>( isolate, localStringExternal( isolate, "name" ) );
		check->widthString = new Eternal<String>( isolate, localStringExternal( isolate, "width" ) );
		check->heightString = new Eternal<String>( isolate, localStringExternal( isolate, "height" ) );
		check->borderString = new Eternal<String>( isolate, localStringExternal( isolate, "border" ) );
		check->createString = new Eternal<String>( isolate, localStringExternal( isolate, "create" ) );
		check->mouseString = new Eternal<String>( isolate, localStringExternal( isolate, "mouse" ) );
		check->drawString = new Eternal<String>( isolate, localStringExternal( isolate, "draw" ) );
		check->keyString = new Eternal<String>( isolate, localStringExternal( isolate, "key" ) );
		check->destroyString = new Eternal<String>( isolate, localStringExternal( isolate, "destroy" ) );
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

		//check->String = new Eternal<String>( isolate, localStringExternal( isolate, "" ) );
		AddLink( &strings, check );
	}
	return check;
}


static Local<Value> ProcessEvent( Isolate* isolate, struct event *evt, RenderObject *r ) {
	//Local<Object> object = Object::New( isolate );
	Local<Object> object;
	Local<Context> context = isolate->GetCurrentContext();


	switch( evt->type ) {
	case Event_Render_Mouse:
		{
			static Persistent<Object> mo;
			if( mo.IsEmpty() ) {
				object = Object::New( isolate );
				mo.Reset( isolate, object );
			}
			else
				object = Local<Object>::New( isolate, mo );

			object->Set( context, localStringExternal( isolate, "x" ), Number::New( isolate, evt->data.mouse.x ) );
			object->Set( context, localStringExternal( isolate, "y" ), Number::New( isolate, evt->data.mouse.y ) );
			object->Set( context, localStringExternal( isolate, "b" ), Number::New( isolate, evt->data.mouse.b ) );
		}
		break;
	case Event_Render_Draw:
		if( r->surface.IsEmpty() )
			r->surface.Reset( isolate, ImageObject::NewImage( isolate, GetDisplayImage( r->r ), TRUE ) );
		return Local<Object>::New( isolate, r->surface );
	case Event_Render_Key:
		return Number::New( isolate, evt->data.key.code );
	default:
		lprintf( "Unhandled event %d(%04x)", evt->type, evt->type );
		return Undefined( isolate );
	}
	return object;
}

static void asyncmsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	Local<Context> context = isolate->GetCurrentContext();

	RenderObject* myself = (RenderObject*)handle->data;

	HandleScope scope(isolate);
	//lprintf( "async message notice. %p", myself );
	{
		struct event *evt;

		while( evt = (struct event *)DequeLink( &myself->receive_queue ) ) {

			Local<Value> object = ProcessEvent( isolate, evt, myself );
			Local<Value> argv[] = { object };
			Local<Function> cb;
			switch( evt->type ){
			case Event_Render_Mouse:
				cb = Local<Function>::New( isolate, myself->cbMouse );
				break;
			case Event_Render_Key:
				cb = Local<Function>::New( isolate, myself->cbKey );
				break;
			case Event_Render_Draw:
				cb = Local<Function>::New( isolate, myself->cbDraw );
				break;
			}
			Local<Value> r = cb->Call( context, isolate->GetCurrentContext()->Global(), 1, argv ).ToLocalChecked();
			if( evt->waiter ) {
				if( r.IsEmpty() )
					evt->success = true;
				else
					evt->success = (int)r->IntegerValue(context).ToChecked();
				evt->flags.complete = TRUE;
				WakeThread( evt->waiter );
			}
		}
	}
	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
	//lprintf( "done calling message notice." );
}




void RenderObject::Init( Local<Object> exports ) {
	{
		//extern void Syslog
	}
		Isolate* isolate = Isolate::GetCurrent();
		Local<Context> context = isolate->GetCurrentContext();
		Local<FunctionTemplate> renderTemplate;

		// Prepare constructor template
		renderTemplate = FunctionTemplate::New( isolate, New );
		renderTemplate->SetClassName( localStringExternal( isolate, "sack.Renderer" ) );
		renderTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); /* one internal for wrap */


		// Prototype
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "getImage", RenderObject::getImage );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "setDraw", RenderObject::setDraw );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "setMouse", RenderObject::setMouse );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "setKey", RenderObject::setKey );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "show", RenderObject::show );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "hide", RenderObject::hide );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "reveal", RenderObject::reveal );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "redraw", RenderObject::redraw );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "update", RenderObject::update );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "close", RenderObject::close );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "on", RenderObject::on );

		renderTemplate->PrototypeTemplate()->SetAccessorProperty( localStringExternal( isolate, "size" )
			, FunctionTemplate::New( isolate, RenderObject::getCoordinate, Integer::New( isolate, 12 ) )
			, FunctionTemplate::New( isolate, RenderObject::setCoordinate, Integer::New( isolate, 11 ) )
			, DontDelete );
		renderTemplate->PrototypeTemplate()->SetAccessorProperty( localStringExternal( isolate, "position" )
			, FunctionTemplate::New( isolate, RenderObject::getCoordinate, Integer::New( isolate, 12 ) )
			, FunctionTemplate::New( isolate, RenderObject::setCoordinate, Integer::New( isolate, 10 ) )
			, DontDelete );
		renderTemplate->PrototypeTemplate()->SetAccessorProperty( localStringExternal( isolate, "layout" )
			, FunctionTemplate::New( isolate, RenderObject::getCoordinate, Integer::New( isolate, 12 ) )
			, FunctionTemplate::New( isolate, RenderObject::setCoordinate, Integer::New( isolate, 12 ) )
			, DontDelete );


		Local<Function> renderFunc = renderTemplate->GetFunction(context).ToLocalChecked();
		SET_READONLY_METHOD( renderFunc, "is3D", RenderObject::is3D );

    class constructorSet* c = getConstructors( isolate );
    c->RenderObject_constructor.Reset( isolate, renderTemplate->GetFunction(context).ToLocalChecked() );
		SET_READONLY( exports, "Renderer", renderTemplate->GetFunction(context).ToLocalChecked() );
		SET_READONLY( renderTemplate->GetFunction(context).ToLocalChecked(), "getDisplay"
				, Function::New( context, RenderObject::getDisplay ).ToLocalChecked() );
	}

RenderObject::RenderObject( const char *title, int x, int y, int w, int h, RenderObject *over )  {
	if( title )
		r = OpenDisplayAboveSizedAt( 0, w, h, x, y, over ? over->r : NULL );
	else
		r = NULL;
	receive_queue = NULL;
	drawn = 0;
	closed = 0;
}

void RenderObject::setRenderer(PRENDERER r) {

}

RenderObject::~RenderObject() {
}

	void RenderObject::New( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		Local<Context> context = isolate->GetCurrentContext();
		if( args.IsConstructCall() && ( args.This()->InternalFieldCount() == 1)  ) {

			char *title = NULL;
			int x = 0, y = 0, w = 1024, h = 768, border = 0;
			Local<Object> parent_object;
			RenderObject *parent = NULL;

			int argc = args.Length();
			if( argc > 0 ) {
				String::Utf8Value fName( isolate, args[0] );
				title = StrDup( *fName );
			}
			if( argc > 1 ) {
				x = (int)args[1]->IntegerValue(context).ToChecked();
			}
			if( argc > 2 ) {
				y = (int)args[2]->IntegerValue(context).ToChecked();
			}
			if( argc > 3 ) {
				w = (int)args[3]->IntegerValue(context).ToChecked();
			}
			if( argc > 4 ) {
				h = (int)args[4]->IntegerValue(context).ToChecked();
			}
			if( argc > 5 ) {
				if( args[5]->IsNull() ) {
					parent = NULL;
				}
				else {
					parent_object = args[5]->ToObject(context).ToLocalChecked();
					parent = ObjectWrap::Unwrap<RenderObject>( parent_object );
				}
			}
			// Invoked as constructor: `new MyObject(...)`
			RenderObject* obj = new RenderObject( title?title:"Node Application", x, y, w, h, parent );

			MemSet( &obj->async, 0, sizeof( obj->async ) );
			uv_async_init( uv_default_loop(), &obj->async, asyncmsg );

			obj->async.data = obj;

			obj->Wrap( args.This() );
			args.GetReturnValue().Set( args.This() );
			if( title )
				Deallocate( char*, title );
		}
		else {
			// Invoked as plain function `MyObject(...)`, turn into construct call.
			int argc = args.Length();
			Local<Value> *argv = new Local<Value>[argc];
			for( int n = 0; n < argc; n++ )
				argv[n] = args[n];

			class constructorSet* c = getConstructors( isolate );
			Local<Function> cons = Local<Function>::New( isolate, c->RenderObject_constructor );
			args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
			delete argv;
		}
	}


	
void RenderObject::is3D( const FunctionCallbackInfo<Value>& args ) {
	if( GetRender3dInterface() )
		args.GetReturnValue().Set( True( args.GetIsolate() ));
	else
		args.GetReturnValue().Set( False( args.GetIsolate() ));
}


void RenderObject::getCoordinate( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context>context = isolate->GetCurrentContext();
	RenderObject* me = ObjectWrap::Unwrap<RenderObject>( args.This() );
	int coord = (int)args.Data()->IntegerValue( context ).ToChecked();
	Local<Object> o = Object::New( isolate );
	struct optionStrings* strings = getStrings( isolate );
	int32_t x, y;
	uint32_t w, h;
	GetDisplayPosition( me->r, &x, &y, &w, &h );
	switch( coord ) {
	case 10:
		o->Set( context, strings->xString->Get( isolate ), Integer::New( isolate, x ) );
		o->Set( context, strings->yString->Get( isolate ), Integer::New( isolate, y ) );
		break;
	case 11:
		o->Set( context, strings->wString->Get( isolate ), Integer::New( isolate, w ) );
		o->Set( context, strings->hString->Get( isolate ), Integer::New( isolate, h ) );
		break;
	case 12:
		o->Set( context, strings->xString->Get( isolate ), Integer::New( isolate, x ) );
		o->Set( context, strings->yString->Get( isolate ), Integer::New( isolate, y ) );
		o->Set( context, strings->wString->Get( isolate ), Integer::New( isolate, w ) );
		o->Set( context, strings->hString->Get( isolate ), Integer::New( isolate, h ) );
		break;
	}
	args.GetReturnValue().Set( o );
}

void RenderObject::setCoordinate( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context>context = isolate->GetCurrentContext();
	RenderObject* me = ObjectWrap::Unwrap<RenderObject>( args.This() );
	int coord = (int)args.Data()->IntegerValue( context ).ToChecked();
	Local<Object> o = args[0]->ToObject( context ).ToLocalChecked();
	if( !o.IsEmpty() ) {
		struct optionStrings* strings = getStrings( isolate );
		int32_t x, y;
		uint32_t w, h;
		switch( coord ) {
		case 10:
			x = (int32_t)o->Get( context, strings->xString->Get( isolate ) ).ToLocalChecked()->IntegerValue( context ).ToChecked();
			y = (int32_t)o->Get( context, strings->yString->Get( isolate ) ).ToLocalChecked()->IntegerValue( context ).ToChecked();
			MoveDisplay( me->r, x, y );
			break;
		case 11:
			w = (uint32_t)o->Get( context, strings->wString->Get( isolate ) ).ToLocalChecked()->IntegerValue( context ).ToChecked();
			h = (uint32_t)o->Get( context, strings->hString->Get( isolate ) ).ToLocalChecked()->IntegerValue( context ).ToChecked();
			SizeDisplay( me->r, w, h );
			break;
		case 12:
			x = (int32_t)o->Get( context, strings->xString->Get( isolate ) ).ToLocalChecked()->IntegerValue( context ).ToChecked();
			y = (int32_t)o->Get( context, strings->yString->Get( isolate ) ).ToLocalChecked()->IntegerValue( context ).ToChecked();
			w = (uint32_t)o->Get( context, strings->wString->Get( isolate ) ).ToLocalChecked()->IntegerValue( context ).ToChecked();
			h = (uint32_t)o->Get( context, strings->hString->Get( isolate ) ).ToLocalChecked()->IntegerValue( context ).ToChecked();
			MoveSizeDisplay( me->r, x, y, w, h );
			break;
		}
	}

}



void RenderObject::close( const FunctionCallbackInfo<Value>& args ) {
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	if( !r->closed ) {
		r->closed = TRUE;
		lprintf( "Close async" );
		uv_close( (uv_handle_t*)&r->async, NULL );
	}
}

void RenderObject::getImage( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );

	// results as a new Image in result...
	r->surface.Reset( isolate, ImageObject::NewImage( isolate, GetDisplayImage( r->r ), TRUE ) );

}

void RenderObject::redraw( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	Redraw( r->r );
}

void RenderObject::update( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	int argc = args.Length();
	if( argc > 3 ) {
		int x, y, w, h;
		x = (int)args[0]->IntegerValue(context).ToChecked();
		y = (int)args[1]->IntegerValue(context).ToChecked();
		w = (int)args[2]->IntegerValue(context).ToChecked();
		h = (int)args[3]->IntegerValue(context).ToChecked();
		UpdateDisplayPortion( r->r, x,y,w,h );
	}
	else  {
		UpdateDisplayPortion( r->r, 0, 0, 0, 0 );
	}
}

void RenderObject::getDisplay( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> result = Object::New( isolate );
	int32_t x = 0, y = 0;
	uint32_t w, h;
	if( args.Length() < 1 ) {
		GetDisplaySizeEx( 0, &x, &y, &w, &h );
		result->Set( context, localStringExternal( isolate, "x" ), Integer::New( isolate, x ) );
		result->Set( context, localStringExternal( isolate, "y" ), Integer::New( isolate, y ) );
		result->Set( context, localStringExternal( isolate, "width" ), Integer::New( isolate, w ) );
		result->Set( context, localStringExternal( isolate, "height" ), Integer::New( isolate, h ) );
	}
	else {
		GetDisplaySizeEx( (int)args[0]->IntegerValue(context).ToChecked(), &x, &y, &w, &h );
		{
			result->Set( context, localStringExternal( isolate, "x" ), Integer::New( isolate, x ) );
			result->Set( context, localStringExternal( isolate, "y" ), Integer::New( isolate, y ) );
			result->Set( context, localStringExternal( isolate, "width" ), Integer::New( isolate, w ) );
			result->Set( context, localStringExternal( isolate, "height" ), Integer::New( isolate, h ) );
		}
	}
	args.GetReturnValue().Set( result );
}

void RenderObject::show( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	// UpdateDisplay deadlocks; so use this method instead....
	// this means the display is not nessecarily shown when this returns, but will be.
	RestoreDisplay( r->r );
}

void RenderObject::hide( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	HideDisplay( r->r );
}

void RenderObject::reveal( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	RestoreDisplay( r->r );
}

static uintptr_t CPROC doMouse( uintptr_t psv, int32_t x, int32_t y, uint32_t b ) {
	RenderObject *r = (RenderObject *)psv;
	if( !r->closed )
		return MakeEvent( &r->async, &r->receive_queue, Event_Render_Mouse, x, y, b );
	return 0;
}

void RenderObject::setMouse( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	Local<Function> arg0 = Local<Function>::Cast( args[0] );
	Persistent<Function> cb( isolate, arg0 );
	r->cbMouse = cb;
	SetMouseHandler( r->r, doMouse, (uintptr_t)r );
}

static void CPROC doRedraw( uintptr_t psv, PRENDERER out ) {
	RenderObject *r = (RenderObject *)psv;
	if( !r->closed )
		MakeEvent( &r->async, &r->receive_queue, Event_Render_Draw );
}

void RenderObject::setDraw( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	Local<Function> arg0 = Local<Function>::Cast( args[0] );
	Persistent<Function> cb( isolate, arg0 );
	SetRedrawHandler( r->r, doRedraw, (uintptr_t)r );
	r->cbDraw = cb;
	
}


static int CPROC doKey( uintptr_t psv, uint32_t key ) {
	RenderObject *r = (RenderObject *)psv;
	if( !r->closed )
		return (int)MakeEvent( &r->async, &r->receive_queue, Event_Render_Key, key );
	return 0;
}


void RenderObject::setKey( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	Local<Function> arg0 = Local<Function>::Cast( args[0] );
	Persistent<Function> cb( isolate, arg0 );
	SetKeyboardHandler( r->r, doKey, (uintptr_t)r );
	r->cbKey = cb;
}

void RenderObject::on( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	if( !args[0]->IsString() ) {
		isolate->ThrowException( Exception::Error(
			localStringExternal( isolate, TranslateText( "First argument must be event name as a string." ) ) ) );
		return;
	}
	if( !args[1]->IsFunction() ) {
		isolate->ThrowException( Exception::Error(
			localStringExternal( isolate, TranslateText( "Second argument must be callback function." ) ) ) );
		return;
	}
	Local<Function> arg1 = Local<Function>::Cast( args[1] );

	String::Utf8Value fName( isolate, args[0] );
	if( StrCmp( *fName, "draw" ) == 0 ) {
		Persistent<Function> cb( isolate, arg1 );
		SetRedrawHandler( r->r, doRedraw, (uintptr_t)r );
		r->cbDraw = cb;
	}
	else if( StrCmp( *fName, "mouse" ) == 0 ) {
		Persistent<Function> cb( isolate, arg1 );
		SetMouseHandler( r->r, doMouse, (uintptr_t)r );
		r->cbMouse = cb;
	}
	else if( StrCmp( *fName, "key" ) == 0 ) {
		Persistent<Function> cb( isolate, arg1 );
		SetKeyboardHandler( r->r, doKey, (uintptr_t)r );
		r->cbMouse = cb;
	}
}

uintptr_t MakeEvent( uv_async_t *async, PLINKQUEUE *queue, enum GUI_eventType type, ... ) {
	event e;
	va_list args;
	va_start( args, type );
	e.type = type;
	switch( type ) {
	case Event_Render_Mouse:
		e.data.mouse.x = va_arg( args, int32_t );
		e.data.mouse.y = va_arg( args, int32_t );
		e.data.mouse.b = va_arg( args, uint32_t );
		break;
	case Event_Render_Draw:
		break;
	case Event_Render_Key:
		e.data.key.code = va_arg( args, uint32_t );
		break;
	}

	e.waiter = MakeThread();
	e.flags.complete = 0; 
	e.success = 0;
	EnqueLink( queue, &e );
	uv_async_send( async );

	while( !e.flags.complete ) WakeableSleep( 1000 );

	return e.success;
}
