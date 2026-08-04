// Regression soak for the http.c parser changes (EndHttp keeping the next
// message, and GatherHttpData no longer letting a REQUEST swallow the buffer):
// ordinary keep-alive serving, request bodies, big response bodies, pipelining,
// and the pre-existing one-shot client - all sharing the same parser.
import { sack } from "sack.vfs";
const PORT = Number(process.env.PORT)||8075;
const N = Number(process.env.N)||500;

let served = 0;
const s = new sack.WebSocket.Server( { port: PORT } );
s.onrequest = ( req, res ) => {
	served++;
	// echo the received request-body length back, so a request that carries
	// content is verified end to end and not just by its status code
	const got = req.content ? String( req.content ).length : 0;
	const body = `ok-${req.url}:${got}${"x".repeat( Number(req.CGI?.size)||0 )}`;
	res.writeHead( 200, { "Content-Type":"text/plain" } );
	res.end( body );
};

// 1) streaming connection, sequential + parallel, default depth
const conn = await sack.HTTP.stream( { hostname:"localhost", port:PORT } );
let bad = 0;
for( let n = 0; n < N; n++ ) {
	const r = await conn.request( { method:"GET", path:`/s/${n}` } );
	if( r.statusCode !== 200 || r.content !== `ok-/s/${n}:0` ) { bad++; if(bad<4) console.log("bad seq", n, r.statusCode, r.content); }
}
console.log( `sequential ${N}: bad=${bad}` );

const par = [];
for( let n = 0; n < N; n++ ) par.push( conn.request( { method:"GET", path:`/p/${n}` } ) );
const pres = await Promise.all( par );
let badp = 0;
pres.forEach( (r,n) => { if( r.statusCode !== 200 || r.content !== `ok-/p/${n}:0` ) { badp++; if(badp<4) console.log("bad par", n, r.content); } } );
console.log( `parallel  ${N}: bad=${badp}` );

// 1b) requests that CARRY a body - the branch whose request/response semantics
// were split; the server echoes back how many bytes it actually received.
let badc = 0;
for( const len of [ 1, 10, 500, 5000, 40000 ] ) {
	for( const method of [ "POST", "PUT" ] ) {
		const r = await conn.request( { method, path:"/body", content:"z".repeat( len ) } );
		if( r.statusCode !== 200 || r.content !== `ok-/body:${len}` ) {
			badc++; console.log( `bad ${method} body len=${len}:`, r.statusCode, r.content && r.content.slice(0,40) );
		}
	}
}
console.log( `request bodies (POST/PUT x5 sizes): bad=${badc}` );

// 1c) pipelined depth 4 over its own connection
const pconn = await sack.HTTP.stream( { hostname:"localhost", port:PORT, pipeline:4 } );
const ppar = [];
for( let n = 0; n < N; n++ ) ppar.push( pconn.request( { method:"GET", path:`/q/${n}` } ) );
const ppres = await Promise.all( ppar );
let badpp = 0;
ppres.forEach( (r,n) => { if( r.statusCode !== 200 || r.content !== `ok-/q/${n}:0` ) { badpp++; if(badpp<4) console.log("bad pipe", n, r.content); } } );
console.log( `pipelined ${N}: bad=${badpp}` );
pconn.close();

// 2) big bodies through the same connection (exercises content-length splitting)
let badb = 0;
for( let n = 0; n < 20; n++ ) {
	const r = await conn.request( { method:"GET", path:`/big?size=40000` } );
	if( r.statusCode !== 200 || r.content.length < 40000 ) { badb++; console.log("bad big", r.statusCode, r.content && r.content.length); }
}
console.log( `big x20: bad=${badb}` );

// 3) the pre-existing one-shot client still works (EndHttp is shared with it)
let bad1 = 0;
await Promise.all( Array.from( {length:100}, (_,n) => new Promise( resolve => {
	sack.HTTP.get( { hostname:"localhost", port:PORT, method:"GET", path:`/one/${n}`,
		onReply( r ) { if( r.statusCode !== 200 ) { bad1++; } resolve(); } } );
} ) ) );
console.log( `one-shot x100: bad=${bad1}` );

conn.close();
console.log( `served=${served} TOTALBAD=${bad+badp+badc+badpp+badb+bad1}` );
process.exit( (bad+badp+badc+badpp+badb+bad1) ? 1 : 0 );
