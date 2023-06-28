
import {sack} from "sack.vfs";

const mouse = sack.Mouse( events );
const keys = sack.Keyboard( keyEvents );

const mouse2 = sack.Mouse( events2 );
const keys2 = sack.Keyboard( keyEvents2 );

function events( event ) {
    console.log( "Got Mouse Event 1:", event );
}

function events2( event ) {
    console.log( "Got Mouse Event 2:", event );
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
    console.log( "Got Key Event 1:", event, control_keys );
}


const control_keys2 = {
	alt:false,
	shift:false,
	control:false 
};

function keyEvents2( event ) {
	if( event.down ) {
		if( event.scan == 56 ) control_keys2.alt=true;
		if( event.scan == 42 ) control_keys2.shift=true;
		if( event.scan == 54 ) control_keys2.shift=true;
		if( event.scan == 29 ) control_keys2.control = true;
	} else {
		if( event.scan == 56 ) control_keys2.alt=false;
		if( event.scan == 42 ) control_keys2.shift=false;
		if( event.scan == 54 ) control_keys2.shift=false;
		if( event.scan == 29 ) control_keys2.control = false;
	}
	if( event.down && control_keys2.shift && control_keys2.control && event.char === 'R' )
		console.log( "REBOOT 2?" );
    console.log( "Got Key Event 2:", event, control_keys2 );
}

