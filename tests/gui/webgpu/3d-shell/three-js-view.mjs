
import {sack} from "sack-gui"
import {Events} from "sack-gui/Events2"
import {NaturalCamera} from "./NaturalCamera.mjs"
import * as THREE from "three/webgpu"
import {Motion} from "./personalFill.mjs"

import "./document.mjs"

import {lnQuat} from "./lnQuatSq.mjs"

const movers2 = []
let myMover = null;
let animCall = null;

const r = new threeSackRenderer();

r.addEventListener( "wheel", (evt)=>{
//	evt.preventDefault = ()=>{evt.used = true};
//	evt.used = false;
	console.log( "wheel:", evt );
} );

r.addEventListener( "mousedown", (evt)=>{
//	evt.preventDefault = ()=>{evt.used = true};
//	evt.used = false;
	//console.log( "Moused;", evt );
} );
r.addEventListener( "mouseup", (evt)=>{
//	evt.preventDefault = ()=>{evt.used = true};
//	evt.used = false;
	//console.log( "Mouse;", evt );
} );
r.addEventListener( "mousemove", (evt)=>{
//	evt.preventDefault = ()=>{evt.used = true};
//	evt.used = false;
	//console.log( "Mousemove;", evt );
} );

r.addEventListener( "keydown", (event,r)=>{
//	if( !event.
	//console.log( "window keydown", window.events.keydown );
	event._used = false;
	event.preventDefault = ()=>{ event._used = true };
	window.events.keydown.forEach( cb=>{
		if( event._used ) return;
		cb(event) ;

	});
} );
r.addEventListener( "keyup", (event,r)=>{
	//console.log( "window keyup" );
	event._used = false;
	event.preventDefault = ()=>{ event._used = true };
	window.events.keyup.forEach( cb=>{
		if( event._used ) return;

		cb(event)
	});
	
} );

if(0)
r.on( "key", (key_bits)=>{
	console.log( "Key:", key_bits );
} );
r.on( "touch", (x,y,b,z)=>{
	console.log( "Touch:", x, y, b, z );
} );

import {BinaryArray as ExpandableBufferAttribue} from "./BinaryArray.mjs"

const a = await navigator.gpu.requestAdapter({ powerPreference: 'high-performance' });
const device = await a.requestDevice();

console.log("backend:", a.info.backendType);

const ctx = r.getContext('webgpu');

r.show();


var l = 0;

//var words1 = voxelUniverse.createTextCluster( "Hello World" );


	var scene;
	var scene2;
	var camera, renderer;
	let mode = 0;
	var light;
	var geometry, material, mesh = [];
	var frame_target = [];
	var slow_animate = false;
	var frame = 0;

	var tests = [];

var mx = 0;
var my = 0;
var xorg = -0.5 + ( -0.5/16 ) + ( 0.5 / 3200 );
var yorg = 0.5;
var display_scale = 1.0/3200000.0;

const 	moveSpeed = 12 * 0.0254;


var ofsx, ofsy;
var dx, dy;

function Color(r,g,b) { return [r,g,b,255]; }


const BASE_COLOR_WHITE = [255,255,255,255];
const BASE_COLOR_BLACK = [0,0,0,255];
const BASE_COLOR_RED = [127,0,0,255];
const BASE_COLOR_LIGHTBLUE = [0,0,255,255];
const BASE_COLOR_LIGHTRED = [255,0,0,255];
const BASE_COLOR_LIGHTGREEN = [0,255,0,255];
const BASE_COLOR_BLUE = [0,0,127,255];
const BASE_COLOR_GREEN = [0,127,0,255];
const BASE_COLOR_MAGENTA = [127,0,127,255];
const BASE_COLOR_BROWN = [127,92,0,255];


const BASE_COLOR_DARK_BLUE = [0,0,132,255];
const BASE_COLOR_MID_BLUE = [0x2A,0x4F,0xA8,255];
const BASE_COLOR_YELLOW = [255,255,0,255];
const BASE_COLOR_LIGHTCYAN = [0,192,192,255];
const BASE_COLOR_DARK_BLUEGREEN = [0x06, 0x51, 0x42,255];
const BASE_COLOR_DARK_GREEN = [0,93,0,255];
const BASE_COLOR_DARK_BROWN = [0x54,0x33,0x1c,255];  //54331C
const BASE_COLOR_LIGHT_TAN = [0xE2,0xB5,0x71,255];    //E2B571

const BASE_COLOR_ORANGE = [150,128,0,255];



var appContainer = document.getElementById( "display" );


var screen = { width:window.innerWidth, height:window.innerHeight };

	//const totalUnit = Math.PI/(2*60);
	//const unit = totalUnit;
	var delay_counter = 60*3;
	//const pause_counter = delay_counter + 120;
	var single_counter = 60;
	var totalUnit = Math.PI/2;
	var unit = totalUnit / single_counter;
	var pause_counter = 120;

	var counter= 0;

	var clock = new THREE.Timer()

	const keys = { LEFT: 37, UP: 38, RIGHT: 39, BOTTOM: 40
        , A:65, S:83, D:68, W:87, SPACE:32, C:67 
, TAB: 9
, NUM0: 45
, NUM1: 35
, NUM2: 40
, NUM3: 34
, NUM4: 37
, NUM5: 12
, NUM6: 39
, NUM7: 36
, NUM8: 38
, NUM9: 33
, NUMDOT: 46
, ENTER:13
, TILDE:192
, Z:90
, X:88
};

//const Accel1 = 2*Math.PI / 12;
const Accel1 = 2*Math.PI / 24;
const linAccel1 = 10;
let controlForm = null;

export class Viewer extends Events {
	controlNatural = null;
	constructor( tick ) {
		super()
		const context= ctx;
		const renderer = r;

		init(context, renderer, device, this).then( ()=>{
			console.log( '------------' );
			animCall( performance.now() );


		function lockChangeAlert() {
			if (document.pointerLockElement === renderer.domElement ||
				document.mozPointerLockElement === renderer.domElement) {
					//canvas.rotateStart.set( event.clientX, event.clientY );
				//console.log('The pointer lock status is now locked');
				mode = 1;
							
				//controls.userRotate = true;
				//document.addEventListener("mousemove", updatePosition, false);
			} else {
				mode = 0;
				//console.log('The pointer lock status is now unlocked');
				//document.removeEventListener("mousemove", updatePosition, false);
			}
		}
		document.addEventListener('pointerlockchange', lockChangeAlert, false)
		} );

	}
	
	

	static addModelToScene2(object) {
	//	geometry, materials
	//   var material = new THREE.MeshFaceMaterial(materials);
	//   var object = new THREE.Mesh(geometry, material);
		var x;
		var m;

		scene.add( x = object);
		
		const motion =  new Motion(x);

		motion.dipole = new lnQuat( 0, 0, 0, 1 ).update();
		motion.orientation.set( 0, 0, -Math.PI, 0 );
		motion.position.set( 0, 0, 100 );
		const body = { object, motion };
		movers2.push(body);
		x.matrixAutoUpdate = true;
		// attach the camera to one smart object.

		if( !myMover ) {
			myMover = object;
			console.log( "attaching camera to a mover..." );
			myMover.add( camera );
		}

		//body.brain = myBrainBoard.brain;
		//myBrainBoard.setBody( body )
		return motion;
	}

		
	addModelToScene(object) {
	//	geometry, materials
	//   var material = new THREE.MeshFaceMaterial(materials);
	//   var object = new THREE.Mesh(geometry, material);
		//object.scale.set(10, 10, 10);
			scene.add(object);
	}


}


function init(ctx, r, device, viewer ) {
        
        window.setRenderer( r );
        
		animCall = animate.bind( viewer, null )
		scene = new THREE.Scene();
		scene2 = new THREE.Scene();
		console.log( "What we get?", window.innerWidth, window.innerHeight );
		camera = new THREE.PerspectiveCamera( 90, window.innerWidth / window.innerHeight, 0.01, 40000 );

		camera.position.z = -1;
		camera.position.y = 0.75;
		//camera.quaternion
		//camera.matrix.origin.z = 3;
		camera.matrixWorldNeedsUpdate = true;

		camera.matrixAutoUpdate = true;

		 // for phong hello world test....
 		var light = new THREE.PointLight( 0xffFFFF, 1, 10000 );
 		light.position.set( 0, 0, 100 );
 		scene.add( light );


		const controlNatural = viewer.controlNatural = new NaturalCamera( camera, r );
		controlNatural.enable( /* optional extended callback*/ );
		controlNatural.motion.orientation.set( 0, 0, Math.PI, 0 ).update();
		controlNatural.motion.orientation.exp( camera.quaternion );


		//renderer = new THREE.WebGLRenderer();
		renderer = new THREE.WebGPURenderer({alpha:true, canvas: r, device});
		return renderer.init( {device, context:ctx} ).then( ()=>{
			//renderer.setSize( window.innerWidth, window.innerHeight );
                        console.log( "Setting clear color.." );
			renderer.setClearColor( 0x00FFffff, 0.1 );
			//renderer.setClearAlpha( 0 );
		} );

}


function slowanim(animate) {
	// and then the next animation frame.
	requestAnimationFrame( animate );
}


let fz = 0;

function render() {
	//if( fz++ === 0 ) sack.system.dumpMemory(true);
	//else if( fz === 100 ) sack.system.dumpMemory(true);
//	renderer.clear();
console.log( "Render.", camera.matrixWorld );
	renderer.render( scene, camera );

/*	          
	if(++fz % 60 === 0 )	{
		console.log( "camera:", camera.position, camera.quaternion, camera.matrix, camera.projectionMatrix );
		console.log( "segments position:", IRF.lineSegments.position, IRF.lineSegments.matrixWorld);
	}
*/
}

var sumDel =0;


	movers2.forEach( (ent,idx)=>{
		const motion = ent.motion;
		motion.torque.dirty = true;
		motion.torque.update();
		const m = ent.object;
		motion.inertialmove(m.matrix,delta)
		m.updateMatrix()

	});



function animate( cb, tick ) {
    clock.update(tick);
	var delta = clock.getDelta();

	Viewer.on( "update", clock );
        console.log( "tick update controls:", tick, delta );
	this.controlNatural.update( delta )
	render();

	if( slow_animate )
		setTimeout( ()=>slowanim( animCall, 250 ) );
	else
		requestAnimationFrame( animCall );
}



export {THREE}