// First light: clear each eye to a flat colour and submit it to the runtime.
//
// Deliberately no three.js. This proves the whole interop chain on its own —
// LUID match, graphics binding, session, swapchains, ID3D12Resource wrapped as
// a WGPUTexture, and xrEndFrame — so that when a real scene misbehaves later you
// already know the plumbing under it is sound.
//
// Left eye blue, right eye green: distinguishable, and swapped eyes are obvious.
//
//   set XR_RUNTIME_JSON=c:\games2\steam\steamapps\common\SteamVR\steamxr_win64.json
//   node first-light.mjs
//
// (unquoted — see probe.mjs. Needs the headset on and the runtime running.)

import { sack } from "@d3x0r/sack-gui";

const FRAMES = Number( process.argv[ 2 ] ) || 600;

const adapter = await sack.gpu.requestAdapter( { powerPreference: 'high-performance' } );
if( !adapter ) { console.error( "no adapter" ); process.exit( 1 ); }
if( adapter.info?.backendType !== 4 ) {
	console.error( `need the D3D12 backend, got backendType ${adapter.info?.backendType}` );
	process.exit( 1 );
}

// The import of runtime-owned D3D12 resources needs both the feature and the
// toggle that un-gates it.
const device = await adapter.requestDevice( {
	requiredFeatures: [ 'shared-texture-memory-d3d12-resource' ],
	dawnToggles: { enabled: [ 'allow_unsafe_apis' ] }
} );

const session = sack.xr.beginSession( device );
console.log( `eyes      : ${session.eyes.map( e => `${e.width}x${e.height} (${e.images} images)` ).join( ", " )}` );
console.log( `format    : DXGI ${session.dxgiFormat}` );

const CLEAR = [
	{ r: 0.05, g: 0.10, b: 0.85, a: 1 },   // left  — blue
	{ r: 0.05, g: 0.75, b: 0.15, a: 1 },   // right — green
];

let frames = 0, rendered = 0, lastState = "";

// The runtime paces this loop through waitFrame, so a plain while() is right —
// there's no rAF here and nothing to schedule against.
while( frames < FRAMES ) {
	const st = session.poll();
	if( st.state !== lastState ) {
		console.log( `state     : ${st.state}` );
		lastState = st.state;
		if( st.state === "EXITING" || st.state === "LOSS_PENDING" ) break;
	}
	if( !st.running ) continue;   // not READY yet; poll again

	const frame = session.waitFrame();
	if( !frame ) continue;
	frames++;

	if( frame.shouldRender ) {
		for( let eye = 0; eye < session.eyes.length; eye++ ) {
			const tex = session.acquire( eye );
			const enc = device.createCommandEncoder();
			const pass = enc.beginRenderPass( {
				colorAttachments: [ {
					view: tex.createView(),
					loadOp: 'clear',
					storeOp: 'store',
					clearValue: CLEAR[ eye % CLEAR.length ],
				} ]
			} );
			pass.end();
			device.queue.submit( [ enc.finish() ] );
			session.release( eye );
		}
		rendered++;
	}

	// endFrame must be called for every frame that began, rendered or not.
	session.endFrame();

	if( frames % 90 === 0 ) {
		const v = frame.views?.[ 0 ];
		console.log( `frame ${frames}: rendered=${rendered} posesValid=${frame.posesValid}`
			+ ( v ? ` head=(${v.position.x.toFixed(2)}, ${v.position.y.toFixed(2)}, ${v.position.z.toFixed(2)})` : "" ) );
	}
}

console.log( `done      : ${frames} frames, ${rendered} rendered` );
session.destroy();
