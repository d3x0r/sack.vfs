// Websocket upgrade regression: the 101 response path shares the HTTP parser
// that GatherHttpData was changed in, and 101 is precisely the response case
// the fix deliberately left alone.  Also serves plain HTTP on the same server
// so both shapes of the parser are exercised.
import { sack } from "sack.vfs";

const PORT = Number( process.env.PORT ) || 8067;
const N = Number( process.env.N ) || 50;

const server = sack.WebSocket.Server( { port: PORT } );
let opened = 0;
server.on( "connect", ws => {
	opened++;
	ws.on( "message", msg => ws.send( "echo:" + msg ) );
} );
server.onrequest = ( req, res ) => { res.writeHead( 200, {} ); res.end( "http-ok" ); };

let bad = 0, got = 0;
await new Promise( resolve => {
	const done = setTimeout( () => { console.log( "ws timeout" ); bad++; resolve(); }, 8000 );
	const ws = sack.WebSocket.Client( `ws://localhost:${PORT}/` );
	ws.on( "open", () => { for( let n = 0; n < N; n++ ) ws.send( `m${n}` ); } );
	ws.on( "message", msg => {
		const s = String( msg );
		if( s !== `echo:m${got}` ) { bad++; if( bad < 4 ) console.log( "bad ws msg", got, s ); }
		got++;
		if( got >= N ) { clearTimeout( done ); ws.close(); resolve(); }
	} );
	ws.on( "error", e => { console.log( "ws error", e ); bad++; clearTimeout( done ); resolve(); } );
} );
console.log( `ws: connects=${opened} echoed=${got}/${N} bad=${bad}` );

// plain HTTP on the same server, after the upgrade traffic
const conn = await sack.HTTP.stream( { hostname: "localhost", port: PORT } );
const res = await conn.request( { method: "GET", path: "/plain" } );
conn.close();
if( res.statusCode !== 200 || res.content !== "http-ok" ) { bad++; console.log( "bad http", res.statusCode, res.content ); }
console.log( `http on same server: ${res.statusCode} ${JSON.stringify( res.content )}` );

process.exit( bad ? 1 : 0 );
