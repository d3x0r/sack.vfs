
import {sack} from "sack.vfs";

const keyboard = sack.Keyboard();

keyboard.lock( true );

console.log( "Can you type ANYTHIGN right now?" );

setTimeout( ()=>{
	keyboard.lock( false );
	console.log( "What about Now?" );
        keyboard.onKey( null );
}, 5000 );