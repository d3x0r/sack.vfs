// Streaming HTTP client: one connection carrying many requests.
//   sack.HTTP.stream( {hostname,port} )  -> Promise<connection>
//   connection.request( {method,path,...} ) -> Promise<result>
//   connection.close()
// Replies correlate to requests strictly in order, so the interesting failure
// this checks for is a reply landing on the wrong promise.

import { sack } from "sack.vfs";

const PORT = Number( process.env.PORT ) || 8099;

let served = 0;
const server = new sack.WebSocket.Server( { port: PORT } );
server.onrequest = ( req, res ) => {
	served++;
	// echo the path back so every reply can be matched to its request
	const body = JSON.stringify( { path: req.url, n: served } );
	res.writeHead( 200, { "Content-Type": "application/json" } );
	res.end( body );
};

let failures = 0;
function check( label, ok, detail ) {
	if( ok ) console.log( `  ok   ${label}` );
	else { failures++; console.log( `  FAIL ${label} ${detail === undefined ? "" : detail}` ); }
}

function bodyOf( res ) {
	try { return JSON.parse( res.content ); } catch( e ) { return null; }
}

async function sequential( conn ) {
	console.log( "sequential:" );
	for( let n = 0; n < 5; n++ ) {
		const path = `/seq/${n}`;
		const res = await conn.request( { method: "GET", path } );
		const body = bodyOf( res );
		check( `${path} -> ${res.statusCode} ${res.status}`
		     , res.statusCode === 200 && body && body.path === path
		     , JSON.stringify( { statusCode: res.statusCode, content: res.content } ) );
	}
}

async function parallel( conn, count ) {
	console.log( `parallel x${count}:` );
	const paths = [];
	for( let n = 0; n < count; n++ ) paths.push( `/par/${n}` );
	const results = await Promise.all( paths.map( path => conn.request( { method: "GET", path } ) ) );
	let mismatched = 0;
	results.forEach( ( res, n ) => {
		const body = bodyOf( res );
		if( res.statusCode !== 200 || !body || body.path !== paths[n] )
			mismatched++;
	} );
	check( `${count} replies landed on the right promises`, mismatched === 0, `${mismatched} mismatched` );
}

async function post( conn ) {
	console.log( "post:" );
	const res = await conn.request( { method: "POST", path: "/post", content: "hello body" } );
	check( `POST -> ${res.statusCode}`, res.statusCode === 200 );
}

async function headers( conn ) {
	console.log( "headers:" );
	const res = await conn.request( { method: "GET", path: "/hdr", headers: { "X-Test": "yes" } } );
	const ct = res.headers["Content-Type"] || res.headers["content-type"];
	check( `reply carried headers (Content-Type: ${ct})`, !!ct );
}

async function afterClose( conn ) {
	console.log( "after close:" );
	conn.close();
	try {
		await conn.request( { method: "GET", path: "/nope" } );
		check( "request after close rejects", false, "resolved instead" );
	} catch( err ) {
		check( `request after close rejects (${err.message})`, true );
	}
}

async function run( label, opts ) {
	console.log( `\n=== ${label} ===` );
	const conn = await sack.HTTP.stream( opts );
	await sequential( conn );
	await parallel( conn, 20 );
	await post( conn );
	await headers( conn );
	await afterClose( conn );
}

try {
	await run( "on demand (pipeline 1)", { hostname: "localhost", port: PORT } );
	// Depth > 1 packs several requests into one segment, which is what the
	// GatherHttpData request/response content-state fix made survivable on the
	// server; rawpipe.mjs is the sack-client-free repro of that.
	await run( "pipelined (pipeline 4)", { hostname: "localhost", port: PORT, pipeline: 4 } );

	console.log( "\nconnect failure:" );
	try {
		await sack.HTTP.stream( { hostname: "localhost", port: 1 } );
		check( "connect to a dead port rejects", false, "resolved instead" );
	} catch( err ) {
		check( `connect to a dead port rejects (${err.message})`, true );
	}
} catch( err ) {
	failures++;
	console.log( "threw:", err );
}

console.log( `\nserved=${served} failures=${failures}` );
process.exit( failures ? 1 : 0 );
