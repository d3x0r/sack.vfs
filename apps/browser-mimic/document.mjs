
import {sack} from "@d3x0r/sack-gui"

export let r = null;
const pendingKey = [];

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
			const canvas = {
				requestPointerLock() {
					r.lockMouse();
					r.hideMouse();
				},
				exitPointerLock() {
					r.unlockMouse();
					r.showMouse();
				},
				r: sack.Renderer( "Hello World", -1, -1, 1600, 800, null, sack.Renderer.attributes.DISPLAY_ATTRIBUTE_NO_REDIRECT ),
				get width() { return canvas.r.width },
				set width(val) { r.width = val },
				get height() { return canvas.r.height },
				set height(val) { r.height = val },
				style : {},
				addEventListener( event, cb ) {
					//console.log( "wanted to add handler to renderer:", event );
					canvas.r.on( event, cb );
				},
				removeEventListener( event, cb ) {
					//console.log( "wanted to remove handler to renderer:", event );
					canvas.r.off( event, cb );
				},
				setAttribute( name, val ) {
					console.log( "set attribute:", name, JSON.stringify( val ) );
				},
				getAttribute( name ) {
					console.log( "get attribute?", name );
				},
				getContext(mode) {
					if( mode === "2d" ) {
						// used as a memory for building atlas textures...
					}
					console.log( "Return context", mode );
					return this.r.getContext( mode );
				},
				getBoundingClientRect() {
					return { x:0, y:0, width: canvas.r.width, height: canvas.r.height };
				}
			}
			r = canvas.r;
			for( let k of pendingKey ) {
				r.on( k.event, k.cb );
			}
			pendingKey.length = 0;
			return canvas;
		} else {
			return new Element();
		}
	}
};


class Element {
	style = {};
	rel = null;
	appendChild(el) {
	}
	insertBefore( node, before ) {
		
	}
	childNodes() {
		return [];
	}
}

document.head = document.createElement( "HEAD" );
document.body = document.createElement( "BODY" );


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
		//console.trace( "get innerWidth:", r );
		return (r && r.width) || 1024;
	},
	get innerHeight() {
		return (r && r.height) || 768;
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
			if( !r ) pendingKey.push( {event, cb} );
			else r.on( "keyup", cb );
			//window.events.keyup.push( (event)=>{ event.preventDefault = ()=>{event.used = true}; return cb(event); } );
			break;
		case "keydown":
			if( !r ) pendingKey.push( {event, cb} );
			else r.on( "keydown", cb );
			//window.events.keydown.push( (event)=>{ event.preventDefault = ()=>{event.used = true}; return cb(event); } );
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
globalThis.location = {
	port: 7999,
	host: "localhost:7999",
	hostname: "localhost",
	protocol: "http",
}
globalThis.window = window;

globalThis.requestAnimationFrame = window.requestAnimationFrame;

if (!globalThis.HTMLImageElement)   globalThis.HTMLImageElement = class {};
if (!globalThis.HTMLCanvasElement)  globalThis.HTMLCanvasElement = class {};
if (!globalThis.ImageBitmap)        globalThis.ImageBitmap = class {};
if (!globalThis.OffscreenCanvas)    globalThis.OffscreenCanvas = class {};

if (!globalThis.document) globalThis.document = {
  body: {},
};