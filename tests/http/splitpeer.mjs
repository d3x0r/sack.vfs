// Plain node (run WITHOUT --import sack.vfs/import) - the server half of
// splitfuzz.mjs.  Answers `GET /<case>/<cut>` with the canned response for
// <case>, written as two segments broken at offset <cut> with a pause between
// them, so the client's parser sees a read boundary exactly there.  cut 0 (or
// >= the length) sends it in one write; `drip` sends it a byte per write, which
// puts a boundary at every offset in the same response.
import net from "node:net";
import { RESPONSES } from "./splitcases.mjs";

const PORT = Number( process.env.PORT ) || 8021;
const GAP = Number( process.env.GAP ) || 15;   // ms between the two writes

const sleep = ms=>new Promise( r=>setTimeout( r, ms ) );

net.createServer( sock=>{
	let seen = "";
	let answered = false;
	sock.on( "error", ()=>{} );
	sock.on( "data", async d=>{
		seen += d.toString( "latin1" );
		if( answered || seen.indexOf( "\r\n" ) < 0 ) return;
		answered = true;   // one canned reply per connection
		const m = /^[A-Z]+ \/([^/ ]+)\/(\d+|drip)/.exec( seen );
		const body = m && RESPONSES[m[1]];
		if( !body ) {
			sock.end( "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n" );
			return;
		}
		if( m[2] === "drip" ) {
			for( let n = 0; n < body.length; n++ ) {
				sock.write( body[n] );
				await sleep( 2 );
			}
			await sleep( GAP );
			sock.end();
			return;
		}
		const cut = Number( m[2] );
		if( cut > 0 && cut < body.length ) {
			sock.write( body.slice( 0, cut ) );
			await sleep( GAP );
			sock.write( body.slice( cut ) );
		} else
			sock.write( body );
		await sleep( GAP );
		sock.end();
	} );
} ).listen( PORT, "127.0.0.1", ()=>console.log( `splitpeer on ${PORT}` ) );
