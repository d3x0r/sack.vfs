
#include "../global.h"
#ifdef INCLUDE_DAWN
#include "webgpu/canvas_context.h"
#endif
#include "sack_dom_key_names.h"   // kDomKeyName / kDomCodeName / sack_vk_to_dom_keyCode

extern "C" void* sack_image_get_webgpu_reverse_for_render(void);

struct render_private_data {
	PLIST renderers;
	PIMAGE_INTERFACE pri_gpu_image;
	PRENDER_INTERFACE pri_gpu_render;
} render_global;

struct optionStrings {
	Isolate* isolate;

	Eternal<String>* nameString;
	Eternal<String>* widthString;
	Eternal<String>* heightString;
	Eternal<String>* borderString;
	Eternal<String>* createString;
	Eternal<String>* drawString;
	Eternal<String>* mouseString;
	Eternal<String> *touchString;
	Eternal<String> *penString;
	Eternal<String> *keyString;
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
		check->touchString   = new Eternal<String>( isolate, localStringExternal( isolate, "touch" ) );
		check->penString   = new Eternal<String>( isolate, localStringExternal( isolate, "pen" ) );
		check->drawString    = new Eternal<String>( isolate, localStringExternal( isolate, "draw" ) );
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


// ---------- helpers for browser-shaped events ----------

// Sack mouse-button bit index → browser MouseEvent.button (0=L 1=M 2=R 3=back 4=fwd).
// Sack's b uses MK_LBUTTON=0x01, MK_RBUTTON=0x02, MK_MBUTTON=0x10, X1=0x20, X2=0x40.
static int sackBitToBrowserButton( int sackBitIndex ) {
	switch( sackBitIndex ) {
		case 0: return 0;  // LBUTTON  → left
		case 1: return 2;  // RBUTTON  → right
		case 4: return 1;  // MBUTTON  → middle
		case 5: return 3;  // XBUTTON1 → back
		case 6: return 4;  // XBUTTON2 → forward
		default: return -1;
	}
}

// Sack `b` bitmask of held buttons → browser MouseEvent.buttons (1=L 2=R 4=M 8=back 16=fwd).
static uint32_t sackButtonsToBrowserButtons( uint32_t b ) {
	uint32_t out = 0;
	if( b & MK_LBUTTON )  out |= 1;
	if( b & MK_RBUTTON )  out |= 2;
	if( b & MK_MBUTTON )  out |= 4;
	if( b & MK_XBUTTON1 ) out |= 8;
	if( b & MK_XBUTTON2 ) out |= 16;
	return out;
}

// Private symbols flagging events as "consumed" (preventDefault was called)
// or "stopped" (stopImmediatePropagation was called). Cached per-isolate by
// V8 — Private::ForApi interns by name.
static v8::Local<v8::Private> getConsumedPrivate( v8::Isolate *iso ) {
	return v8::Private::ForApi( iso
	                          , v8::String::NewFromUtf8Literal( iso, "__sack_consumed" ) );
}
static v8::Local<v8::Private> getStoppedPrivate( v8::Isolate *iso ) {
	return v8::Private::ForApi( iso
	                          , v8::String::NewFromUtf8Literal( iso, "__sack_stopped" ) );
}

// Native event.preventDefault() — signals sack the default action is
// suppressed (window proc returns non-zero). Does NOT stop other
// listeners on the same Renderer.
static void evtPreventDefault( const FunctionCallbackInfo<Value>& args ) {
	Isolate *iso = args.GetIsolate();
	Local<Context> ctx = iso->GetCurrentContext();
	(void)args.This()->SetPrivate( ctx, getConsumedPrivate( iso ), True( iso ) );
}

// Native event.stopImmediatePropagation() — dispatcher breaks the listener
// loop. Does NOT signal sack.
static void evtStopImmediatePropagation( const FunctionCallbackInfo<Value>& args ) {
	Isolate *iso = args.GetIsolate();
	Local<Context> ctx = iso->GetCurrentContext();
	(void)args.This()->SetPrivate( ctx, getStoppedPrivate( iso ), True( iso ) );
}

// Native event.stopPropagation() — no-op stub. A sack Renderer has no
// parent to bubble to; method exists only for browser API parity so
// copy-pasted code doesn't throw.
static void evtStopPropagation( const FunctionCallbackInfo<Value>& args ) {
	(void)args;
}

// Lazy-create + cache a per-method function in constructorSet, then
// attach to the event object.
static void attachOne( Isolate *iso
                     , Local<Context> ctx
                     , Local<Object> evt
                     , v8::Persistent<v8::Function> &slot
                     , v8::FunctionCallback impl
                     , const char *name ) {
	Local<Function> fn;
	if( slot.IsEmpty() ) {
		fn = Function::New( ctx, impl ).ToLocalChecked();
		slot.Reset( iso, fn );
	} else {
		fn = Local<Function>::New( iso, slot );
	}
	(void)evt->Set( ctx, localStringExternal( iso, name ), fn );
}

// Attach all three browser-event control methods to a fresh event object.
static void attachEventMethods( Isolate *iso, Local<Context> ctx, Local<Object> evt ) {
	constructorSet *c = getConstructors( iso );
	attachOne( iso, ctx, evt, c->preventDefault_fn          , evtPreventDefault          , "preventDefault" );
	attachOne( iso, ctx, evt, c->stopImmediatePropagation_fn, evtStopImmediatePropagation, "stopImmediatePropagation" );
	attachOne( iso, ctx, evt, c->stopPropagation_fn         , evtStopPropagation         , "stopPropagation" );
}

// Read either private off an event object.
static bool eventConsumed( Isolate *iso, Local<Context> ctx, Local<Object> evt ) {
	Local<Value> v;
	if( !evt->GetPrivate( ctx, getConsumedPrivate( iso ) ).ToLocal( &v ) ) return false;
	return v->BooleanValue( iso );
}
static bool eventStopped( Isolate *iso, Local<Context> ctx, Local<Object> evt ) {
	Local<Value> v;
	if( !evt->GetPrivate( ctx, getStoppedPrivate( iso ) ).ToLocal( &v ) ) return false;
	return v->BooleanValue( iso );
}

// (vkToDomKey moved to sack_dom_key_names.h as kDomKeyName + kDomCodeName.)

static Local<Value> ProcessEvent( Isolate* isolate, struct event *evt, RenderObject *r ) {
	//Local<Object> object = Object::New( isolate );
	Local<Object> object;
	Local<Context> context = isolate->GetCurrentContext();
	constructorSet *c      = getConstructors( isolate );

	switch( evt->type ) {
	case Event_Render_Pen: {
			if( c->pen_object.IsEmpty() ) {
				object = Object::New( isolate );
				c->pen_object.Reset( isolate, object );
			} else
				object = c->pen_object.Get( isolate );

			Local<Array> pens = Array::New( isolate );

			object->Set( context, localStringExternal( isolate, "x" ), Number::New( isolate, evt->data.pen.event.x ) );
			object->Set( context, localStringExternal( isolate, "y" ), Number::New( isolate, evt->data.pen.event.y ) );
			object->Set( context, localStringExternal( isolate, "flags" )
			           , Number::New( isolate, evt->data.pen.event.penFlags ) );
			object->Set( context, localStringExternal( isolate, "mask" )
			           , Number::New( isolate, evt->data.pen.event.penMask ) );
			object->Set( context, localStringExternal( isolate, "tiltX" ), Number::New( isolate, evt->data.pen.event.tiltX ) );
			object->Set( context, localStringExternal( isolate, "tiltY" ), Number::New( isolate, evt->data.pen.event.tiltY ) );
			object->Set( context, localStringExternal( isolate, "pressure" )
			           , Number::New( isolate, evt->data.pen.event.pressure ) );
			object->Set( context, localStringExternal( isolate, "rotation" )
			           , Number::New( isolate, evt->data.pen.event.rotation) );

			pens->Set( context, 0, object );
			for( int i = 0; i < evt->data.pen.event.nOverflow; i++ ) {
			   Local<Object> object = Object::New( isolate );
			   object->Set( context, localStringExternal( isolate, "x" ), Number::New( isolate, evt->data.pen.event.pOverflow[i].x ) );
			   object->Set( context, localStringExternal( isolate, "y" )
			              , Number::New( isolate, evt->data.pen.event.pOverflow[ i ].y ) );
			   object->Set( context, localStringExternal( isolate, "flags" )
			              , Number::New( isolate, evt->data.pen.event.pOverflow[ i ].penFlags ) );
			   object->Set( context, localStringExternal( isolate, "mask" )
			              , Number::New( isolate, evt->data.pen.event.pOverflow[ i ].penMask ) );
			   object->Set( context, localStringExternal( isolate, "tiltX" )
			              , Number::New( isolate, evt->data.pen.event.pOverflow[ i ].tiltX ) );
			   object->Set( context, localStringExternal( isolate, "tiltY" )
			              , Number::New( isolate, evt->data.pen.event.pOverflow[ i ].tiltY ) );
			   object->Set( context, localStringExternal( isolate, "pressure" )
			              , Number::New( isolate, evt->data.pen.event.pOverflow[ i ].pressure ) );
			   object->Set( context, localStringExternal( isolate, "rotation" )
			              , Number::New( isolate, evt->data.pen.event.pOverflow[ i ].rotation ) );
			   pens->Set( context, i + 1, object );
			}
		} 
		break;
	case Event_Render_Touch: {
			Local<Array> touches = Array::New( isolate );
			for( int n = 0; n < evt->data.touch.nTouches; n++ ) {
				Local<Object> touch = Object::New( isolate );
				touch->Set( context, localStringExternal( isolate, "new" )
						  , evt->data.touch.pTouches[ n ].flags.new_event ? True( isolate ) : False( isolate ) );
				touch->Set( context, localStringExternal( isolate, "end" )
						  , evt->data.touch.pTouches[ n ].flags.end_event ? True( isolate ) : False( isolate ) );
				touch->Set( context, localStringExternal( isolate, "x" )
						  , Number::New( isolate, evt->data.touch.pTouches[ n ].x ) );
				touch->Set( context, localStringExternal( isolate, "y" )
						  , Number::New( isolate, evt->data.touch.pTouches[ n ].y ) );
				touches->Set( context, n, touch );
			}
			Deallocate( PINPUT_POINT, evt->data.touch.pTouches );
			object = touches;
		}
		break;
	case Event_Render_Mouse:
		{
			if( c->mouse_object.IsEmpty() ) {
					object = Object::New( isolate );
				c->mouse_object.Reset( isolate, object );
			}
			else
				object = c->mouse_object.Get( isolate );

			object->Set( context, localStringExternal( isolate, "x" ), Number::New( isolate, evt->data.mouse.x ) );
			object->Set( context, localStringExternal( isolate, "y" ), Number::New( isolate, evt->data.mouse.y ) );
			object->Set( context, localStringExternal( isolate, "b" ), Number::New( isolate, evt->data.mouse.b ) );
		}
		break;
	case Event_Render_Draw:
		if (r->surface.IsEmpty()) {
			Local<Object> img= ImageObject::NewImage(isolate, GetDisplayImage(r->r), TRUE);
			r->surface.Reset(isolate, img);
			if (r->has_webgpu_context) {
				ImageObject* io = ImageObject::Unwrap<ImageObject>(img);
				io->pii = render_global.pri_gpu_image;
			}
		}
		return Local<Object>::New( isolate, r->surface );
	case Event_Render_Key:
		return Number::New( isolate, evt->data.key.code );
	case Event_Render_MouseExpanded: {
		Local<Object> me = Object::New( isolate );
		me->Set( context, localStringExternal( isolate, "type" ),
			localStringExternal( isolate, evt->data.mouseExpanded.type ) );
		me->Set( context, localStringExternal( isolate, "x" ),
			Number::New( isolate, evt->data.mouseExpanded.x ) );
		me->Set( context, localStringExternal( isolate, "y" ),
			Number::New( isolate, evt->data.mouseExpanded.y ) );
		me->Set( context, localStringExternal( isolate, "clientX" ),
			Number::New( isolate, evt->data.mouseExpanded.x ) );
		me->Set( context, localStringExternal( isolate, "clientY" ),
			Number::New( isolate, evt->data.mouseExpanded.y ) );
		me->Set( context, localStringExternal( isolate, "offsetX" ),
			Number::New( isolate, evt->data.mouseExpanded.x ) );
		me->Set( context, localStringExternal( isolate, "offsetY" ),
			Number::New( isolate, evt->data.mouseExpanded.y ) );
		me->Set( context, localStringExternal( isolate, "movementX" ),
			Number::New( isolate, evt->data.mouseExpanded.movementX ) );
		me->Set( context, localStringExternal( isolate, "movementY" ),
			Number::New( isolate, evt->data.mouseExpanded.movementY ) );
		me->Set( context, localStringExternal( isolate, "button" ),
			Number::New( isolate, evt->data.mouseExpanded.button ) );
		me->Set( context, localStringExternal( isolate, "buttons" ),
			Number::New( isolate, evt->data.mouseExpanded.buttons ) );
		me->Set( context, localStringExternal( isolate, "shiftKey" ),
			Boolean::New( isolate, !!evt->data.mouseExpanded.shiftKey ) );
		me->Set( context, localStringExternal( isolate, "ctrlKey" ),
			Boolean::New( isolate, !!evt->data.mouseExpanded.ctrlKey ) );
		me->Set( context, localStringExternal( isolate, "altKey" ),
			Boolean::New( isolate, !!evt->data.mouseExpanded.altKey ) );
		me->Set( context, localStringExternal( isolate, "metaKey" ),
			False( isolate ) );
		if( evt->data.mouseExpanded.deltaX != 0
		 || evt->data.mouseExpanded.deltaY != 0
		 || evt->data.mouseExpanded.type[0] == 'w' /* "wheel" */ ) {
			me->Set( context, localStringExternal( isolate, "deltaX" ),
				Number::New( isolate, evt->data.mouseExpanded.deltaX ) );
			me->Set( context, localStringExternal( isolate, "deltaY" ),
				Number::New( isolate, evt->data.mouseExpanded.deltaY ) );
			me->Set( context, localStringExternal( isolate, "deltaZ" ),
				Number::New( isolate, 0 ) );
			me->Set( context, localStringExternal( isolate, "deltaMode" ),
				Number::New( isolate, 0 ) );  // 0 = pixel
		}
		attachEventMethods( isolate, context, me );
		return me;
	}
	case Event_Render_KeyExpanded: {
		Local<Object> ke = Object::New( isolate );
		uint32_t packed = evt->data.keyExpanded.packed;
		bool down  = ( packed & KEY_PRESSED ) != 0;
		uint32_t mods = KEY_MOD( packed );
		uint32_t vk   = packed & 0xFFFF;

		ke->Set( context, localStringExternal( isolate, "type" ),
			localStringExternal( isolate, down ? "keydown" : "keyup" ) );
		// keyCode/which: DOM contract is Win32 VK numbers cross-platform.
		// On Windows this is passthrough; Linux build needs a scancode→VK
		// translation table (TODO in sack_dom_key_names.h).
		uint32_t domKeyCode = sack_vk_to_dom_keyCode( vk );
		ke->Set( context, localStringExternal( isolate, "keyCode" ),
			Number::New( isolate, domKeyCode ) );
		ke->Set( context, localStringExternal( isolate, "which" ),
			Number::New( isolate, domKeyCode ) );
		ke->Set( context, localStringExternal( isolate, "shiftKey" ),
			Boolean::New( isolate, ( mods & KEY_MOD_SHIFT ) != 0 ) );
		ke->Set( context, localStringExternal( isolate, "ctrlKey" ),
			Boolean::New( isolate, ( mods & KEY_MOD_CTRL ) != 0 ) );
		ke->Set( context, localStringExternal( isolate, "altKey" ),
			Boolean::New( isolate, ( mods & KEY_MOD_ALT ) != 0 ) );
		ke->Set( context, localStringExternal( isolate, "metaKey" ),
			False( isolate ) );  // Phase 2: track VK_LWIN/RWIN make/break

		// event.key — named-key table first, else GetKeyText (printables).
		const char *keyName  = ( vk < 256 ) ? kDomKeyName [ vk ] : NULL;
		const char *codeName = ( vk < 256 ) ? kDomCodeName[ vk ] : NULL;
		if( keyName ) {
			ke->Set( context, localStringExternal( isolate, "key" ),
				localStringExternal( isolate, keyName ) );
		} else if( evt->data.keyExpanded.text ) {
			Local<String> s = String::NewFromUtf8( isolate,
				evt->data.keyExpanded.text,
				NewStringType::kNormal,
				(int)evt->data.keyExpanded.textLen ).ToLocalChecked();
			ke->Set( context, localStringExternal( isolate, "key" ), s );
		} else {
			ke->Set( context, localStringExternal( isolate, "key" ),
				String::Empty( isolate ) );
		}
		// event.code — physical key position from the same table.
		if( codeName ) {
			ke->Set( context, localStringExternal( isolate, "code" ),
				localStringExternal( isolate, codeName ) );
		} else {
			ke->Set( context, localStringExternal( isolate, "code" ),
				String::Empty( isolate ) );
		}

		// Phase 2 will move this `repeat` calc into the sack-thread callback
		// (held-set racing across the queue is fine for one renderer, but
		// not strictly correct for fast key streams). Good enough for now.
		bool wasHeld = r->keysHeld_.count( vk ) > 0;
		bool repeat  = down && wasHeld;
		if( down ) r->keysHeld_.insert( vk );
		else       r->keysHeld_.erase( vk );
		ke->Set( context, localStringExternal( isolate, "repeat" ),
			Boolean::New( isolate, repeat ) );

		if( evt->data.keyExpanded.text ) {
			Deallocate( char*, evt->data.keyExpanded.text );
			evt->data.keyExpanded.text = NULL;
		}
		attachEventMethods( isolate, context, ke );
		return ke;
	}
	default:
		lprintf( "Unhandled event %d(%04x)", evt->type, evt->type );
		return Undefined( isolate );
	}
	return object;
}

static void asyncmsg_( v8::Isolate* isolate, Local<Context>context, RenderObject *myself ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	{
		struct event *evt;

		while( evt = (struct event *)DequeLink( &myself->receive_queue ) ) {

			Local<Value> object = ProcessEvent( isolate, evt, myself );
			Local<Value> argv[] = { object };

			// Build up to two callbacks per event — a specific one and
			// (for the expanded variants) an optional unified one.
			Local<Function> cb;
			PLIST cbList = nullptr;
			switch( evt->type ){
			case Event_Render_Pen:
				cb = Local<Function>::New( isolate, myself->cbPen );
				break;
			case Event_Render_Touch:
				cb = Local<Function>::New( isolate, myself->cbTouch );
				break;
			case Event_Render_Mouse:
				cb = Local<Function>::New( isolate, myself->cbMouse );
				break;
			case Event_Render_Key:
				cb = Local<Function>::New( isolate, myself->cbKey );
				break;
			case Event_Render_Draw:
				cb = Local<Function>::New( isolate, myself->cbDraw );
				break;
			case Event_Render_MouseExpanded: {
				// type field on the payload picks the specific slot.
				const char* t = evt->data.mouseExpanded.type;
				if (!strcmp(t, "mousedown")) { cb.Clear(); cbList = myself->cbMouseDown_listeners; }
				else if( !strcmp( t, "mouseup"   ) ) { cb.Clear(); cbList = myself->cbMouseUp_listeners; }
				else if( !strcmp( t, "mousemove" ) ) { cb.Clear(); cbList = myself->cbMouseMove_listeners; }
				else if( !strcmp( t, "wheel"     ) ) { cb.Clear(); cbList = myself->cbWheel_listeners; }
				break;
			}
			case Event_Render_KeyExpanded: {
				uint32_t packed = evt->data.keyExpanded.packed;
				bool down = ( packed & KEY_PRESSED ) != 0;
				cbList = down ? myself->cbKeyDown_listeners : myself->cbKeyUp_listeners;
				cb.Clear();
				break;
			}
			}
			Local<Value> r;
			if( cbList ) {
				// Fire each listener; aggregate return values into an array
				// (so JS callers that read e.success see all answers). Two
				// independent event flags affect the loop:
				//   preventDefault           → sets `consumed` private; we
				//                              force r truthy so sack's
				//                              evt->success comes back
				//                              non-zero (event handled).
				//                              Does NOT break the loop.
				//   stopImmediatePropagation → sets `stopped` private; we
				//                              break the loop. Does NOT
				//                              alter r unless preventDefault
				//                              was also called.
				Local<Object> evtObj
					= argv[ 0 ]->IsObject()
					? argv[ 0 ].As<Object>()
					: Local<Object>();
				INDEX idx;
				PERSISTENT_FUNCTION *pcb;
				LIST_FORALL( cbList, idx, PERSISTENT_FUNCTION*, pcb ) {
					if( !pcb ) continue;  // listener was off()'d (SetLink to null)
					Local<Function> lcb = pcb->Get( isolate );
					Local<Value> thisResult
						= lcb->Call( context, context->Global(), 1, argv ).ToLocalChecked();
					if( r.IsEmpty() ) {
						r = thisResult;
					} else if( r->IsArray() ) {
						Local<Array> rArr = Local<Array>::Cast( r );
						(void)rArr->Set( context, rArr->Length(), thisResult );
					} else {
						Local<Array> rArr = Array::New( isolate, 2 );
						(void)rArr->Set( context, 0, r );
						(void)rArr->Set( context, 1, thisResult );
						r = rArr;
					}
					if( evtObj.IsEmpty() ) continue;
					if( eventConsumed( isolate, context, evtObj ) ) {
						r = True( isolate );  // force success non-zero
					}
					if( eventStopped( isolate, context, evtObj ) ) {
						break;
					}
				}
			}
			else if( !cb.IsEmpty() ) {
				r = cb->Call( context, context->Global(), 1, argv ).ToLocalChecked();
			}
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
	//lprintf( "done calling message notice." );
}

static void asyncmsg( uv_async_t* handle ) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	RenderObject* myself = (RenderObject*)handle->data;
	asyncmsg_( isolate, context, myself );
	{
		class constructorSet* c = getConstructors( isolate );
		if( !c->ThreadObject_idleProc.IsEmpty() ) {
			Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
			cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
		}
	}
}

struct asyncMsgTask : SackTask {
	RenderObject *myself;
	asyncMsgTask( RenderObject *myself ) : myself( myself ) {}
	void Run2( Isolate *isolate, Local<Context> context ) {
		asyncmsg_( isolate, context, myself );
	}
};

void RenderObject::sigint( void ) {
	RenderObject *r;
	INDEX idx;
	LIST_FORALL( render_global.renderers, idx, RenderObject *, r ){
		r->do_close();
	}
}

void RenderObject::Init( Local<Object> exports ) {
	{
		//extern void Syslog
	}
	render_global.pri_gpu_image = (PIMAGE_INTERFACE)GetInterface( "webgpu.image" );

	// this should just be this?
	//render_global.pri_gpu_render = GetInterface( "webgpu.render" );

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
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "setTouch", RenderObject::setTouch );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "setPen", RenderObject::setPen );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "setKey", RenderObject::setKey );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "show", RenderObject::show );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "hide", RenderObject::hide );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "reveal", RenderObject::reveal );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "redraw", RenderObject::redraw );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "update", RenderObject::update );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "close", RenderObject::close );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "lockMouse", RenderObject::lockMouse);
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "unlockMouse", RenderObject::unlockMouse);
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "on", RenderObject::on );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "off", RenderObject::off );
		NODE_SET_PROTOTYPE_METHOD( renderTemplate, "getContext", RenderObject::getContext);

		renderTemplate->PrototypeTemplate()->SetAccessorProperty( localStringExternal( isolate, "size" )
			, FunctionTemplate::New( isolate, RenderObject::getCoordinate, Integer::New( isolate, 12 ) )
			, FunctionTemplate::New( isolate, RenderObject::setCoordinate, Integer::New( isolate, 11 ) )
			, DontDelete );
		renderTemplate->PrototypeTemplate()->SetAccessorProperty(localStringExternal(isolate, "width")
			, FunctionTemplate::New(isolate, RenderObject::getCoordinate, Integer::New(isolate, 2))
			, FunctionTemplate::New(isolate, RenderObject::setCoordinate, Integer::New(isolate, 2))
			, DontDelete);
		renderTemplate->PrototypeTemplate()->SetAccessorProperty(localStringExternal(isolate, "height")
			, FunctionTemplate::New(isolate, RenderObject::getCoordinate, Integer::New(isolate, 3))
			, FunctionTemplate::New(isolate, RenderObject::setCoordinate, Integer::New(isolate, 3))
			, DontDelete);
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

		Local<Object> attributes = Object::New(isolate);
		SET_READONLY(renderFunc, "attributes", attributes);
#define SetStyle( object, style ) SET_READONLY( object, #style, Integer::New( isolate, style ) );

		SetStyle(attributes, DISPLAY_ATTRIBUTE_LAYERED );
		SetStyle(attributes, DISPLAY_ATTRIBUTE_CHILD);
		SetStyle(attributes, PANEL_ATTRIBUTE_ALPHA);
		SetStyle(attributes, PANEL_ATTRIBUTE_HOLEY);
		SetStyle(attributes, PANEL_ATTRIBUTE_EXCLUSIVE);
		SetStyle(attributes, PANEL_ATTRIBUTE_INTERNAL);
		SetStyle(attributes, DISPLAY_ATTRIBUTE_NO_MOUSE);
		SetStyle(attributes, DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS);
		SetStyle(attributes, DISPLAY_ATTRIBUTE_TOPMOST);
		SetStyle(attributes, DISPLAY_ATTRIBUTE_NO_REDIRECT);


    class constructorSet* c = getConstructors( isolate );
    c->RenderObject_constructor.Reset( isolate, renderTemplate->GetFunction(context).ToLocalChecked() );
		SET_READONLY( exports, "Renderer", renderTemplate->GetFunction(context).ToLocalChecked() );
		SET_READONLY( renderTemplate->GetFunction(context).ToLocalChecked(), "getDisplay"
				, Function::New( context, RenderObject::getDisplay ).ToLocalChecked() );
	}

RenderObject::RenderObject( int attr, const char *title, int x, int y, int w, int h, RenderObject *over )  {
	AddLink( &render_global.renderers, this );
	if (title) {
		r = OpenDisplayAboveSizedAt(attr, w, h, x, y, over ? over->r : NULL);
	} else
		r = NULL;
	receive_queue = NULL;
	closed = 0;
	has_webgpu_context = 0;
}

void RenderObject::setRenderer(PRENDERER r) {

}

RenderObject::~RenderObject() {
	DeleteLink( &render_global.renderers, this );
}

	void RenderObject::New( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		Local<Context> context = isolate->GetCurrentContext();
		if( args.IsConstructCall() && ( args.This()->InternalFieldCount() == 1)  ) {

			char *title = NULL;
			int x = 0, y = 0, w = 1024, h = 768, border = 0;
			int attributes = DISPLAY_ATTRIBUTE_LAYERED;
			Local<Object> parent_object;
			RenderObject *parent = NULL;

			int argc = args.Length();

			// Options-object form:
			//   new Renderer({ title, x, y, width, height, flags, style, parent })
			// All fields optional. `flags` and `style` both map to attributes
			// (style is an alias — pick whichever reads better in user code).
			// If first arg is a plain object (not a string), use this path
			// and ignore any further positional args.
			if( argc > 0 && args[0]->IsObject() && !args[0]->IsString() ) {
				Local<Object> opts = args[0].As<Object>();

				auto getInt = [&]( const char *name, int dflt ) -> int {
					Local<Value> v;
					if( opts->Get( context, localStringExternal( isolate, name ) ).ToLocal( &v )
					    && v->IsNumber() )
						return (int)v->IntegerValue( context ).ToChecked();
					return dflt;
				};
				auto getStr = [&]( const char *name ) -> char * {
					Local<Value> v;
					if( opts->Get( context, localStringExternal( isolate, name ) ).ToLocal( &v )
					    && v->IsString() ) {
						String::Utf8Value s( isolate, v );
						return StrDup( *s );
					}
					return NULL;
				};

				title      = getStr( "title" );
				x          = getInt( "x",      x );
				y          = getInt( "y",      y );
				w          = getInt( "width",  w );
				h          = getInt( "height", h );
				attributes = getInt( "flags",  attributes );

				Local<Value> pv;
				if( opts->Get( context, localStringExternal( isolate, "parent" ) ).ToLocal( &pv )
				    && pv->IsObject() && !pv->IsNull() ) {
					parent_object = pv->ToObject( context ).ToLocalChecked();
					parent = ObjectWrap::Unwrap<RenderObject>( parent_object );
				}
			}
			else {
				// Positional form:
				//   new Renderer(title, x, y, width, height, parent, attributes)
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
				if (argc > 6) {
					attributes = (int)args[6]->IntegerValue(context).ToChecked();
				}
			}
			// Invoked as constructor: `new MyObject(...)`
			RenderObject* obj = new RenderObject( attributes, title?title:"Node Application", x, y, w, h, parent );
			obj->this_.Reset( isolate, args.This() );
			MemSet( &obj->async, 0, sizeof( obj->async ) );
			obj->c = getConstructors( isolate );
			if( !( obj->ivm_hosted = ( obj->c->ivm_holder != nullptr ) ) )
				uv_async_init( obj->c->loop, &obj->async, asyncmsg );
			//uv_unref( (uv_handle_t*)obj->async );
			obj->isolate = isolate;
			obj->eventThread = MakeThread();

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
			delete[] argv;
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
	case 2:
		args.GetReturnValue().Set(Integer::New(isolate, w));
		return;
	case 3:
		args.GetReturnValue().Set( Integer::New(isolate, h));
		return;
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
		case 2:
			w = (int32_t)o->IntegerValue(context).ToChecked();
			{
				uint32_t h;
				GetDisplayPosition(me->r, nullptr, nullptr, nullptr, &h);
				SizeDisplay(me->r, w, h);
			}
			break;
		case 3:
			h = (int32_t)o->IntegerValue(context).ToChecked();
			{
				uint32_t w;
				GetDisplayPosition(me->r, nullptr, nullptr, &w, nullptr);
				SizeDisplay(me->r, w, h);
			}
			break;
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

void RenderObject::do_close( void ) {
	if( !this->closed ) {
		this->closed = TRUE;
		lprintf( "Close async" );
		uv_unref( (uv_handle_t*)&this->async );

		//uv_close( (uv_handle_t*)&this->async, NULL );
	}
}


void RenderObject::close( const FunctionCallbackInfo<Value>& args ) {
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	r->do_close();
}

void RenderObject::lockMouse(const FunctionCallbackInfo<Value>& args) {
	RenderObject* r = ObjectWrap::Unwrap<RenderObject>(args.This());
	OwnMouse(r->r, TRUE);
}

void RenderObject::unlockMouse(const FunctionCallbackInfo<Value>& args) {
	RenderObject* r = ObjectWrap::Unwrap<RenderObject>(args.This());
	OwnMouse(r->r, FALSE);
}

void RenderObject::getContext( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	if( args.Length() < 1 || !args[ 0 ]->IsString() ) {
		isolate->ThrowException( Exception::TypeError( localStringExternal(
			isolate, TranslateText( "First argument must be a context type string." ) ) ) );
		return;
	}
	RenderObject* r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	String::Utf8Value mName( isolate, args[ 0 ] );

	if( StrCaseCmp( *mName, "webgpu" ) == 0 ) {
#ifdef INCLUDE_DAWN
		// WebGPU spec marks GPUCanvasContext as [SameObject] — repeated
		// getContext('webgpu') calls must return the same instance. Cache it
		// via a private symbol on the renderer's JS object. Without this,
		// THREE re-calls getContext between configure and getCurrentTexture
		// and ends up with an unconfigured second instance.
		Local<Context> jsCtx = isolate->GetCurrentContext();
		Local<Object> self = args.This();
		Local<Private> key = Private::ForApi( isolate,
			String::NewFromUtf8Literal( isolate, "__webgpuContextCache" ) );
		Local<Value> cached;
		if( self->GetPrivate( jsCtx, key ).ToLocal( &cached )
		    && cached->IsObject() ) {
			args.GetReturnValue().Set( cached );
			return;
		}
		Local<Object> ctx = webgpu_canvas_context_for_renderer( isolate, r->r );
		if( ctx.IsEmpty() ) {
			isolate->ThrowException( Exception::Error( localStringExternal(
				isolate, "WebGPU not initialized or surface creation failed." ) ) );
			return;
		}
		(void)self->SetPrivate( jsCtx, key, ctx );

		// From now on this renderer's surface ImageObject should route
		// through the webgpu.image driver. Flag the display image as a
		// GPU final-render target (no in-memory pixmap mutation) and
		// drop any previously-built surface ImageObject so the next draw
		// callback rebuilds it with the per-object pii override.
		r->has_webgpu_context = 1;
		Image display = GetDisplayImage( r->r );
		if( display ) {
			display->flags |=  IF_FLAG_FINAL_RENDER;
			display->flags &= ~IF_FLAG_IN_MEMORY;
			// Install our driver as the surface's reverse_interface so
			// sack's font code (and any other reverse-aware code) routes
			// its final BlotImage back through us when it sees a FINAL_RENDER
			// dest. Without this, font glyph rendering would still try to
			// poke pixels into the CPU pixmap that isn't there.
			void *iface = sack_image_get_webgpu_reverse_for_render();
			if( iface ) {
				// reverse_interface is declared as `struct image_interface_tag *`
				// inside the namespace selected by the imglib build flags;
				// the void* punt sidesteps the namespace lookup so this
				// compiles whether IMAGE_NAMESPACE expanded to ::Interface
				// or directly into sack::image.
				display->reverse_interface =
					reinterpret_cast<decltype(display->reverse_interface)>( iface );
				display->reverse_interface_instance = display;
			}
		}
		if( !r->surface.IsEmpty() )
			r->surface.Reset();

		args.GetReturnValue().Set( ctx );
#else
		isolate->ThrowException( Exception::Error( localStringExternal(
			isolate, "WebGPU not built into this binary." ) ) );
#endif
		return;
	}
	if( StrCaseCmp( *mName, "2d" )     == 0
	 || StrCaseCmp( *mName, "webgl" )  == 0
	 || StrCaseCmp( *mName, "webgl2" ) == 0 ) {
		isolate->ThrowException( Exception::Error( localStringExternal(
			isolate, "Context type not implemented." ) ) );
		return;
	}
	// Unknown type — browsers return null per spec.
	args.GetReturnValue().Set( Null( isolate ) );
}

void RenderObject::getImage( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );

	// Build the surface ImageObject. If the renderer has a webgpu context
	// attached, bind the per-object pii to the webgpu.image driver so JS
	// draw calls record into render bundles. Otherwise (CPU rendering or
	// webgpu not built) leave pii NULL → global default.
	struct image_interface_tag *pii = r->has_webgpu_context
		? (struct image_interface_tag *)render_global.pri_gpu_image
		: NULL;
	Local<Object> img = ImageObject::NewImage(isolate, GetDisplayImage(r->r), TRUE, pii);
	r->surface.Reset( isolate, img );
	args.GetReturnValue().Set( img );
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
		r->updated = 1;
	}
	else  {
		UpdateDisplayPortion( r->r, 0, 0, 0, 0 );
		r->updated = 1;
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
	//Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	// UpdateDisplay deadlocks; so use this method instead....
	// this means the display is not nessecarily shown when this returns, but will be.
	RestoreDisplay( r->r );
}

void RenderObject::hide( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	HideDisplay( r->r );
}

void RenderObject::reveal( const FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	RestoreDisplay( r->r );
}

static uintptr_t CPROC doMouse( uintptr_t psv, int32_t x, int32_t y, uint32_t b ) {
	RenderObject *r = (RenderObject *)psv;
	//lprintf( "Mouse:%d,%d,%d", x, y, b );
	if( r->closed ) return 0;

	// Always fire the original "mouse" event (back-compat).
	uintptr_t rv = MakeEvent( r, Event_Render_Mouse, x, y, b );

	// Synthesize browser-shaped events from the bitmask diff. We only do
	// the diff work if at least one expanded listener is registered, so
	// JS code that hasn't opted in pays nothing.
	bool wantExpanded =
		   r->cbMouseDown_listeners  || r->cbMouseUp_listeners
		|| r->cbMouseMove_listeners  || r->cbWheel_listeners;
	if( !wantExpanded ) return rv;

	uint32_t prev = r->lastMouseButtons_;
	uint32_t buttonBits = b & MK_SOMEBUTTON;
	uint32_t pressed   = buttonBits & ~prev;
	uint32_t released  = prev & ~buttonBits;

	uint8_t shift = (b & MK_SHIFT)   ? 1 : 0;
	uint8_t ctrl  = (b & MK_CONTROL) ? 1 : 0;
	uint8_t alt   = (b & MK_ALT)     ? 1 : 0;
	uint32_t browserButtons = sackButtonsToBrowserButtons( buttonBits );

	int32_t dx = r->hasLastMousePos_ ? (x - r->lastMouseX_) : 0;
	int32_t dy = r->hasLastMousePos_ ? (y - r->lastMouseY_) : 0;

	// One mousedown per newly-pressed bit.
	while( pressed ) {
		uint32_t lowBit = pressed & (uint32_t)-(int32_t)pressed;
		int idx = 0; uint32_t tmp = lowBit; while( tmp >>= 1 ) idx++;
		int browserBtn = sackBitToBrowserButton( idx );
		MakeEvent( r, Event_Render_MouseExpanded,
			"mousedown", x, y, (int32_t)0, (int32_t)0,
			(int)browserBtn, browserButtons,
			(double)0.0, (double)0.0,
			(int)shift, (int)ctrl, (int)alt );
		pressed &= pressed - 1;
	}
	// One mouseup per newly-released bit.
	while( released ) {
		uint32_t lowBit = released & (uint32_t)-(int32_t)released;
		int idx = 0; uint32_t tmp = lowBit; while( tmp >>= 1 ) idx++;
		int browserBtn = sackBitToBrowserButton( idx );
		MakeEvent( r, Event_Render_MouseExpanded,
			"mouseup", x, y, (int32_t)0, (int32_t)0,
			(int)browserBtn, browserButtons,
			(double)0.0, (double)0.0,
			(int)shift, (int)ctrl, (int)alt );
		released &= released - 1;
	}
	// Move if position actually changed.
	if( r->hasLastMousePos_ && (dx != 0 || dy != 0) ) {
		MakeEvent( r, Event_Render_MouseExpanded,
			"mousemove", x, y, dx, dy,
			(int)-1, browserButtons,
			(double)0.0, (double)0.0,
			(int)shift, (int)ctrl, (int)alt );
	}
	// Wheel — sack delivers scroll as transient bits in the same b.
	if( b & MK_SCROLL_UP ) MakeEvent( r, Event_Render_MouseExpanded,
		"wheel", x, y, (int32_t)0, (int32_t)0, (int)-1, browserButtons,
		(double)0.0, (double)-120.0, (int)shift, (int)ctrl, (int)alt );
	if( b & MK_SCROLL_DOWN ) MakeEvent( r, Event_Render_MouseExpanded,
		"wheel", x, y, (int32_t)0, (int32_t)0, (int)-1, browserButtons,
		(double)0.0, (double)120.0, (int)shift, (int)ctrl, (int)alt );
	if( b & MK_SCROLL_LEFT ) MakeEvent( r, Event_Render_MouseExpanded,
		"wheel", x, y, (int32_t)0, (int32_t)0, (int)-1, browserButtons,
		(double)-120.0, (double)0.0, (int)shift, (int)ctrl, (int)alt );
	if( b & MK_SCROLL_RIGHT ) MakeEvent( r, Event_Render_MouseExpanded,
		"wheel", x, y, (int32_t)0, (int32_t)0, (int)-1, browserButtons,
		(double)120.0, (double)0.0, (int)shift, (int)ctrl, (int)alt );

	r->lastMouseButtons_ = buttonBits;
	r->lastMouseX_       = x;
	r->lastMouseY_       = y;
	r->hasLastMousePos_  = true;

	return rv;
}

void RenderObject::setMouse( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	Local<Function> arg0 = Local<Function>::Cast( args[0] );
	r->cbMouse.Reset( isolate, arg0 );
	SetMouseHandler( r->r, doMouse, (uintptr_t)r );
}

static int CPROC doTouch( uintptr_t psvUser, PINPUT_POINT pTouches, int nTouches ) {
	RenderObject *r = (RenderObject *)psvUser;
	if( !r->closed ) {
		PINPUT_POINT p = NewArray( struct input_point, nTouches );
		MemCpy( p, pTouches, sizeof( struct input_point ) * nTouches );
		return MakeEvent( r, Event_Render_Touch, p, nTouches );
	}
	return 0;
}

void RenderObject::setTouch( const FunctionCallbackInfo<Value> &args ) {
#ifndef NO_TOUCH
	Isolate *isolate     = args.GetIsolate();
	RenderObject *r      = ObjectWrap::Unwrap<RenderObject>( args.This() );
	Local<Function> arg0 = Local<Function>::Cast( args[ 0 ] );
	r->cbTouch.Reset( isolate, arg0 );
	SetTouchHandler( r->r, doTouch, (uintptr_t)r );
#endif
}


static int CPROC doPen( uintptr_t psvUser, PPEN_EVENT pEvent ) {
	RenderObject *r = (RenderObject *)psvUser;
	if( !r->closed )
		return MakeEvent( r, Event_Render_Pen, pEvent );
	return 0;
}

void RenderObject::setPen( const FunctionCallbackInfo<Value> &args ) {
#ifndef NO_PEN
	Isolate *isolate     = args.GetIsolate();
	RenderObject *r      = ObjectWrap::Unwrap<RenderObject>( args.This() );
	Local<Function> arg0 = Local<Function>::Cast( args[ 0 ] );
	r->cbMouse.Reset( isolate, arg0 );
	SetPenHandler( r->r, doPen, (uintptr_t)r );
#endif
}

static int CPROC doRedraw( uintptr_t psv, PRENDERER out ) {
	RenderObject *r = (RenderObject *)psv;
	PTHREAD waiter = MakeThread();
	// sometimes the redraw event can happen on the same thread.
	if( !r->closed )
		if( waiter == r->eventThread ){
			if( r->surface.IsEmpty() ) {
				struct image_interface_tag *pii = r->has_webgpu_context
					? (struct image_interface_tag *)render_global.pri_gpu_image
					: NULL;
				r->surface.Reset( r->isolate,
					ImageObject::NewImage( r->isolate, GetDisplayImage( r->r ), TRUE, pii ) );
			}
			Local<Value> argv[] = { Local<Object>::New( r->isolate, r->surface ) };
			Local<Function> cb;
			cb = Local<Function>::New( r->isolate, r->cbDraw );
			//lprintf( "Dispatch to JS callback (redraw)");
			Local<Value> result = cb->Call( r->isolate->GetCurrentContext(), Local<Object>::New( r->isolate, r->this_ ), 1, argv ).ToLocalChecked();
			//lprintf( "callback result: %d", result.IsEmpty() );
			if( !result.IsEmpty() ) {
				//lprintf( "callback result: %d", result->IntegerValue(r->isolate->GetCurrentContext()).ToChecked() );
				return (int)result->IntegerValue(r->isolate->GetCurrentContext()).ToChecked();
			}
			return 0;
		}
		else
			return MakeEvent( r, Event_Render_Draw );
	return 0;
}

void RenderObject::setDraw( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	Local<Function> arg0 = Local<Function>::Cast( args[0] );
	SetRedrawHandler( r->r, doRedraw, (uintptr_t)r );
	r->cbDraw.Reset( isolate, arg0 );
	
}


static int CPROC doKey( uintptr_t psv, uint32_t key ) {
	RenderObject *r = (RenderObject *)psv;
	if( r->closed ) return 0;

	int rv = (int)MakeEvent( r, Event_Render_Key, key );

	bool wantExpanded = r->cbKeyDown_listeners
	                 || r->cbKeyUp_listeners;
	if( wantExpanded ) {
		// MUST run here — GetKeyText reads live OS state (dead keys,
		// IME, current shift register). Deferring to the V8 thread
		// would resolve against stale state.
		const TEXTCHAR* txt = GetKeyText( (int)key );
		MakeEvent( r, Event_Render_KeyExpanded, key, txt );
	}
	return rv;
}


void RenderObject::setKey( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	RenderObject *r = ObjectWrap::Unwrap<RenderObject>( args.This() );
	Local<Function> arg0 = Local<Function>::Cast( args[0] );
	SetKeyboardHandler( r->r, doKey, (uintptr_t)r );
	r->cbKey.Reset( isolate, arg0 );
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
		SetRedrawHandler( r->r, doRedraw, (uintptr_t)r );
		r->cbDraw.Reset( isolate, arg1 );
	}
	else if( StrCmp( *fName, "mouse" ) == 0 ) {
		SetMouseHandler( r->r, doMouse, (uintptr_t)r );
		r->cbMouse.Reset( isolate, arg1 );
	}
	else if( StrCmp( *fName, "touch" ) == 0 ) {
#ifndef NO_TOUCH
		SetTouchHandler( r->r, doTouch, (uintptr_t)r );
		r->cbTouch.Reset( isolate, arg1 );
#endif
	}
	else if( StrCmp( *fName, "pen" ) == 0 ) {
#ifndef NO_PEN
		SetPenHandler( r->r, doPen, (uintptr_t)r );
		r->cbPen.Reset( isolate, arg1 );
#endif
	}
	else if( StrCmp( *fName, "key" ) == 0 ) {
		SetKeyboardHandler( r->r, doKey, (uintptr_t)r );
		r->cbKey.Reset( isolate, arg1 );
	}
	// Browser-shaped mouse events. All four route through doMouse which
	// diffs sack's bitmask to synthesize down/up/move/wheel transitions.
	else if( StrCmp( *fName, "mouseDown" ) == 0 || StrCmp( *fName, "mousedown" ) == 0 ) {
		SetMouseHandler( r->r, doMouse, (uintptr_t)r );
		PERSISTENT_FUNCTION* ppf = new PERSISTENT_FUNCTION();
		ppf->Reset(isolate, arg1);
		AddLink(&r->cbMouseDown_listeners, ppf);
		//r->cbMouseDown.Reset( isolate, arg1 );
	}
	else if( StrCmp( *fName, "mouseUp" ) == 0 || StrCmp( *fName, "mouseup" ) == 0 ) {
		SetMouseHandler( r->r, doMouse, (uintptr_t)r );
		PERSISTENT_FUNCTION* ppf = new PERSISTENT_FUNCTION();
		ppf->Reset(isolate, arg1);
		AddLink(&r->cbMouseUp_listeners, ppf);
		//r->cbMouseUp.Reset( isolate, arg1 );
	}
	else if( StrCmp( *fName, "mouseMove" ) == 0 || StrCmp( *fName, "mousemove" ) == 0 ) {
		SetMouseHandler( r->r, doMouse, (uintptr_t)r );
		PERSISTENT_FUNCTION* ppf = new PERSISTENT_FUNCTION();
		ppf->Reset(isolate, arg1);
		AddLink(&r->cbMouseMove_listeners, ppf);
		//r->cbMouseMove.Reset( isolate, arg1 );
	}
	else if( StrCmp( *fName, "wheel" ) == 0 || StrCmp(*fName, "DOMMouseScroll") == 0 || StrCmp(*fName, "mousewheel") == 0) {
		SetMouseHandler( r->r, doMouse, (uintptr_t)r );
		PERSISTENT_FUNCTION* ppf = new PERSISTENT_FUNCTION();
		ppf->Reset(isolate, arg1);
		AddLink(&r->cbWheel_listeners, ppf);
		//r->cbWheel.Reset( isolate, arg1 );
	}
	// Browser-shaped keyboard events.
	else if( StrCmp( *fName, "keyDown" ) == 0 || StrCmp( *fName, "keydown" ) == 0 ) {
		SetKeyboardHandler( r->r, doKey, (uintptr_t)r );
		PERSISTENT_FUNCTION* ppf = new PERSISTENT_FUNCTION();
		ppf->Reset(isolate, arg1);
		AddLink(&r->cbKeyDown_listeners, ppf);
		//r->cbKeyDown.Reset( isolate, arg1 );
	}
	else if( StrCmp( *fName, "keyUp" ) == 0 || StrCmp( *fName, "keyup" ) == 0 ) {
		SetKeyboardHandler( r->r, doKey, (uintptr_t)r );
		PERSISTENT_FUNCTION* ppf = new PERSISTENT_FUNCTION();
		ppf->Reset(isolate, arg1);
		AddLink(&r->cbKeyUp_listeners, ppf);
		//r->cbKeyUp.Reset( isolate, arg1 );
	}
}

void RenderObject::off(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	RenderObject* r = ObjectWrap::Unwrap<RenderObject>(args.This());
	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::Error(
			localStringExternal(isolate, TranslateText("First argument must be event name as a string."))));
		return;
	}
	if (!args[1]->IsFunction()) {
		isolate->ThrowException(Exception::Error(
			localStringExternal(isolate, TranslateText("Second argument must be callback function."))));
		return;
	}
	Local<Function> arg1 = Local<Function>::Cast(args[1]);

	String::Utf8Value fName(isolate, args[0]);
	if (StrCmp(*fName, "draw") == 0) {
		r->cbDraw.Reset();
	}
	else if (StrCmp(*fName, "mouse") == 0) {
		r->cbMouse.Reset();
	}
	else if (StrCmp(*fName, "touch") == 0) {
#ifndef NO_TOUCH
		r->cbTouch.Reset();
#endif
	}
	else if (StrCmp(*fName, "pen") == 0) {
#ifndef NO_PEN
		r->cbPen.Reset();
#endif
	}
	else if (StrCmp(*fName, "key") == 0) {
		r->cbKey.Reset();
	}
	// Browser-shaped mouse events. All four route through doMouse which
	// diffs sack's bitmask to synthesize down/up/move/wheel transitions.
	else if (StrCmp(*fName, "mouseDown") == 0 || StrCmp(*fName, "mousedown") == 0) {
		INDEX idx;
		PERSISTENT_FUNCTION* ppf;
		LIST_FORALL(r->cbMouseDown_listeners, idx, PERSISTENT_FUNCTION*, ppf) {
			if (ppf->Get(isolate)->StrictEquals(arg1)) {
				SetLink(&r->cbMouseDown_listeners, idx, nullptr );
				ppf->Reset();
				delete ppf;
				break;
			}
		}
	}
	else if (StrCmp(*fName, "mouseUp") == 0 || StrCmp(*fName, "mouseup") == 0) {
		INDEX idx;
		PERSISTENT_FUNCTION* ppf;
		LIST_FORALL(r->cbMouseUp_listeners, idx, PERSISTENT_FUNCTION*, ppf) {
			if (ppf->Get(isolate)->StrictEquals(arg1)) {
				SetLink(&r->cbMouseUp_listeners, idx, nullptr);
				ppf->Reset();
				delete ppf;
				break;
			}
		}
	}
	else if (StrCmp(*fName, "mouseMove") == 0 || StrCmp(*fName, "mousemove") == 0) {
		INDEX idx;
		PERSISTENT_FUNCTION* ppf;
		LIST_FORALL(r->cbMouseMove_listeners, idx, PERSISTENT_FUNCTION*, ppf) {
			if (ppf->Get(isolate)->StrictEquals(arg1)) {
				SetLink(&r->cbMouseMove_listeners, idx, nullptr);
				ppf->Reset();
				delete ppf;
				break;
			}
		}
	}
	else if (StrCmp(*fName, "wheel") == 0 || StrCmp(*fName, "DOMMouseScroll") == 0 || StrCmp(*fName, "mousewheel") == 0) {
		INDEX idx;
		PERSISTENT_FUNCTION* ppf;
		LIST_FORALL(r->cbWheel_listeners, idx, PERSISTENT_FUNCTION*, ppf) {
			if (ppf->Get(isolate)->StrictEquals(arg1)) {
				SetLink(&r->cbWheel_listeners, idx, nullptr);
				ppf->Reset();
				delete ppf;
				break;
			}
		}
	}
	// Browser-shaped keyboard events.
	else if (StrCmp(*fName, "keyDown") == 0 || StrCmp(*fName, "keydown") == 0) {
		INDEX idx;
		PERSISTENT_FUNCTION* ppf;
		LIST_FORALL(r->cbKeyDown_listeners, idx, PERSISTENT_FUNCTION*, ppf) {
			if (ppf->Get(isolate)->StrictEquals(arg1)) {
				SetLink(&r->cbKeyDown_listeners, idx, nullptr);
				ppf->Reset();
				delete ppf;
				break;
			}
		}
	}
	else if (StrCmp(*fName, "keyUp") == 0 || StrCmp(*fName, "keyup") == 0) {
		INDEX idx;
		PERSISTENT_FUNCTION* ppf;
		LIST_FORALL(r->cbKeyUp_listeners, idx, PERSISTENT_FUNCTION*, ppf) {
			if (ppf->Get(isolate)->StrictEquals(arg1)) {
				SetLink(&r->cbKeyUp_listeners, idx, nullptr);
				ppf->Reset();
				delete ppf;
				break;
			}
		}
	}
}

uintptr_t MakeEvent( RenderObject *r, enum GUI_eventType type, ... ) {
	event e;
	PLINKQUEUE *queue = &r->receive_queue;
	va_list args;
	va_start( args, type );
	e.type = type;
	e.waiter = MakeThread();
	switch( type ) {
#ifndef NO_PEN
	case Event_Render_Pen:
		PPEN_EVENT pen_event; 
		pen_event = va_arg( args, PPEN_EVENT );
		e.data.pen.event.penFlags = pen_event->penFlags;
		e.data.pen.event.penMask          = pen_event->penMask;
		e.data.pen.event.pressure         = pen_event->pressure;
		e.data.pen.event.rotation         = pen_event->rotation;
		e.data.pen.event.tiltX            = pen_event->tiltX;
		e.data.pen.event.tiltY            = pen_event->tiltY;
		e.data.pen.event.x                = pen_event->x;
		e.data.pen.event.y                = pen_event->y;
		e.data.pen.event.nOverflow        = pen_event->nOverflow;
		e.data.pen.event.pOverflow        = NewArray( struct pen_event, pen_event->nOverflow );
		MemCpy( e.data.pen.event.pOverflow, pen_event->pOverflow, sizeof( struct pen_event ) * pen_event->nOverflow );
		break;
#endif
#ifndef NO_TOUCH
	case Event_Render_Touch:
		e.data.touch.pTouches = va_arg( args, PINPUT_POINT );
		e.data.touch.nTouches = va_arg( args, int );
		break;
#endif
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
	case Event_Render_MouseExpanded:
		// Arg order matches the emit calls in doMouse:
		//   type, x, y, movementX, movementY, button, buttons,
		//   deltaX, deltaY, shiftKey, ctrlKey, altKey
		// Notes: small ints / chars promote to int through varargs;
		// floats promote to double.
		e.data.mouseExpanded.type      = va_arg( args, const char* );
		e.data.mouseExpanded.x         = va_arg( args, int32_t );
		e.data.mouseExpanded.y         = va_arg( args, int32_t );
		e.data.mouseExpanded.movementX = va_arg( args, int32_t );
		e.data.mouseExpanded.movementY = va_arg( args, int32_t );
		e.data.mouseExpanded.button    = (int8_t)va_arg( args, int );
		e.data.mouseExpanded.buttons   = va_arg( args, uint32_t );
		e.data.mouseExpanded.deltaX    = (float)va_arg( args, double );
		e.data.mouseExpanded.deltaY    = (float)va_arg( args, double );
		e.data.mouseExpanded.shiftKey  = (uint8_t)va_arg( args, int );
		e.data.mouseExpanded.ctrlKey   = (uint8_t)va_arg( args, int );
		e.data.mouseExpanded.altKey    = (uint8_t)va_arg( args, int );
		break;
	case Event_Render_KeyExpanded: {
		e.data.keyExpanded.packed = va_arg( args, uint32_t );
		const TEXTCHAR* txt = va_arg( args, const TEXTCHAR* );
		if( txt ) {
			size_t len = StrLen( txt );
			e.data.keyExpanded.text = NewArray( char, len + 1 );
			MemCpy( e.data.keyExpanded.text, txt, len );
			e.data.keyExpanded.text[ len ] = 0;
			e.data.keyExpanded.textLen = len;
		} else {
			e.data.keyExpanded.text    = NULL;
			e.data.keyExpanded.textLen = 0;
		}
		break;
	}
	default:
		lprintf( "Unhandled event requested %d", type );
		break;
	}

	e.flags.complete = 0; 
	e.success = 0;
	EnqueLink( queue, &e );
	if( r->ivm_hosted ) r->c->ivm_post( r->c->ivm_holder, std::make_unique<asyncMsgTask>( r ) );
	else uv_async_send( &r->async );

	while( !e.flags.complete ) IdleFor( 1000 );

	return e.success;
}
