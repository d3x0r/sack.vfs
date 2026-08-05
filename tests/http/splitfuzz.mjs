// Walks the read boundary through every offset of a fixed response and checks
// the client parses the same thing every time.  Spawns splitpeer.mjs (plain
// node) to do the splitting; see splitcases.mjs for the responses.
//
//   node --import sack.vfs/import tests/http/splitfuzz.mjs [case|all]
//
// The parser carries line state (bLine/partial/scanned) from one read to the
// next, so a boundary landing on or inside a line ending is what breaks it.
// Two regressions this pins, both from a read that ended exactly after a CRLF:
//   - after the status line: the status line was re-scanned with its CRLF
//     already counted, so it was parsed as an empty line and the reply came
//     back as "No Content" with no code (sqlite.org's 503 arrives this way).
//   - after the last header: the leftover CRLF was counted into the body.
import { spawn } from "node:child_process";
import { sack } from "sack.vfs";
import { RESPONSES } from "./splitcases.mjs";

const PORT = Number( process.env.PORT ) || 8021;

const peer = spawn( process.execPath, [ "tests/http/splitpeer.mjs" ]
	, { env: { ...process.env, PORT: String( PORT ) }, stdio: [ "ignore", "pipe", "inherit" ] } );
await new Promise( resolve=>peer.stdout.once( "data", resolve ) );

function request( which, cut ) {
	return new Promise( resolve=>{
		let done = false;
		const finish = r=>{ if( !done ) { done = true; resolve( r ); } };
		sack.HTTP.get( { hostname:"127.0.0.1", port:PORT, path:`/${which}/${cut}`, timeout:2000, retries:1
		               , onReply: finish } );
		setTimeout( ()=>finish( { error:"NO REPLY" } ), 2500 );
	} );
}

function describe( res ) {
	if( !res ) return "(null)";
	if( res.error ) return "error:" + res.error;
	return `code=${res.statusCode} status=${JSON.stringify(res.status)} len=${res.content?res.content.length:0}`;
}

async function runCase( which ) {
	const response = RESPONSES[which];
	const results = new Map();
	const add = ( d, where )=>{
		if( !results.has( d ) ) results.set( d, [] );
		results.get( d ).push( where );
	};
	// the unsplit response is the reference - every cut has to agree with it
	const whole = describe( await request( which, 0 ) );
	add( whole, "whole" );
	for( let cut = 1; cut < response.length; cut++ )
		add( describe( await request( which, cut ) ), String( cut ) );
	// and once with a boundary at every offset at the same time
	add( describe( await request( which, "drip" ) ), "drip" );

	console.log( `\n== ${which} (${response.length} bytes) unsplit: ${whole}` );
	let bad = 0;
	for( const [ d, where ] of results ) {
		const ok = d === whole;
		if( !ok ) bad += where.length;
		console.log( `  ${ok?"ok  ":"FAIL"} ${String(where.length).padStart(3)} boundaries -> ${d}` );
		if( !ok )
			for( const c of where )
				console.log( `        at ${c}${( c==="whole" || c==="drip" )?"":` after ${JSON.stringify( response.slice( Math.max( 0, c-10 ), c ) )}`}` );
	}
	return bad;
}

const pick = process.argv[2] || "all";
const cases = pick === "all" ? Object.keys( RESPONSES ) : [ pick ];
if( cases.some( c=>!RESPONSES[c] ) ) {
	console.log( "cases:", Object.keys( RESPONSES ).join( " " ), "all" );
	peer.kill( "SIGKILL" );
	process.exit( 1 );
}

let bad = 0;
for( const c of cases ) bad += await runCase( c );
peer.kill( "SIGKILL" );
console.log( bad ? `\nFAIL: ${bad} read boundaries parsed differently than the unsplit response`
                 : "\nPASS: every read boundary agrees with the unsplit response" );
process.exit( bad ? 1 : 0 );
