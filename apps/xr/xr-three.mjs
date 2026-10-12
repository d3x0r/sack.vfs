// three.js glue for the OpenXR session in "@d3x0r/sack-gui/xr".
//
//     import { installAnimationContext, eyeCameras, EyeTargets } from "@d3x0r/sack-gui/xr-three";
//
// three is a peer dependency, optional and only needed by this file — the core
// xr.mjs has no renderer opinion at all.
//
// None of this uses renderer.xr, and it can't: three r184's XRManager throws
// outright on a WebGPU backend and is built entirely on XRWebGLBinding and
// XRWebGLLayer. Shimming navigator.xr would not help either, because nothing
// would consume it. Stereo is therefore driven by hand — which is much less
// work than it sounds, and is most of what this file is.

import * as THREE from "three/webgpu";

// This glue depends on three *internals*, not just its public API, so a minor
// bump can break it silently — the failure mode is a black eye texture or a
// symmetric projection, neither of which throws. Revisions below have been
// checked by hand; anything else gets a warning naming what to re-verify.
//
// Verified: r184, r185.
const XR_THREE_VERIFIED = [ 184, 185 ];
if( !XR_THREE_VERIFIED.includes( Number( THREE.REVISION ) ) ) {
	console.warn( `@d3x0r/sack-gui/xr-three: three r${THREE.REVISION} has not been verified`
		+ ` (checked: r${XR_THREE_VERIFIED.join( ", r" )}). Three things to re-check if XR output`
		+ ` looks wrong:\n`
		+ `  1. WebGPUTextureUtils.createTexture's isExternalTexture branch still returns early\n`
		+ `     WITHOUT setting textureData.format — EyeTargets seeds that field by hand.\n`
		+ `  2. Animation still captures \`self\` in the Renderer *constructor* —\n`
		+ `     installAnimationContext() must run before the renderer is built.\n`
		+ `  3. Matrix4.makePerspective still takes ( l, r, t, b, near, far, coordinateSystem,\n`
		+ `     reversedDepth ) — eyeCameras builds asymmetric FOV frusta with it.` );
}

/**
 * Give three a frame clock that the XR runtime drives.
 *
 * MUST be called before constructing the renderer. three's Animation captures
 * `self` in the *Renderer constructor*, not on init, and calls
 * requestAnimationFrame on it — so in Node it either throws on a null context
 * or, worse, quietly runs on a setTimeout that has nothing to do with the
 * compositor's pacing.
 *
 * Returns the pump: call it once per XR frame (as runFrames' `tick` hook) and
 * three's nodeFrame, time uniforms and animated nodes advance exactly in step
 * with the headset instead of on a timer.
 */
export function installAnimationContext() {
	const ctx = {
		pending: null,
		requestAnimationFrame( cb ) { ctx.pending = cb; return 1; },
		cancelAnimationFrame() { ctx.pending = null; },
	};
	globalThis.self = ctx;
	return function pump( nowMs ) {
		const cb = ctx.pending;
		ctx.pending = null;
		if( cb ) cb( nowMs );
	};
}

/**
 * One camera per eye, with projections built from the runtime's FOV.
 *
 * The runtime reports four half-angles and they are asymmetric — the outer edge
 * of each eye sees further than the inner one — so a single `fov` number cannot
 * express them and PerspectiveCamera's own projection is always wrong here.
 *
 * updateProjectionMatrix is *replaced* rather than the matrix being assigned
 * once: Renderer.render calls it whenever it notices the coordinate system or
 * reversed-depth flag changed, and would otherwise overwrite a correct
 * asymmetric frustum with a symmetric one on the first frame.
 */
export function eyeCameras( session, { near = 0.05, far = 200 } = {} ) {
	return session.eyes.map( () => {
		const cam = new THREE.PerspectiveCamera( 90, 1, near, far );
		cam.coordinateSystem = THREE.WebGPUCoordinateSystem;
		cam.viewFov = null;
		cam.updateProjectionMatrix = function () {
			const v = this.viewFov;
			if( !v ) return;                  // still inside the base constructor
			const n = this.near;
			this.projectionMatrix.makePerspective(
				n * Math.tan( v.left ), n * Math.tan( v.right ),
				n * Math.tan( v.up ),   n * Math.tan( v.down ),
				n, this.far, this.coordinateSystem, this.reversedDepth );
			this.projectionMatrixInverse.copy( this.projectionMatrix ).invert();
		};
		return cam;
	} );
}

/** Point an eye camera at this frame's view pose and FOV. */
export function syncEyeCamera( cam, view ) {
	cam.viewFov = view.fov;
	cam.updateProjectionMatrix();
	cam.position.set( view.position.x, view.position.y, view.position.z );
	cam.quaternion.set( view.orientation.x, view.orientation.y,
	                    view.orientation.z, view.orientation.w );
	return cam;
}

/**
 * Render targets that draw straight into the runtime's eye textures.
 *
 * session.acquire() returns the same GPUTexture every frame — the D3D12
 * intermediate created once at session start, not a rotating swapchain image —
 * so each target is built once, lazily, on the first frame that has a texture.
 *
 * The one thing three cannot work out for itself is the texture's format:
 * WebGPUTextureUtils.createTexture takes an early exit for external textures
 * which sets the GPU handle but leaves backend data's `format` undefined, and
 * pipeline creation reads it. Seeding that single field is enough — three still
 * runs its normal path and allocates the depth buffer. Note that `initialized`
 * must NOT be pre-seeded: updateRenderTarget sets needsUpdate on the first
 * call, so createTexture runs anyway and throws "Texture already initialized".
 */
export class EyeTargets {
	constructor( renderer, session, { format, formatIsSrgb } ) {
		this.renderer = renderer;
		this.session = session;
		this.format = format;
		this.formatIsSrgb = formatIsSrgb;
		this.targets = [];
	}

	get( eye, gpuTexture ) {
		if( this.targets[ eye ] ) return this.targets[ eye ];

		const { width, height } = this.session.eyes[ eye ];
		const rt = new THREE.RenderTarget( width, height, { depthBuffer: true, samples: 0 } );

		const ext = new THREE.ExternalTexture( gpuTexture );
		ext.image = { width, height, depth: 1 };   // Textures.getSize reads this
		ext.isRenderTargetTexture = true;
		ext.renderTarget = rt;
		ext.flipY = false;
		ext.generateMipmaps = false;
		// An -srgb format means the hardware encodes on write, so three must be
		// asked for linear output or the transfer function is applied twice.
		ext.colorSpace = this.formatIsSrgb ? THREE.LinearSRGBColorSpace : THREE.SRGBColorSpace;
		rt.texture = ext;

		this.renderer.backend.get( ext ).format = this.format;

		this.targets[ eye ] = rt;
		return rt;
	}
}

/**
 * A renderer configured for headless XR output.
 *
 * No window and no canvas: nothing is presented to the desktop, so the only
 * thing three wants a canvas for is a size it never uses. Because every draw
 * goes through setRenderTarget, the backend's lazy `context` getter is never
 * touched and no surface is ever created.
 *
 * antialias is off deliberately — MSAA would need a resolve into the eye
 * texture, which the external-texture path does not set up.
 */
export async function eyeRenderer( { session, device } ) {
	const renderer = new THREE.WebGPURenderer( {
		canvas: { width: session.eyes[ 0 ].width, height: session.eyes[ 0 ].height, style: {} },
		device,
		antialias: false,
	} );
	await renderer.init();
	return renderer;
}

// ---- grab helpers ----------------------------------------------------

const _objInv = new THREE.Matrix4();
const _segA = new THREE.Vector3(), _segB = new THREE.Vector3(), _pt = new THREE.Vector3();
const _p = new THREE.Vector3(), _q = new THREE.Quaternion(), _one = new THREE.Vector3( 1, 1, 1 );

/** Grip pose of `hand` as a matrix, written into `out`. No allocation. */
export function gripMatrix( hand, out ) {
	hand.readGrip( _p, _q );
	return out.compose( _p, _q, _one );
}

/** Aim pose of `hand` as a matrix, written into `out`. No allocation. */
export function aimMatrix( hand, out ) {
	hand.readAim( _p, _q );
	return out.compose( _p, _q, _one );
}

/**
 * Gap in metres between a controller's volume and an object's surface —
 * negative when they overlap.
 *
 * Two things this gets right that a point-to-centre test does not:
 *
 *   * The object contributes its surface, not its centre, so the tolerance
 *     means one thing regardless of how big the object is.
 *   * The controller contributes its whole body. The grip pose is defined
 *     *inside* the controller, where the palm wraps the handle, so measuring
 *     from it alone forces the user to push the controller into an object
 *     before anything registers.
 *
 * A controller proxy is a long thin box, so it is modelled as its centre
 * segment plus a radius. Sampling the segment beats solving box-to-box exactly:
 * at 9 samples over 12cm the spacing is 15mm, well under a typical tolerance,
 * and this runs on grab attempts rather than every frame. The approximation is
 * loosest at the edge-edge case, where it slightly overestimates the gap — so
 * if you tighten the tolerance below ~10mm, raise `samples` with it.
 *
 * Assumes unit scale on the object; a scaled object needs the result scaled.
 */
export function grabDistance( obj, handMatrix, { length = 0.12, radius = 0.015, samples = 9 } = {} ) {
	const geo = obj.geometry;
	if( !geo.boundingBox ) geo.computeBoundingBox();

	// Object space handles the object's own rotation for free.
	_objInv.copy( obj.matrixWorld ).invert();
	_segA.set( 0, 0, -length / 2 ).applyMatrix4( handMatrix ).applyMatrix4( _objInv );
	_segB.set( 0, 0,  length / 2 ).applyMatrix4( handMatrix ).applyMatrix4( _objInv );

	let best = Infinity;
	for( let i = 0; i < samples; i++ ) {
		_pt.lerpVectors( _segA, _segB, i / ( samples - 1 ) );
		// distanceToPoint is 0 inside the box, so overlap reads as touching.
		const d = geo.boundingBox.distanceToPoint( _pt );
		if( d < best ) best = d;
	}
	return best - radius;
}

export { THREE };
