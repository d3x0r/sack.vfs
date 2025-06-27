
import {Events} from "./events.mjs"

const events = new Events();

function f(x,y, arr) {
	console.log( "run on", x,y, "prior Results ", arr );
	return x*x + x*3;
}


events.on( "a", (y,a,b)=>f(1,y,a,b) );
events.on( "a", (y,a,b)=>f(2,y,a,b), 1 );
events.on( "a", (y,a,b)=>f(3,y,a,b), 2 );
events.on( "a", (y,a,b)=>f(4,y,a,b), 3 );

events.on( "a", 3 );

events.on( "a", (y,a,b)=>f(2,y,a,b), 1 );
events.on( "a", (y,a,b)=>f(3,y,a,b), 2 );
events.on( "a", (y,a,b)=>f(4,y,a,b), 3 );

events.on( "a", 5 );

