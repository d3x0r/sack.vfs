
import {sack} from "sack-gui"

export let r = null;

const document = {
	addEventListener() {
		console.log( "addEventListener" );
	},
	getElementById( id ) {
		console.log( "getElementById:", id );
	},
   exitPointerLock() {
		r?.unlockMouse();
	},
  createElementNS: () => ({ style: {} }),
	createElement( type ) {
		console.log( "create a:", type );
		if( type === "canvas" ) {
			return {
				r: sack.Renderer( "Hello World", 1600, 800 ),
				get width() { return this.r.width },
				set width(val) { r.width = val },
				get height() { return this.r.height },
				set height(val) { r.height = val },
				style : {},
				getContext(mode) {
					return this.r.getContext( mode );
					console.log( "Return context" );
				}
			}
		}
	}
};

globalThis.document = document;

const callbacks = new Map();
let nextId = 0;


const window = {
	self: null,
	events: {
		keyup: [],
		keydown: [],
	},
	get innerWidth() {
		return r.size.w || 1024;
	},
	get innerHeight() {
		return r.size.h || 768;
	},
	requestAnimationFrame(cb) {


// Browser timing APIs that THREE expects to exist.
    const id = ++nextId;
    const t = setTimeout(() => {
      callbacks.delete(id);
      cb(performance.now());
    }, 16);  // ~60fps
    callbacks.set(id, t);
    return id;
		console.log( "Hook animation frame" );
	},

  cancelAnimationFrame(id) {
    const t = callbacks.get(id);
    if (t) { clearTimeout(t); callbacks.delete(id); }
  },

	addEventListener( event, cb ) {
		switch( event ) {
		case "keyup":
			window.events.keyup.push( (event)=>{ event.preventDefault = ()=>{event.used = true}; return cb(event); } );
			break;
		case "keydown":
			window.events.keydown.push( (event)=>{ event.preventDefault = ()=>{event.used = true}; return cb(event); } );
			break;
		default:
			console.log( "Unhandle addEvent:", event );
		}
	}
	,removeEventListener( event, cb ) {
		switch( event ) {
		case "keyup":
			
			break;
		case "keydown":

			break;
		default:
			console.log( "Unhandle removeEvent:", event );
		}
	},
	setRenderer( rnd ) {
		r = rnd;
	}
};

window.self = window;
globalThis.self = window;
globalThis.window = window;

globalThis.requestAnimationFrame = window.requestAnimationFrame;

if (!globalThis.HTMLImageElement)   globalThis.HTMLImageElement = class {};
if (!globalThis.HTMLCanvasElement)  globalThis.HTMLCanvasElement = class {};
if (!globalThis.ImageBitmap)        globalThis.ImageBitmap = class {};
if (!globalThis.OffscreenCanvas)    globalThis.OffscreenCanvas = class {};

if (!globalThis.document) globalThis.document = {
  body: {},
};