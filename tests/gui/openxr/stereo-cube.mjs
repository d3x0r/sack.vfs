// Stereo three.js in the headset: a real scene, one render pass per eye,
// straight into the runtime's swapchains.
//
// This is first-light.mjs plus an actual renderer. Everything specific to XR
// is in this file — three.js contributes nothing, by necessity: r184's
// XRManager.js throws outright on a WebGPU backend and is built entirely on
// XRWebGLBinding/XRWebGLLayer, so `renderer.xr` and `navigator.xr` are dead
// ends here. What's left is small: build each eye's projection from the FOV
// angles the runtime reports, drive the frame loop from xrWaitFrame, and point
// a plain RenderTarget at the eye texture.
//
//   set XR_RUNTIME_JSON=c:\games2\steam\steamapps\common\SteamVR\steamxr_win64.json
//   node stereo-cube.mjs [frames]
//
// (unquoted — see probe.mjs. Needs the headset on and the runtime running.)
//
// What you should see: a grid floor with three cubes at 1m, 2m and 4m, and a
// red post at arm's length on the left. Judge it by depth and by head motion,
// not by colour — the eyes render the same scene, which is the whole point.

import { sack } from "@d3x0r/sack-gui";

// three's Animation grabs `self` in the Renderer constructor and calls
// requestAnimationFrame on it. In XR the runtime owns pacing, so hand it a
// context we drive by hand: each XR frame we invoke the one pending callback,
// which is what advances nodeFrame (frameId, time uniforms, animated nodes).
// Nothing here schedules anything — pump() below is the only clock.
const anim = {
	pending: null,
	requestAnimationFrame( cb ) { anim.pending = cb; return 1; },
	cancelAnimationFrame() { anim.pending = null; },
};
globalThis.self = anim;

function pump( timeMs ) {
	const cb = anim.pending;
	anim.pending = null;
	if( cb ) cb( timeMs );
}

import * as THREE from "three/webgpu";

const FRAMES = Number( process.argv[ 2 ] ) || 1800;

// ---- device -----------------------------------------------------------
// Same requirements as first-light: the D3D12 backend, plus the feature and
// the toggle that un-gates importing our D3D12 intermediate.

const adapter = await sack.gpu.requestAdapter( { powerPreference: 'high-performance' } );
if( !adapter ) { console.error( "no adapter" ); process.exit( 1 ); }
if( adapter.info?.backendType !== 4 ) {
	console.error( `need the D3D12 backend, got backendType ${adapter.info?.backendType}` );
	process.exit( 1 );
}
const device = await adapter.requestDevice( {
	requiredFeatures: [ 'shared-texture-memory-d3d12-resource' ],
	dawnToggles: { enabled: [ 'allow_unsafe_apis' ] }
} );

const session = sack.xr.beginSession( device );
console.log( `eyes      : ${session.eyes.map( e => `${e.width}x${e.height} (${e.images} images)` ).join( ", " )}` );
console.log( `format    : DXGI ${session.dxgiFormat}` );

// The swapchain format the runtime picked, as WebGPU spells it. SteamVR
// offers only the _SRGB variants, which matters below: with an -srgb format
// the hardware applies the transfer function on write, so three must hand it
// linear values rather than encoding them itself.
const DXGI_TO_WGPU = {
	87: 'bgra8unorm',  91: 'bgra8unorm-srgb',
	28: 'rgba8unorm',  29: 'rgba8unorm-srgb',
};
const eyeFormat = DXGI_TO_WGPU[ session.dxgiFormat ];
if( !eyeFormat ) {
	console.error( `no WebGPU format for DXGI ${session.dxgiFormat}` );
	process.exit( 1 );
}
const eyeIsSrgb = eyeFormat.endsWith( '-srgb' );

// ---- renderer ---------------------------------------------------------
// No window and no canvas: nothing is presented to the desktop, so the only
// thing three needs a "canvas" for is a size it never uses. Everything goes
// through setRenderTarget, which never touches the backend's context getter.

const renderer = new THREE.WebGPURenderer( {
	canvas: { width: session.eyes[ 0 ].width, height: session.eyes[ 0 ].height, style: {} },
	device,
	antialias: false,      // MSAA would need a resolve into the eye texture
} );
await renderer.init();
renderer.autoClear = true;

// ---- scene ------------------------------------------------------------

// Laid out for the LOCAL reference space beginSession asks for: the origin is
// roughly where the headset was when the session started, so y=0 is eye level
// and the floor is *below* it. Content also wraps all the way around, because
// nothing here controls where the headset happens to be pointing — a scene
// only in front of -Z shows an empty grid two thirds of the time.

const scene = new THREE.Scene();

const EYE_HEIGHT = 1.5;                        // assumed, only used to place the grid
const grid = new THREE.GridHelper( 10, 10, 0x888888, 0x444444 );
grid.position.y = -EYE_HEIGHT;
scene.add( grid );

// A ring of posts at eye level: whichever way you turn, some are in view, and
// their spacing is what makes head rotation read as rotation rather than drift.
for( let i = 0; i < 12; i++ ) {
	const a = ( i / 12 ) * Math.PI * 2;
	const post = new THREE.Mesh(
		new THREE.CylinderGeometry( 0.04, 0.04, 1.0 ),
		new THREE.MeshStandardMaterial( { color: i % 3 ? 0x777788 : 0xcc3333 } ) );
	post.position.set( Math.sin( a ) * 2.5, -0.5, Math.cos( a ) * 2.5 );
	scene.add( post );
}

// Three cubes straight ahead at 1m, 2m and 4m. Depth ordering between these is
// the stereo check: at 1m the disparity is obvious, at 4m nearly nil.
const cubes = [];
for( const [ z, colour ] of [ [ -1, 0x33cc66 ], [ -2, 0x3366cc ], [ -4, 0xcc9933 ] ] ) {
	const cube = new THREE.Mesh(
		new THREE.BoxGeometry( 0.3, 0.3, 0.3 ),
		new THREE.MeshStandardMaterial( { color: colour } ) );
	// -Z is forward, same convention as three's camera.
	cube.position.set( 0, 0, z );
	scene.add( cube );
	cubes.push( cube );
}

// A close object off to one side. It shifts most between the eyes, so it's the
// quickest way to catch swapped or identical views.
const near = new THREE.Mesh(
	new THREE.BoxGeometry( 0.08, 0.4, 0.08 ),
	new THREE.MeshStandardMaterial( { color: 0xdd4444 } ) );
near.position.set( -0.35, -0.1, -0.6 );
scene.add( near );

scene.add( new THREE.AmbientLight( 0xffffff, 0.6 ) );
const light = new THREE.PointLight( 0xffffff, 12, 30 );
light.position.set( 1, 1.5, 1 );
scene.add( light );

// ---- per-eye cameras --------------------------------------------------
//
// The runtime's FOV is four half-angles, and they are asymmetric — the outer
// edge of each eye sees further than the inner one. A single `fov` number
// can't express that, so the projection is built from the angles directly and
// updateProjectionMatrix is replaced rather than fed an aspect ratio.
//
// Overriding the method (instead of assigning projectionMatrix once) matters:
// Renderer.render calls updateProjectionMatrix whenever it notices the
// coordinate system or reversed-depth flag changed, and would otherwise
// overwrite the matrix with a symmetric one on the first frame.

const NEAR = 0.05, FAR = 200;

function makeEyeCamera() {
	const cam = new THREE.PerspectiveCamera( 90, 1, NEAR, FAR );
	cam.coordinateSystem = THREE.WebGPUCoordinateSystem;
	cam.viewFov = null;
	cam.updateProjectionMatrix = function () {
		const v = this.viewFov;
		if( !v ) return;                       // still in the base constructor
		const n = this.near;
		this.projectionMatrix.makePerspective(
			n * Math.tan( v.left ), n * Math.tan( v.right ),
			n * Math.tan( v.up ),   n * Math.tan( v.down ),
			n, this.far, this.coordinateSystem, this.reversedDepth );
		this.projectionMatrixInverse.copy( this.projectionMatrix ).invert();
	};
	return cam;
}

const cameras = session.eyes.map( makeEyeCamera );

// ---- per-eye render targets -------------------------------------------
//
// acquire() hands back the same GPUTexture every frame — the D3D12
// intermediate created once in beginSession, not a rotating swapchain image
// (the runtime's own images can't be imported by Dawn; see xr_session.cc). So
// the render target is built once, on the first frame that has a texture, and
// reused.
//
// The one piece three can't work out for itself is the texture's format:
// WebGPUTextureUtils.createTexture takes an early exit for external textures
// that sets the GPU handle but leaves backend data's `format` undefined, and
// pipeline creation reads it. Seeding just that one field is enough; three
// still runs its normal createTexture path and allocates the depth buffer.

const targets = [];

function eyeTarget( eye, gpuTexture ) {
	if( targets[ eye ] ) return targets[ eye ];

	const { width, height } = session.eyes[ eye ];
	const rt = new THREE.RenderTarget( width, height, { depthBuffer: true, samples: 0 } );

	const ext = new THREE.ExternalTexture( gpuTexture );
	ext.image = { width, height, depth: 1 };
	ext.isRenderTargetTexture = true;
	ext.renderTarget = rt;
	ext.flipY = false;
	ext.generateMipmaps = false;
	// With an -srgb format the hardware encodes on write, so ask three for
	// linear output; with a plain unorm format three has to encode itself.
	ext.colorSpace = eyeIsSrgb ? THREE.LinearSRGBColorSpace : THREE.SRGBColorSpace;
	rt.texture = ext;

	renderer.backend.get( ext ).format = eyeFormat;

	targets[ eye ] = rt;
	return rt;
}

// ---- frame loop -------------------------------------------------------
//
// Paced by xrWaitFrame, which blocks until the runtime is ready for the next
// frame. Scheduled through setImmediate rather than run in a tight while():
// Dawn's callbacks are pumped from a uv_check_t, and a loop that never
// returns to libuv never lets them run.

let frames = 0, rendered = 0, lastState = "", started = performance.now();

function tick() {
	if( frames >= FRAMES ) return finish();

	const st = session.poll();
	if( st.state !== lastState ) {
		console.log( `state     : ${st.state}` );
		lastState = st.state;
		if( st.state === "EXITING" || st.state === "LOSS_PENDING" ) return finish();
	}
	if( !st.running ) return setImmediate( tick );   // not READY yet

	const frame = session.waitFrame();
	if( !frame ) return setImmediate( tick );
	frames++;

	// Advance three's internal frame (nodeFrame/time uniforms) exactly once
	// per XR frame, before anything is rendered with it.
	pump( performance.now() );

	const t = ( performance.now() - started ) / 1000;
	for( const cube of cubes ) {
		cube.rotation.x = t * 0.7;
		cube.rotation.y = t * 0.9;
	}

	if( frame.shouldRender ) {
		for( let eye = 0; eye < session.eyes.length && eye < frame.views.length; eye++ ) {
			const view = frame.views[ eye ];
			const cam = cameras[ eye ];

			cam.viewFov = view.fov;
			cam.updateProjectionMatrix();
			cam.position.set( view.position.x, view.position.y, view.position.z );
			cam.quaternion.set( view.orientation.x, view.orientation.y,
			                    view.orientation.z, view.orientation.w );

			const tex = session.acquire( eye );
			renderer.setRenderTarget( eyeTarget( eye, tex ) );
			renderer.render( scene, cam );
			// release() ends Dawn's access to the texture (which flushes the
			// work above) and copies it into the runtime's image.
			session.release( eye );

			// One-shot: the first eye ever drawn. Zero draw calls here means
			// three built no work at all (scene/camera/culling), which looks
			// identical in the headset to a compositing failure.
			if( !rendered && !eye )
				console.log( `first eye : ${renderer.info.render.drawCalls} draws,`
					+ ` ${renderer.info.render.triangles} triangles,`
					+ ` ${session.eyes[ eye ].width}x${session.eyes[ eye ].height} ${eyeFormat}` );
		}
		rendered++;
	}

	// endFrame must be called for every frame that began, rendered or not.
	session.endFrame();

	if( frames % 180 === 0 ) {
		const v = frame.views?.[ 0 ];
		console.log( `frame ${frames}: rendered=${rendered} posesValid=${frame.posesValid}`
			+ ` draws=${renderer.info.render.drawCalls}`
			+ ( v ? ` head=(${v.position.x.toFixed( 2 )}, ${v.position.y.toFixed( 2 )}, ${v.position.z.toFixed( 2 )})` : "" ) );
	}

	setImmediate( tick );
}

function finish() {
	const secs = ( performance.now() - started ) / 1000;
	console.log( `done      : ${frames} frames, ${rendered} rendered, ${( frames / secs ).toFixed( 1 )} fps` );
	session.destroy();
	process.exit( 0 );
}

tick();
