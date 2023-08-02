
import {sack} from "sack.vfs";

const tick = setTimeout( f, 10000 );

function f() {
	console.log( 'did you sigint?' );
}

process.on( "SIGINT", ()=>{
	clearTimeout( tick );
	console.log( "My Sigint just stops what's waiting...." );
} );
