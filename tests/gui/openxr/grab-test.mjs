// Controller input + grab, built on @d3x0r/sack-gui/xr.
//
// Everything mechanical about XR now lives in apps/xr — session setup, the
// frame loop, input accessors, eye targets, per-eye projections. What's left
// here is the application: a scene, and what grabbing means.
//
//   set XR_RUNTIME_JSON=c:\games2\steam\steamapps\common\SteamVR\steamxr_win64.json
//   node grab-test.mjs [frames]
//
// (stereo-cube.mjs is deliberately NOT refactored — it stays written against
// the raw sack.xr primitives, both as a baseline if this module regresses and
// as documentation of what the module is actually doing.)

import { beginSession, runFrames, LEFT, RIGHT } from "@d3x0r/sack-gui/xr";
import {
	installAnimationContext, eyeRenderer, eyeCameras, syncEyeCamera,
	EyeTargets, gripMatrix, grabDistance, THREE,
} from "@d3x0r/sack-gui/xr-three";

const FRAMES = Number( process.argv[ 2 ] ) || 5400;

// Before the renderer exists — three captures `self` in its constructor.
const pump = installAnimationContext();

const xr = await beginSession();
const { session, hands: controllers } = xr;
console.log( `eyes      : ${session.eyes.map( e => `${e.width}x${e.height}` ).join( ", " )}` );
console.log( `format    : ${xr.format}` );

const renderer = await eyeRenderer( xr );
const targets  = new EyeTargets( renderer, session, xr );
const cameras  = eyeCameras( session, { near: 0.05, far: 200 } );

// ---- scene -----------------------------------------------------------
// LOCAL reference space: y=0 is roughly eye level, not the floor.

const scene = new THREE.Scene();
const grid = new THREE.GridHelper( 10, 10, 0x888888, 0x444444 );
grid.position.y = -1.5;
scene.add( grid );
scene.add( new THREE.AmbientLight( 0xffffff, 0.6 ) );
const light = new THREE.PointLight( 0xffffff, 12, 30 );
light.position.set( 1, 1.5, 1 );
scene.add( light );

const cubes = [];
for( const [ x, y, z, colour ] of [
	[  0.00, -0.10, -0.45, 0x33cc66 ],
	[ -0.30, -0.05, -0.55, 0x3366cc ],
	[  0.30, -0.15, -0.50, 0xcc9933 ],
	[  0.00,  0.15, -0.60, 0xcc44aa ],
] ) {
	const cube = new THREE.Mesh(
		new THREE.BoxGeometry( 0.12, 0.12, 0.12 ),
		new THREE.MeshStandardMaterial( { color: colour } ) );
	cube.position.set( x, y, z );
	scene.add( cube );
	cubes.push( cube );
}

// The proxy's dimensions are also the grab hitbox, which is the point of naming
// them once: the shape you see is the shape that grabs, so aiming with the tip
// does what aiming with the tip looks like it should.
const HAND_LEN = 0.12, HAND_THICK = 0.03;
const HAND_SHAPE = { length: HAND_LEN, radius: HAND_THICK / 2, samples: 9 };

const proxies = controllers.map( ( h, i ) => {
	const m = new THREE.Mesh(
		new THREE.BoxGeometry( HAND_THICK, HAND_THICK, HAND_LEN ),
		new THREE.MeshStandardMaterial( { color: i ? 0x66ccff : 0xffcc66 } ) );
	m.visible = false;
	scene.add( m );
	return m;
} );

// ---- grab ------------------------------------------------------------

const GRAB_SLOP = 0.02;                 // metres of gap still allowed to grab
const GRAB_ON = 0.6, GRAB_OFF = 0.35;   // hysteresis against a shaky trigger

const held = controllers.map( () => null );
const handM = controllers.map( () => new THREE.Matrix4() );
const localM = controllers.map( () => new THREE.Matrix4() );
const _inv = new THREE.Matrix4(), _s = new THREE.Vector3();

let frames = 0;
const lastProfile = controllers.map( () => "none" );

function update( dt ) {
	frames++;

	// Report the bound interaction profile on *change*, not once at first
	// activity. The profile arrives with an INTERACTION_PROFILE_CHANGED event,
	// which typically lands a frame or two after a hand first reports active --
	// so a one-shot report fires too early, prints "none", and latches that
	// forever while the controller is visibly working. Profiles also change
	// legitimately later, when a controller sleeps, wakes or is swapped, and
	// those transitions are worth seeing.
	//
	// Starting from "none" means the initial idle state prints nothing.
	for( const h of controllers ) {
		const p = h.active ? h.profile : "none";
		if( p === lastProfile[ h.index ] ) continue;
		lastProfile[ h.index ] = p;
		console.log( `hand ${h.index}    : active=${h.active} profile=${p}` );
	}

	for( let h = 0; h < controllers.length; h++ ) {
		const hand = controllers[ h ];
		if( !hand.active ) { proxies[ h ].visible = false; held[ h ] = null; continue; }

		gripMatrix( hand, handM[ h ] );
		proxies[ h ].visible = true;
		handM[ h ].decompose( proxies[ h ].position, proxies[ h ].quaternion, _s );

		const grip = hand.grab;   // whichever analog this controller offers

		if( !held[ h ] && grip > GRAB_ON ) {
			let best = null, bestD = GRAB_SLOP;
			for( const cube of cubes ) {
				if( held[ 1 - h ] === cube ) continue;      // no tug of war
				cube.updateMatrixWorld();
				const d = grabDistance( cube, handM[ h ], HAND_SHAPE );
				if( d < bestD ) { bestD = d; best = cube; }
			}
			if( best ) {
				held[ h ] = best;
				// Convert the held transform into controller space once, then
				// replay it: tracking position alone would snap the object's
				// rotation to the hand and lose however you were holding it.
				_inv.copy( handM[ h ] ).invert();
				localM[ h ].multiplyMatrices( _inv, best.matrixWorld );
				hand.pulse( 0.6, 25 );
			}
		} else if( held[ h ] && grip < GRAB_OFF ) {
			held[ h ] = null;
			hand.pulse( 0.25, 12 );
		}

		if( held[ h ] ) {
			const cube = held[ h ];
			cube.matrix.multiplyMatrices( handM[ h ], localM[ h ] );
			cube.matrix.decompose( cube.position, cube.quaternion, cube.scale );
		}
	}

	if( frames % 180 === 0 ) {
		const L = controllers[ LEFT ], R = controllers[ RIGHT ];
		console.log( `frame ${frames}:`
			+ ` L[a=${L.active ? 1 : 0} grab=${L.grab.toFixed( 2 )}`
			+ ` ax=${L.axisX.toFixed( 2 )},${L.axisY.toFixed( 2 )}`
			+ ` btn=${[ 0, 1, 2, 3 ].map( i => L.button( i ) ? 1 : 0 ).join( "" )}]`
			+ ` R[a=${R.active ? 1 : 0} grab=${R.grab.toFixed( 2 )}]`
			+ ` held=${held.filter( Boolean ).length}` );
	}
}

// ---- run -------------------------------------------------------------

const stats = await runFrames( session, {
	frames: FRAMES,
	tick: pump,
	update,
	onState( state ) { console.log( `state     : ${state}` ); },
	eye( i, view, texture ) {
		renderer.setRenderTarget( targets.get( i, texture ) );
		renderer.render( scene, syncEyeCamera( cameras[ i ], view ) );
	},
} );

console.log( `done      : ${stats.frames} frames, ${stats.rendered} rendered,`
	+ ` ${stats.fps.toFixed( 1 )} fps` );
session.destroy();
process.exit( 0 );
