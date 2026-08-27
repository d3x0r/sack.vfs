// Does the opening-byte gate let real HTTP through?  Raw sockets so we control bytes exactly.
const sack = require( process.env.SACK_NODE || "c:/general/work/javascript/vfs/native/build/RelWithDebInfo/sack_vfs.node" );
const net = require( "net" );
const port = Number(process.env.PORT) || 8478;
const seen = [];
const server = sack.WebSocket.Server( { port } );
server.onrequest = ( req, res ) => { seen.push( req.method ); res.writeHead(200,{'Content-Type':'text/plain'}); res.end("ok"); };

function raw( name, payload, expectOk ) {
	return new Promise( ( done ) => {
		let got = "";
		const s = net.connect( port, "127.0.0.1", () => s.write( payload ) );
		s.on( "data", ( d ) => { got += d.toString(); } );
		const finish = ( why ) => { s.destroy(); done( { name, ok: got.startsWith("HTTP/"), why, expectOk } ); };
		s.on( "error", () => finish( "error" ) );
		s.on( "close", () => finish( "close" ) );
		setTimeout( () => finish( got ? "data" : "TIMEOUT" ), 3000 );
	} );
}

setTimeout( async () => {
	const cases = [
		[ "plain GET",        "GET / HTTP/1.1\r\nHost: x\r\n\r\n", true ],
		[ "OPTIONS",          "OPTIONS / HTTP/1.1\r\nHost: x\r\n\r\n", true ],
		[ "DELETE",           "DELETE /a HTTP/1.1\r\nHost: x\r\n\r\n", true ],
		[ "leading CRLF",     "\r\n\r\nGET / HTTP/1.1\r\nHost: x\r\n\r\n", false ],
		[ "keep-alive x2",    "GET /1 HTTP/1.1\r\nHost: x\r\n\r\nGET /2 HTTP/1.1\r\nHost: x\r\n\r\n", true ],
		[ "TLS ClientHello",  Buffer.from([0x16,0x03,0x01,0x00,0x2f,0x01,0x00,0x00,0x2b,0x03,0x03]), false ],
		[ "SSH banner",       "SSH-2.0-OpenSSH_8.9\r\n", false ],
		[ "binary junk",      Buffer.from([0x00,0xff,0xfe,0x01,0x02,0x03]), false ],
	];
	let bad = 0;
	for( const [ n, p, exp ] of cases ) {
		const r = await raw( n, p, exp );
		const pass = ( r.ok === exp );
		if( !pass ) bad++;
		console.log( (pass?"  ok  ":"  FAIL") + "  " + n.padEnd(18)
		           + " responded=" + r.ok + " expected=" + exp + " (" + r.why + ")" );
	}
	console.log( "methods reaching app:", JSON.stringify( seen ) );
	console.log( bad === 0 ? "GATE PASS" : "GATE FAIL" );
	process.exit( bad === 0 ? 0 : 1 );
}, 400 );
