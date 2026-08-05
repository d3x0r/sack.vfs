// Plain node (run WITHOUT --import sack.vfs/import) - the server half of
// reqline.mjs.  Echoes each request's own header block back as the response
// body so the client can assert on exactly what it put on the wire.
import net from "node:net";

const PORT = Number( process.env.PORT ) || 8022;

net.createServer( sock=>{
	let seen = "";
	sock.on( "error", ()=>{} );
	sock.on( "data", d=>{
		seen += d.toString( "latin1" );
		const end = seen.indexOf( "\r\n\r\n" );
		if( end < 0 ) return;
		const head = seen.slice( 0, end + 2 );   // request line + fields, one trailing CRLF
		seen = "";
		const body = Buffer.from( head, "latin1" );
		sock.write( `HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ${body.length}\r\n\r\n` );
		sock.write( body );
	} );
} ).listen( PORT, "127.0.0.1", ()=>console.log( `reqpeer on ${PORT}` ) );
