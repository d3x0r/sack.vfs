// Discriminator for the pipeline>1 hang: run the same sack streaming client
// against node's http server (which handles pipelining) and against sack's own
// server, and report how many requests each actually saw.
//   SERVER=node|sack  DEPTH=n  COUNT=n

import { sack } from "sack.vfs";
import http from "node:http";

const PORT = Number( process.env.PORT ) || 8098;
const DEPTH = Number( process.env.DEPTH ) || 4;
const COUNT = Number( process.env.COUNT ) || 8;
const WHICH = process.env.SERVER || "node";

let served = 0;
if( WHICH === "node" ) {
	http.createServer( ( req, res ) => {
		served++;
		console.log( `  server saw #${served} ${req.url}` );
		const body = JSON.stringify( { path: req.url, n: served } );
		res.writeHead( 200, { "Content-Type": "application/json", "Content-Length": body.length } );
		res.end( body );
	} ).listen( PORT );
} else {
	const server = new sack.WebSocket.Server( { port: PORT } );
	server.onrequest = ( req, res ) => {
		served++;
		console.log( `  server saw #${served} ${req.url}` );
		const body = JSON.stringify( { path: req.url, n: served } );
		res.writeHead( 200, { "Content-Type": "application/json" } );
		res.end( body );
	};
}

console.log( `server=${WHICH} depth=${DEPTH} count=${COUNT} port=${PORT}` );

const conn = await sack.HTTP.stream( { hostname: "localhost", port: PORT, pipeline: DEPTH } );

const paths = [];
for( let n = 0; n < COUNT; n++ ) paths.push( `/p/${n}` );

let answered = 0;
const timer = setTimeout( () => {
	console.log( `TIMEOUT: server saw ${served}, client got ${answered} of ${COUNT}` );
	process.exit( 2 );
}, 5000 );

const results = await Promise.all( paths.map( p =>
	conn.request( { method: "GET", path: p } ).then( r => { answered++; return r; } ) ) );

clearTimeout( timer );
let bad = 0;
results.forEach( ( res, n ) => {
	let body = null;
	try { body = JSON.parse( res.content ); } catch( e ) { /* not json */ }
	if( res.statusCode !== 200 || !body || body.path !== paths[n] ) {
		bad++;
		console.log( `  MISMATCH [${n}] wanted ${paths[n]} got ${body && body.path} (${res.statusCode})` );
	}
} );
console.log( `served=${served} answered=${answered} mismatched=${bad}` );
conn.close();
process.exit( bad ? 1 : 0 );
