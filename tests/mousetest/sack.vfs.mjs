
import { createRequire } from 'node:module';
const require = createRequire(import.meta.url);

// Now you can use require as usual
const sack = require('./sack_vfs.node');

const keyboard = sack.Keyboard( keyEvents );

const control_keys = {
	alt:false,
	shift:false,
	control:false 
};

let usedRebootKey = false;
let usedTabKey = false;
let usedServiceRestartKey = false;

function keyEvents( event ) {
	//console.log( "Got Key:", event );
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
	if( event.down && !control_keys.alt && control_keys.control && control_keys.shift && event.char === 'W' ) {
		if( usedRebootKey ) {
			console.log( "Let go of the W key?!" );
			return true;
		}
		 usedRebootKey = true;
		return true; // used this key.
	}
	if( !event.down && !control_keys.alt && control_keys.shift && control_keys.control && event.char === 'W' ) {
		if( usedRebootKey ) {
			return true;
		} 
	}
	if( event.down && !control_keys.alt && control_keys.control && control_keys.shift && event.char === 'T' ) {
		if( usedTabKey ) {
			console.log( "Let go of the T key?!" );
			return true;
		}
		 usedTabKey = true;
		return true; // used this key.
	}
	if( !event.down && !control_keys.alt && control_keys.shift && control_keys.control && event.char === 'T' ) {
		if( usedTabKey ) {
			return true;
		} 
	}

	if( !control_keys.shift && !control_keys.control && !event.down ) {
		//console.log( "last Key???" );
		if( usedRebootKey ) {
			//console.log( "Sent windows key ..." );
			usedRebootKey= false;
			setTimeout( ()=>{
				console.log( "Generate windows key" );
				keyboard.send( [{ key:91, code:91, down:true, extended:true}])
				setTimeout( ()=>{
					keyboard.send( [{key:91, code:91, down:false, extended:true}] );
				}, 50 );
			} );
		}

		if( usedTabKey ) {
			//console.log( "Sent windows key ..." );
			usedTabKey = false;
			setTimeout( ()=>{
				console.log( "Generate task key" );
				keyboard.send( [{ key:164, code:56, down:true, extended:false}, {key:9, code:15, down:true, extended:false} ])
				setTimeout( ()=>{
					keyboard.send( [{key:9, code:15, down:false, extended:false}, {key:164, code:56, down:false, extended:true}] );
				}, 50 );
			} );
		}

	}

}