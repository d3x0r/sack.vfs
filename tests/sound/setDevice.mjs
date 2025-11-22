
import {sack} from "sack.vfs"


const devices = sack.sound.devices

let defaultDevice = null;

for( let s of devices ) {
	if( s.default ) {
		defaultDevice = s;
		console.log( "Current:", s.name );	
	}
}

if( defaultDevice === devices[0] )
	sack.sound.default = devices[1];
else 
	sack.sound.default = devices[0];

//setTimeout( ()=>{}, 5000 );