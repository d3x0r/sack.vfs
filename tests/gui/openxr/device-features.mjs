// Verifies requestDevice honours requiredFeatures, including the Dawn-only
// names the generated IDL mapper doesn't know.
//
// SharedTextureMemoryD3D12Resource is what lets the XR path import runtime-owned
// ID3D12Resource swapchain images as WGPUTextures. It has to be requested at
// device creation — a device built without it looks completely healthy and then
// fails the import much later, so this check is worth having on its own.
//
//   node device-features.mjs

import { sack } from "@d3x0r/sack-gui";

const adapter = await sack.gpu.requestAdapter( { powerPreference: 'high-performance' } );
if( !adapter ) { console.error( "no adapter" ); process.exit( 1 ); }

console.log( `backend   : ${adapter.info?.backendType} (4 = D3D12, 6 = Vulkan)` );

// Ask for the feature the OpenXR interop needs.
let device = null;
try {
	device = await adapter.requestDevice( {
		requiredFeatures: [ 'shared-texture-memory-d3d12-resource' ],
		// The feature is gated: without this Dawn rejects with "guarded by
		// toggle allow_unsafe_apis", which reads like the backend lacks it.
		dawnToggles: { enabled: [ 'allow_unsafe_apis' ] }
	} );
} catch( err ) {
	console.error( "requestDevice threw:", err );
	process.exit( 1 );
}

if( !device ) {
	console.error( "requestDevice returned nothing — the feature was probably rejected." );
	console.error( "A bare requestDevice() succeeding while this fails means the adapter" );
	console.error( "doesn't advertise SharedTextureMemoryD3D12Resource; check the backend" );
	console.error( "is really D3D12 above, since the Vulkan backend has no such feature." );
	process.exit( 1 );
}

console.log( "device    : created with shared-texture-memory-d3d12-resource" );

// Control: an unknown name should be ignored with a log line, not fatal.
// Needs its own adapter — Dawn consumes an adapter on device creation and
// rejects any second requestDevice with "adapter is consumed".
const adapter2 = await sack.gpu.requestAdapter( { powerPreference: 'high-performance' } );
const d2 = await adapter2.requestDevice( { requiredFeatures: [ 'not-a-real-feature' ] } );
console.log( `unknown   : ${d2 ? "ignored, device still created" : "FAILED — should have been ignored"}` );
