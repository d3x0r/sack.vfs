import {sack} from "sack.vfs"

sack.Keyboard( (key)=>{
	console.log( "Got key:", key );
        return false; // return true to consume
} );