// OpenXR for sack-gui — session setup, controller input, and the frame loop.
//
//     import { beginSession, hands, runFrames, LEFT, RIGHT } from "@d3x0r/sack-gui/xr";
//
// Deliberately free of any three.js reference so this module matches the
// package's empty dependency list. The renderer-specific glue — eye render
// targets, per-eye cameras, grab maths — lives in "@d3x0r/sack-gui/xr-three".
//
// Two constraints shape this API, both from OpenXR rather than from taste:
//
//   * The frame loop belongs to the runtime. xrWaitFrame blocks until the
//     compositor wants the next frame, so nothing here schedules on a timer,
//     and requestAnimationFrame is not the clock.
//   * Actions cannot be declared after the loop starts. xrAttachSessionActionSets
//     is once-per-session and must precede the first xrSyncActions, so the
//     native side builds one fixed action set covering the union of controllers.
//     What you get here is a view over it, not a way to define your own.

import { sack } from "@d3x0r/sack-gui";

export const LEFT = 0, RIGHT = 1;

// Offsets within each hand's block of session.input / session.buttons.
//
// This is a struct contract with the C++ side: it mirrors the enum in
// src/gui/openxr/xr_session.cc and the two must move together. Both typed
// arrays are views over one native buffer, so reads are free and there is
// nothing to copy or invalidate.
export const SLOT = {
	gripPos: 0, gripOri: 3,     // vec3, quat — the pose *inside* the controller,
	aimPos:  7, aimOri: 10,     // vec3, quat — where the user is pointing
	trigger: 14, squeeze: 15,
	axisX: 16, axisY: 17,       // stick, or trackpad on hardware with no stick
	axis2X: 18, axis2Y: 19,     // trackpad where a stick already took the primary
	flags: 24, profile: 25,     // read these through session.buttons
};

export const FLAG = {
	active:  1 << 0,            // pose valid *and* currently tracked
	axisClick: 1 << 1,
	button0: 1 << 2, button1: 1 << 3, button2: 1 << 4, button3: 1 << 5,
	menu:    1 << 6, axis2Click: 1 << 7,
};

// The runtime picks the swapchain format; these are the ones the native binding
// knows how to wrap as a WGPUTexture. SteamVR offers only the _SRGB variants.
const DXGI_TO_WGPU = {
	87: 'bgra8unorm', 91: 'bgra8unorm-srgb',
	28: 'rgba8unorm', 29: 'rgba8unorm-srgb',
};

/**
 * Open an XR session on a GPU device suitable for it.
 *
 * The device requirements are not negotiable and not discoverable: the D3D12
 * backend because that is the only Dawn backend with Win32 external memory,
 * plus a gated feature *and* the toggle that ungates it. Without the toggle
 * Dawn reports the feature as "guarded by toggle", which reads like the
 * backend simply lacks it.
 */
export async function beginSession( { powerPreference = 'high-performance', device = null } = {} ) {
	let adapter = null;
	if( !device ) {
		adapter = await sack.gpu.requestAdapter( { powerPreference } );
		if( !adapter )
			throw new Error( "xr: no GPU adapter available" );
		if( adapter.info?.backendType !== 4 )
			throw new Error( `xr: needs the D3D12 backend, got backendType ${adapter.info?.backendType}`
				+ " — OpenXR interop here is D3D12-only (Vulkan on Windows has no external memory in Dawn)" );
		device = await adapter.requestDevice( {
			requiredFeatures: [ 'shared-texture-memory-d3d12-resource' ],
			dawnToggles: { enabled: [ 'allow_unsafe_apis' ] },
		} );
	}

	const session = sack.xr.beginSession( device );

	const format = DXGI_TO_WGPU[ session.dxgiFormat ];
	if( !format ) {
		session.destroy();
		throw new Error( `xr: runtime chose DXGI format ${session.dxgiFormat}, which this binding does not map` );
	}

	return {
		session, device, adapter, format,
		// With an -srgb format the hardware applies the transfer function on
		// write, so the renderer must be told to hand over linear values or the
		// image is encoded twice.
		formatIsSrgb: format.endsWith( '-srgb' ),
		hands: hands( session ),
	};
}

/**
 * A non-allocating view over one hand's slice of the input buffer.
 *
 * Every accessor reads the live native buffer, so a Hand is built once and
 * stays correct. Pose readers write into objects you supply rather than
 * returning new ones — at 90Hz, two hands' worth of fresh vectors and
 * quaternions is over a thousand allocations a second for data that is
 * overwritten every frame anyway.
 */
export class Hand {
	constructor( session, index ) {
		this.session = session;
		this.index = index;
		this.f = session.input;
		this.u = session.buttons;
		this.base = index * session.handStride;
	}

	get flags()  { return this.u[ this.base + SLOT.flags ]; }
	get active() { return ( this.flags & FLAG.active ) !== 0; }

	get trigger() { return this.f[ this.base + SLOT.trigger ]; }
	get squeeze() { return this.f[ this.base + SLOT.squeeze ]; }
	get axisX()   { return this.f[ this.base + SLOT.axisX ]; }
	get axisY()   { return this.f[ this.base + SLOT.axisY ]; }
	get axis2X()  { return this.f[ this.base + SLOT.axis2X ]; }
	get axis2Y()  { return this.f[ this.base + SLOT.axis2Y ]; }

	get axisClick()  { return ( this.flags & FLAG.axisClick ) !== 0; }
	get axis2Click() { return ( this.flags & FLAG.axis2Click ) !== 0; }
	get menu()       { return ( this.flags & FLAG.menu ) !== 0; }
	button( i )      { return ( this.flags & ( FLAG.button0 << i ) ) !== 0; }

	// Whichever analog the hardware offers for "close your hand". A Vive wand's
	// squeeze is a button arriving as 0.0/1.0; a Touch's is continuous. Callers
	// wanting a grab gesture should not have to care which.
	get grab() { return Math.max( this.trigger, this.squeeze ); }

	// Which interaction profile the runtime actually bound, as a path string.
	// Changes mid-session when controllers sleep or are swapped.
	get profile() {
		return this.session.profiles[ this.u[ this.base + SLOT.profile ] ] ?? "unknown";
	}

	/** Writes into `pos` {x,y,z} and `quat` {x,y,z,w}; either may be omitted. */
	readGrip( pos, quat ) { return this.#readPose( SLOT.gripPos, SLOT.gripOri, pos, quat ); }
	readAim ( pos, quat ) { return this.#readPose( SLOT.aimPos,  SLOT.aimOri,  pos, quat ); }

	#readPose( pSlot, oSlot, pos, quat ) {
		const f = this.f, b = this.base;
		if( pos ) {
			pos.x = f[ b + pSlot ]; pos.y = f[ b + pSlot + 1 ]; pos.z = f[ b + pSlot + 2 ];
		}
		if( quat ) {
			quat.x = f[ b + oSlot ];     quat.y = f[ b + oSlot + 1 ];
			quat.z = f[ b + oSlot + 2 ]; quat.w = f[ b + oSlot + 3 ];
		}
		return this.active;
	}

	pulse( amplitude = 0.5, ms = 40 ) { this.session.pulse( this.index, amplitude, ms ); }
}

export function hands( session ) {
	const out = [];
	for( let i = 0; i < session.hands; i++ ) out.push( new Hand( session, i ) );
	return out;
}

/**
 * Drive the session from the runtime's own pacing. Resolves when the frame
 * budget runs out or the runtime asks the session to end.
 *
 * Hooks, in the order they are called each frame:
 *   tick( nowMs )              advance renderer-internal clocks
 *   update( dt, frame )        read input, run interactions, set transforms
 *   eye( index, view, texture ) once per eye, between acquire and release
 *   onState( state )           session state transitions
 *
 * Scheduling is via setImmediate rather than a tight while(): Dawn pumps its
 * callbacks from a libuv check handle, and a loop that never yields to libuv
 * never lets them run.
 */
export function runFrames( session, {
	tick = null, update = null, eye = null, onState = null,
	frames: frameBudget = Infinity,
} = {} ) {
	return new Promise( ( resolve ) => {
		let frames = 0, rendered = 0, lastState = "";
		const started = performance.now();
		let lastT = started;

		const finish = () => resolve( {
			frames, rendered,
			seconds: ( performance.now() - started ) / 1000,
			get fps() { return this.frames / this.seconds; },
		} );

		const step = () => {
			if( frames >= frameBudget ) return finish();

			const st = session.poll();
			if( st.state !== lastState ) {
				lastState = st.state;
				if( onState ) onState( st.state );
				if( st.state === "EXITING" || st.state === "LOSS_PENDING" ) return finish();
			}
			// Not READY yet — poll again rather than submitting frames.
			if( !st.running ) return setImmediate( step );

			const frame = session.waitFrame();
			if( !frame ) return setImmediate( step );
			frames++;

			const now = performance.now();
			// Clamped: a hitch or a breakpoint should not launch anything into
			// orbit on the frame after it.
			const dt = Math.min( ( now - lastT ) / 1000, 0.1 );
			lastT = now;

			if( tick ) tick( now );
			if( update ) update( dt, frame );

			// shouldRender is false when the app is not visible (headset off the
			// head, another app in front). Frames still have to be ended.
			if( frame.shouldRender && eye ) {
				const n = Math.min( session.eyes.length, frame.views.length );
				for( let i = 0; i < n; i++ ) {
					const texture = session.acquire( i );
					eye( i, frame.views[ i ], texture );
					session.release( i );
				}
				rendered++;
			}
			session.endFrame();

			setImmediate( step );
		};

		step();
	} );
}
