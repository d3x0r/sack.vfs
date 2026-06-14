

import {sack} from "sack-gui"

console.log( "Navigator?", navigator, Object.getPrototypeOf( navigator ), sack );

console.log( "Fun:", navigator.hardwareConcurrency, navigator.language, navigator.languages, navigator.userAgent, navigator.platform );

console.log( "Adapter?",  sack.gpu.requestAdapter().then( (a)=>{console.log( "waited and got:", a, a.info )}) );

function tick() {
	setTimeout( ()=>{console.log( "tick..." ); tick() }, 2000 );
}
tick()