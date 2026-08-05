// Streaming client half of the wire test; talks to wirepeer.mjs.
import { sack } from "sack.vfs";

const PORT = Number( process.env.PORT ) || 8097;
const DEPTH = Number( process.env.DEPTH ) || 4;
const COUNT = Number( process.env.COUNT ) || 8;

const conn = await sack.HTTP.stream( { hostname: "localhost", port: PORT, pipeline: DEPTH } );

const paths = [];
for( let n = 0; n < COUNT; n++ ) paths.push( `/p/${n}` );

let answered = 0;
const timer = setTimeout( () => {
	console.log( `TIMEOUT answered=${answered}/${COUNT}` );
	process.exit( 2 );
}, 5000 );

const results = await Promise.all( paths.map( p =>
	conn.request( { method: "GET", path: p } ).then( r => { answered++; return r; } ) ) );
clearTimeout( timer );

let bad = 0;
results.forEach( ( res, n ) => {
	if( res.content !== `reply-${paths[n]}` ) {
		bad++;
		console.log( `  MISMATCH [${n}] wanted reply-${paths[n]} got ${JSON.stringify( res.content )}` );
	}
} );
console.log( `answered=${answered} mismatched=${bad}` );
conn.close();
process.exit( bad ? 1 : 0 );
