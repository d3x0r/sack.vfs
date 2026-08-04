// Plain-node raw TCP peer (run WITHOUT --import sack.vfs/import).
// Prints exactly what a client puts on the wire and answers each request line.
import net from "node:net";

const PORT = Number( process.env.PORT ) || 8097;
let reads = 0, seen = 0;

net.createServer( sock => {
	sock.on( "data", buf => {
		reads++;
		const lines = buf.toString( "latin1" ).split( "\r\n" ).filter( l => /^(GET|POST|PUT) /.test( l ) );
		console.log( `read#${reads} ${buf.length}b -> ${lines.length} request(s): ${lines.map( l => l.split( " " )[1] ).join( "," )}` );
		// COALESCE=1 puts every reply for this read into ONE write, so the client
		// receives several complete responses in a single segment - the case that
		// needs EndHttp to keep the bytes of the next reply.
		let batch = "";
		for( const line of lines ) {
			seen++;
			const body = `reply-${line.split( " " )[1]}`;
			const reply = `HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ${body.length}\r\n\r\n${body}`;
			if( process.env.COALESCE === "1" ) batch += reply;
			else sock.write( reply );
		}
		if( batch ) { console.log( `  replying with ${batch.length}b in one write` ); sock.write( batch ); }
	} );
	sock.on( "error", () => {} );
} ).listen( PORT, () => console.log( `wirepeer listening on ${PORT}` ) );

process.on( "SIGTERM", () => process.exit( 0 ) );
