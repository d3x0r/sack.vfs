// Pins the loop-lifetime contract of a streaming connection (run with plain
// node; it spawns the sack processes as children).
//
//   an OPEN connection keeps the process alive - idle or not - and only
//   close() releases it.  Same rule as net.Socket: "no requests right now" is
//   not "no requests coming", so an idle connection must not let node exit out
//   from under the next request.
//
// Each child ends its script without calling process.exit(), so whether node
// returns to the shell is the measurement.
import { spawn } from "node:child_process";

const PORT = Number( process.env.PORT ) || 8023;
const GRACE = 4000;  // long enough that a process which is going to exit, has

const cases = [
	{ stage: "norequest", exits: false, why: "open connection with no request still pins the loop" },
	{ stage: "idle",      exits: false, why: "open connection idle after a request still pins the loop" },
	{ stage: "pending",   exits: false, why: "request in flight keeps the loop alive to settle it" },
	{ stage: "closed",    exits: true,  why: "close() releases the loop" },
];

function run( stage ) {
	return new Promise( resolve => {
		const child = spawn( process.execPath
			, [ "--import", "sack.vfs/import", "tests/http/exitrepro.mjs" ]
			, { env: { ...process.env, STAGE: stage, PORT: String( PORT ) }, stdio: [ "ignore", "pipe", "pipe" ] } );
		let out = "";
		child.stdout.on( "data", d => out += d );
		child.stderr.on( "data", d => out += d );
		const timer = setTimeout( () => { child.kill( "SIGKILL" ); resolve( { exited: false, out } ); }, GRACE );
		child.on( "exit", () => { clearTimeout( timer ); resolve( { exited: true, out } ); } );
	} );
}

const peer = spawn( process.execPath, [ "tests/http/wirepeer.mjs" ]
	, { env: { ...process.env, PORT: String( PORT ) }, stdio: "ignore" } );
await new Promise( r => setTimeout( r, 700 ) );

let failures = 0;
for( const c of cases ) {
	const { exited } = await run( c.stage );
	const ok = exited === c.exits;
	if( !ok ) failures++;
	console.log( `  ${ok ? "ok  " : "FAIL"} ${c.stage}: ${exited ? "exited" : "stayed up"} - ${c.why}` );
}

peer.kill( "SIGKILL" );
console.log( `\nloop lifetime: failures=${failures}` );
process.exit( failures ? 1 : 0 );
