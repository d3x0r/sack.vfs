


import {sack} from "sack.vfs"

const keyboard = sack.Keyboard( keyEvents );

const control_keys = {
	alt:false,
	shift:false,
	control:false 
};

let usedRebootKey = false;
let usedServiceRestartKey = false;

function keyEvents( event ) {
	if( event.down ) {
		if( event.scan == 56 ) control_keys.alt=true;
		if( event.scan == 42 ) control_keys.shift=true;
		if( event.scan == 54 ) control_keys.shift=true;
		if( event.scan == 29 ) control_keys.control = true;
	} else {
		if( event.scan == 56 ) control_keys.alt=false;
		if( event.scan == 42 ) control_keys.shift=false;
		if( event.scan == 54 ) control_keys.shift=false;
		if( event.scan == 29 ) control_keys.control = false;
	}
	if( event.down && !control_keys.alt && control_keys.ctrl && control_keys.control && event.char === 'R' ) {
		if( usedRebootKey ) {
			console.log( "Let go of the R key?!" );
			return true;
		}
		console.log( "Key Event Triggered to REBOOT." );
		sack.system.reboot( "reboot" );
		usedRebootKey = true;
		return true; // used this key.
	}
	if( !event.down && !control_keys.alt && control_keys.shift && control_keys.control && event.char === 'R' ) {
		if( usedRebootKey ) {
			usedRebootKey = false;
			return true;
		} 
	}
