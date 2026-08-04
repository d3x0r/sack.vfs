// Checks the specific claims made in README_HTTP.md's streaming section, so the
// documentation cannot drift away from the binding unnoticed.
import { sack } from "sack.vfs";

const PORT = Number( process.env.PORT ) || 8061;

let lastReq = null;
const server = new sack.WebSocket.Server( { port: PORT } );
server.onrequest = ( req, res ) => {
	lastReq = { url: req.url, method: req.method, headers: req.headers };
	res.writeHead( 200, { "Content-Type": "text/plain" } );
	res.end( `served ${req.url}` );
};

let failures = 0;
function check( label, ok, detail ) {
	if( ok ) console.log( `  ok   ${label}` );
	else { failures++; console.log( `  FAIL ${label} ${detail === undefined ? "" : detail}` ); }
}

// "path ... '/' if unspecified" and "method ... 'GET' if unspecified"
{
	const conn = await sack.HTTP.stream( { hostname: "localhost", port: PORT } );
	const res = await conn.request( {} );
	check( `request({}) defaults to GET /  (saw ${lastReq.method} ${lastReq.url})`
	     , lastReq.url === "/" && String( lastReq.method ).toUpperCase() === "GET" );
	check( "response has statusCode/status/content", res.statusCode === 200 && res.status === "OK" && res.content === "served /"
	     , JSON.stringify( { s: res.statusCode, t: res.status, c: res.content } ) );

	// "bytes | ArrayBuffer of the same content, undecoded"
	check( `bytes is an ArrayBuffer of ${res.content.length}b`
	     , res.bytes instanceof ArrayBuffer && res.bytes.byteLength === res.content.length
	     , `${res.bytes && res.bytes.constructor && res.bytes.constructor.name}/${res.bytes && res.bytes.byteLength}` );

	// "headers | array of header from response"
	const ct = res.headers["Content-Type"] || res.headers["content-type"];
	check( `headers indexed by field name (Content-Type: ${ct})`, !!ct );

	// "The status code does not decide whether the promise resolves"
	server.onrequest = ( req, res2 ) => { res2.writeHead( 404, {} ); res2.end( "nope" ); };
	const missing = await conn.request( { path: "/missing" } );
	check( `404 resolves rather than rejecting (statusCode ${missing.statusCode})`, missing.statusCode === 404 );
	server.onrequest = ( req, res2 ) => {
		lastReq = { url: req.url, method: req.method, headers: req.headers };
		res2.writeHead( 200, {} ); res2.end( `served ${req.url}` );
	};

	conn.close();
}

// "hostname ... may carry the port ('host:8080'), which overrides port"
{
	const conn = await sack.HTTP.stream( { hostname: `localhost:${PORT}` } );
	const res = await conn.request( { path: "/embedded" } );
	check( "hostname with embedded port overrides port", res.statusCode === 200 && res.content === "served /embedded"
	     , JSON.stringify( res.content ) );
	conn.close();
}

// "method/content/headers/agent/version" all reach the server
{
	const conn = await sack.HTTP.stream( { hostname: "localhost", port: PORT } );
	const res = await conn.request( { method: "POST", path: "/opts", content: "abc"
	                               , headers: { "X-Doc-Test": "yes" }, agent: "doctest/1", version: "1.1" } );
	check( `method reaches the server (${lastReq.method})`, String( lastReq.method ).toUpperCase() === "POST" );
	const h = lastReq.headers || {};
	const findHeader = n => Object.keys( h ).find( k => k.toLowerCase() === n );
	check( `custom header arrived (${findHeader( "x-doc-test" )})`, !!findHeader( "x-doc-test" ) );
	check( `agent arrived (${h[findHeader( "user-agent" )]})`, h[findHeader( "user-agent" )] === "doctest/1" );
	check( "POST still results 200", res.statusCode === 200 );
	conn.close();
}

// "close() ... Requests that have not been answered reject"
{
	const conn = await sack.HTTP.stream( { hostname: "localhost", port: PORT } );
	conn.close();
	try {
		await conn.request( { path: "/after" } );
		check( "request after close rejects", false, "resolved" );
	} catch( err ) { check( `request after close rejects (${err.message})`, true ); }
}

// "It rejects if the connection could not be made."
try {
	await sack.HTTP.stream( { hostname: "localhost", port: 1 } );
	check( "stream to a dead port rejects", false, "resolved" );
} catch( err ) { check( `stream to a dead port rejects (${err.message})`, true ); }

console.log( `\nREADME claims: failures=${failures}` );
process.exit( failures ? 1 : 0 );
