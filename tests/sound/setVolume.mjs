
import {sack} from "sack.vfs"


const devices = sack.sound.devices

let defaultDevice = null;

for( let s of devices ) {
	if( s.default ) {
		defaultDevice = s;
		console.log( "Current:", s.name, s.volume, 1-s.volume );	
		s.setVolume( 1-s.volume );
		console.log( "Current:", s.name, s.volume );	
	}
}

//setTimeout( ()=>{}, 5000 );