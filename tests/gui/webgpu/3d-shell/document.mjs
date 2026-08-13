
import {sack} from "@d3x0r/sack-gui"



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
	createElement,
  body: createElement( "BODY" ),
};

function	createElement( type, opts ) {
		console.log( "create a:", type );
		
		if( type === "canvas" ) {
			return new threeSackRenderer( opts );
		} else {
			return {
				tag: type,
				appendChild( c ) {
				}
			}
		}
	}


export  class threeSackRenderer extends sack.Renderer {
	r = this;
	#configured = null;
	#ctx = null;
	#cfg = {
		width:0,
		height:0,
	}
        #device = null;
	constructor( opts ) {
		opts = opts || {};
		console.log( "This window should be sized 800 600, and WS_EX_NOREDIRECTIONBITMAP", sack.Renderer.attributes.DISPLAY_ATTRIBUTE_NO_REDIRECT.toString(16) );
		const displayMonitor = sack.Task.getDisplays().device.find( d=>d.primary );
		console.log( displayMonitor );
		super( 'hello', displayMonitor.width/2 - (opts.width||800)/2
				, displayMonitor.height/2 - (opts.height||600)/2, (opts.width||800), (opts.height||600), null
				, sack.Renderer.attributes.DISPLAY_ATTRIBUTE_NO_REDIRECT );
		window.setRenderer( this );

	}
	requestPointerLock() {
		this.lockMouse();
		console.log( "lock pointer..." );
	}

	addEventListener( event, cb ) {
		switch( event ) {
		case "mouseup":
			this.on( "mouseUp", cb );
			break;
		case "mousedown":
			this.on( "mouseDown", cb );
			break;
		case "mousemove":
			this.on( "mouseMove", cb );
			break;
		case "keyup":
			this.on( "keyUp", cb );
			break;
		case "keydown":
			this.on( "keyDown", cb );
			break;
		case "wheel":
			this.on( "wheel", cb );
			break;
		}
	}
	removeEventListener( event, cb ) {
		switch( event ) {
		case "mouseup": this.off( "mouseUp", cb ); break;
		}
	}
//	get width() { return this.r.width }
//	set width(val) { r.width = val }
//	get height() { return this.r.height }
//	set height(val) { r.height = val }
	configure() {
				this.#ctx.configure({
				  device : this.#device,
				  format: 'bgra8unorm',           // platform-fixed on Windows
				  alphaMode: 'premultiplied',
				})
				//this.setSurfaceDepthFormat('depth24plus') ;
	}
	get configured() {


		// this is just rendered 3d mode out - can use vulkan driver here.
		return this.#configured || 
			sack.gpu.requestAdapter({backendType:"vulkan"}).then( (adapter)=>{
				console.log( "Got back adapter:", adapter.info );
				return adapter.requestDevice({
				    //     requiredFeatures: [ 'shared-texture-memory-d3d12-resource' ],
				    dawnToggles: { enabled: [ /*'allow_unsafe_apis' */   // this is for d3d12-resource enable
                                             /*, 'dump_tint_ir'*/ ]
                                             //, disabled: [ 'enable_tint_ir_validation_asserts' ] 
                                             }
					       } ).then( (device)=>{
                                       this.#device = device;
//				console.log( "Did get back a device?" );

//let n = 0; const orig = this.#device.createShaderModule.bind( device );
//device.createShaderModule = ( desc ) => { sack.Volume().write( `shader-${n++}.wgsl`, desc.code ?? "" ); return orig( desc ); };

					// Hook the canvas surface to that device. This is what informs our
					// imglib-webgpu driver of surface size/format/root Image.
					this.#ctx = r.getContext('webgpu');
					this.configure();
					return this.#configured = Promise.resolve( {ctx:this.#ctx,device} );

			} );
		} );
	}
	style = {};
	// getContext is already handled by the native object
}



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
		return r?.width || 1024;
	},
	get innerHeight() {
		return r?.height || 768;
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
