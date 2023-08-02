
import {sack} from "sack.vfs";

setTimeout( f, 10000 );

function f() {
	console.log( 'did you sigint?' );
}

process.on( "SIGINT", ()=>{
	console.log( "My Sigint anyway...(gen exit)" );
	process.exit(0);
} );
