// Bisect the spin seen in testHttpStreamDocs: STAGE selects the sequence.
import { sack } from "sack.vfs";
const PORT = Number(process.env.PORT)||8059;
const STAGE = process.env.STAGE || "second";

let code = 200;
const s = new sack.WebSocket.Server( { port: PORT } );
s.onrequest = ( req, res ) => { res.writeHead(code,{}); res.end("x"); };

async function once( label, opts, path ) {
	const c = await sack.HTTP.stream( opts );
	console.log( `${label}: connected` );
	const r = await c.request( { path } );
	console.log( `${label}: ${r.statusCode}` );
	c.close();
	console.log( `${label}: closed` );
	return c;
}

if( STAGE === "second" ) {
	await once( "first", { hostname:"localhost", port:PORT }, "/a" );
	await once( "secnd", { hostname:"localhost", port:PORT }, "/b" );
} else if( STAGE === "embedded" ) {
	await once( "first", { hostname:"localhost", port:PORT }, "/a" );
	await once( "embed", { hostname:`localhost:${PORT}` }, "/b" );
} else if( STAGE === "404" ) {
	const c = await sack.HTTP.stream( { hostname:"localhost", port:PORT } );
	console.log( "connected" );
	console.log( "200:", (await c.request({path:"/ok"})).statusCode );
	code = 404;
	console.log( "404:", (await c.request({path:"/missing"})).statusCode );
	code = 200;
	console.log( "200 again:", (await c.request({path:"/ok2"})).statusCode );
	c.close();
	console.log( "closed" );
} else if( STAGE === "reassign" ) {
	const c = await sack.HTTP.stream( { hostname:"localhost", port:PORT } );
	console.log( "200:", (await c.request({path:"/ok"})).statusCode );
	s.onrequest = ( req, res ) => { res.writeHead(404,{}); res.end("nope"); };
	console.log( "404:", (await c.request({path:"/missing"})).statusCode );
	s.onrequest = ( req, res ) => { res.writeHead(200,{}); res.end("x"); };
	c.close();
	console.log( "closed" );
}
await new Promise( r => setTimeout( r, 3000 ) );
console.log( "survived idle" );
process.exit(0);
