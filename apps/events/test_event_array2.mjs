
import {Events} from "./events.mjs"

const events = new Events();

function f(x,y) {
	console.log( "run on", x,y  );
	return x*x + x*3;
}


events.on( "a", f );
events.on( "a", 3 );

events.on( "a", [5,3] );

