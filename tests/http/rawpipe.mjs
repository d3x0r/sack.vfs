// Plain-node pipelining client (run WITHOUT --import sack.vfs/import).
// Writes N HTTP/1.1 requests in ONE socket write and counts the replies, so a
// server that drops coalesced pipelined requests shows up with no sack client
// involved at all.
//   PORT=<server port>  COUNT=<requests>
import net from "node:net";

const PORT = Number( process.env.PORT ) || 8080;
const COUNT = Number( process.env.COUNT ) || 4;

let req = "";
for( let n = 0; n < COUNT; n++ )
	req += `GET /raw/${n} HTTP/1.1\r\nHost: localhost:${PORT}\r\nConnection: Keep-Alive\r\nContent-Length:0\r\n\r\n`;

const sock = net.connect( PORT, "localhost", () => {
	console.log( `wrote ${COUNT} requests in one ${req.length}b write` );
	sock.write( req );
} );

let acc = "";
const timer = setTimeout( () => {
	const replies = ( acc.match( /HTTP\/1\.[01] /g ) || [] ).length;
	console.log( `RESULT: sent=${COUNT} replies=${replies}` );
	process.exit( replies === COUNT ? 0 : 1 );
}, 2500 );

sock.on( "data", d => {
	acc += d.toString( "latin1" );
	const replies = ( acc.match( /HTTP\/1\.[01] /g ) || [] ).length;
	if( replies >= COUNT ) {
		clearTimeout( timer );
		console.log( `RESULT: sent=${COUNT} replies=${replies}` );
		process.exit( 0 );
	}
} );
sock.on( "error", e => { console.log( "socket error", e.message ); process.exit( 3 ); } );
