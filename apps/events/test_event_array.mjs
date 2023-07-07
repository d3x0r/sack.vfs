
import {Events} from "./events.mjs"

const events = new Events();

function f(x,y,s,a) {
	console.log( "run on", x,y,s );
	console.log( "run after", a );
	return x*x + x*3;
}


events.on( "a", f );

let result;

result = events.on( "a", 3 );
console.log( "result", result );


const disable = events.on( "a", f );
disable.enableArrayArgs = false;

result = events.on( "a", [5,3,"blah"] );
console.log( "disabled result", result );

