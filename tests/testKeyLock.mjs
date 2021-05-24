
import {sack} from "sack.vfs";

const keyboard = sack.Keyboard();

keyboard.lock( true );

console.log( "Can you type ANYTHIGN right now?" );

// doesn't clear for garbage collection
keyboard.onKey( ( key )=>{
	console.log( "key event got key:", key );
        return false;
} );

setTimeout( ()=>{
	keyboard.lock( false );
	console.log( "What about Now?" );
        // don't get this callback, but should now garbage collect.
        keyboard.onKey( null, true );
}, 5000 );