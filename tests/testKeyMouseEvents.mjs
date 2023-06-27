
import {sack} from "sack.vfs";

const mouse = sack.Mouse( events );
const keys = sack.Keyboard( keyEvents );

function events( event ) {
    console.log( "Got Mouse Event:", event );
}

const control_keys = {
	alt:false,
	shift:false,
	control:false 
};

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
	if( event.down && control_keys.shift && control_keys.control && event.char === 'R' )
		console.log( "REBOOT?" );
    console.log( "Got Key Event:", event, control_keys );
}

