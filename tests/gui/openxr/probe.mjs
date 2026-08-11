// OpenXR runtime probe. No Dawn, no window, no session — just enough to answer
// three questions before any interop work starts:
//
//   1. does the loader link and find a registered runtime?
//   2. which runtime is it?  (ActiveRuntime / XR_RUNTIME_JSON can surprise you)
//   3. does it offer XR_KHR_D3D12_enable, which the whole D3D12 plan rests on?
//
//   node tests/gui/openxr/probe.mjs
//
// Run it from the package root — the import below is a package self-reference,
// and no import hook is needed since nothing here loads JSOX or network modules.
// Note vfs_module.cjs prefers build/Release over build/Debug, so a stale Release
// .node without the XR module will shadow a freshly built Debug one.
//
// To target a specific runtime without changing the system default, point
// XR_RUNTIME_JSON at its manifest:
//
//   cmd:        set XR_RUNTIME_JSON=c:\...\SteamVR\steamxr_win64.json
//   PowerShell: $env:XR_RUNTIME_JSON='c:\...\SteamVR\steamxr_win64.json'
//
// Do NOT quote it in cmd's `set` — `set VAR="path"` stores the quotes as part of
// the value, and the loader then reports `failed to open "<path>". Does it exist?`
// while echoing back what looks like a perfectly good path. It's quoting, not the
// path.
//
// Pass --all to dump every advertised extension.

import { sack } from "@d3x0r/sack-gui";

const info = sack.xr.probe();

if( !info.ok ) {
	console.error( `XR probe failed at ${info.stage}: ${info.error}` );
	console.error( "" );
	console.error( "If this is 'no runtime': the loader reads" );
	console.error( "  HKLM\\SOFTWARE\\Khronos\\OpenXR\\1\\ActiveRuntime" );
	console.error( "which SteamVR sets when made the default OpenXR runtime," );
	console.error( "or override it for this process with XR_RUNTIME_JSON." );
	process.exit( 1 );
}

console.log( `runtime   : ${info.runtimeName} ${info.runtimeVersion}` );
console.log( `D3D12     : ${info.d3d12 ? "yes" : "NO — the D3D12 interop plan needs this"}` );
console.log( `D3D11     : ${info.d3d11 ? "yes" : "no"}` );
console.log( `extensions: ${info.extensions.length}` );

if( info.system ) {
	const s = info.system;
	console.log( `system    : ${s.name} (vendor 0x${s.vendorId.toString(16)})` );
	console.log( `swapchain : max ${s.maxSwapchainWidth}x${s.maxSwapchainHeight}, ${s.maxLayerCount} layers` );
	console.log( `tracking  : orientation=${s.orientationTracking} position=${s.positionTracking}` );
} else {
	// Normal when nothing is plugged in or SteamVR isn't running — the runtime
	// answered, there's just no HMD behind it.
	console.log( `system    : unavailable (${info.systemError})` );
}

if( process.argv.includes( "--all" ) ) {
	console.log( "" );
	for( const e of info.extensions ) console.log( `  ${e}` );
}
