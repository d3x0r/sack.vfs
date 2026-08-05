// Asserts what the client puts ON THE WIRE.  reqpeer.mjs echoes each request's
// own header block back as the body, so every check here is against the real
// bytes rather than against what we meant to send.
//
//   node --import sack.vfs/import tests/http/reqline.mjs
//
// Two regressions this pins, both found against sqlite.org (althttpd):
//   - a path with no leading slash went out verbatim as `GET download.html
//     HTTP/1.1`.  althttpd's LikelyHackAttempt() treats a target that does not
//     start with '/' as an attack and shuns the source IP for 300s per offense,
//     with every retry adding another - so a typo'd path got the machine banned.
//   - headers were written "Name:value" with no space.  That is legal (RFC 9110
//     makes the OWS optional) but althttpd splits the line on whitespace and
//     compares the first token to "Host:", so the host was lost and the request
//     404'd with "Missing HOST: parameter".
import { spawn } from "node:child_process";
import { sack } from "sack.vfs";

const PORT = Number( process.env.PORT ) || 8022;

const peer = spawn( process.execPath, [ "tests/http/reqpeer.mjs" ]
	, { env: { ...process.env, PORT: String( PORT ) }, stdio: [ "ignore", "pipe", "inherit" ] } );
await new Promise( resolve=>peer.stdout.once( "data", resolve ) );

function sent( opts ) {
	return new Promise( resolve=>{
		let done = false;
		const finish = r=>{ if( !done ) { done = true; resolve( r && r.content ? r.content : "" ); } };
		sack.HTTP.get( { hostname:"127.0.0.1", port:PORT, timeout:2000, retries:1, onReply: finish, ...opts } );
		setTimeout( ()=>finish( null ), 2500 );
	} );
}

let failures = 0;
function check( what, ok, detail ) {
	if( !ok ) failures++;
	console.log( `  ${ok?"ok  ":"FAIL"} ${what}${ok?"":`\n         ${detail}`}` );
}
const lines = wire=>wire.split( "\r\n" ).filter( l=>l );
const field = ( wire, name )=>lines( wire ).find( l=>l.toLowerCase().startsWith( name.toLowerCase() + ":" ) );

// --- request target -------------------------------------------------------
{
	const wire = await sent( { path:"download.html" } );
	check( "a path with no leading slash gets one", lines( wire )[0] === "GET /download.html HTTP/1.1", lines( wire )[0] );
}
{
	const wire = await sent( { path:"/download.html" } );
	check( "a normal path is left alone", lines( wire )[0] === "GET /download.html HTTP/1.1", lines( wire )[0] );
}
{
	const wire = await sent( { path:"/a/b?x=1&y=2" } );
	check( "query string survives", lines( wire )[0] === "GET /a/b?x=1&y=2 HTTP/1.1", lines( wire )[0] );
}
{
	const wire = await sent( { path:"http://127.0.0.1/proxied" } );
	check( "absolute-form target is not re-slashed", lines( wire )[0] === "GET http://127.0.0.1/proxied HTTP/1.1", lines( wire )[0] );
}

// --- header shape ---------------------------------------------------------
{
	const wire = await sent( { path:"/h" } );
	const bad = lines( wire ).slice( 1 ).filter( l=>!/^[^:]+: /.test( l ) );
	check( "every header has a space after the colon", bad.length === 0, bad.join( " | " ) );
	check( "Host names the peer", field( wire, "host" ) === `Host: 127.0.0.1:${PORT}`, field( wire, "host" ) );
	check( "default User-Agent is sent", /^User-Agent: \S/.test( field( wire, "user-agent" ) || "" ), field( wire, "user-agent" ) );
}
{
	const wire = await sent( { path:"/h", agent:"splitfuzz/1" } );
	check( "agent option overrides the default", field( wire, "user-agent" ) === "User-Agent: splitfuzz/1", field( wire, "user-agent" ) );
}
{
	const wire = await sent( { path:"/h", headers:{ "X-Test":"a value", "Accept":"*/*" } } );
	check( "caller headers arrive with the space", field( wire, "x-test" ) === "X-Test: a value", field( wire, "x-test" ) );
	check( "caller Accept arrives", field( wire, "accept" ) === "Accept: */*", field( wire, "accept" ) );
}
{
	const wire = await sent( { path:"/h", headers:{ "User-Agent":"caller/9" } } );
	const uas = lines( wire ).filter( l=>/^user-agent:/i.test( l ) );
	check( "a caller User-Agent replaces rather than duplicates", uas.length === 1 && uas[0] === "User-Agent: caller/9", uas.join( " | " ) );
}

peer.kill( "SIGKILL" );
console.log( `\nrequest shape: failures=${failures}` );
process.exit( failures ? 1 : 0 );
