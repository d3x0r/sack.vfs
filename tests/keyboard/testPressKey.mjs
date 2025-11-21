

import {sack} from "sack.vfs"

// test keboard input (get values for keys to use)
const kb = sack.Keyboard( (key)=>{
	//console.log( "Got key:", key );
    return false; // return true to consume
} );

function k1() {
kb.send( [{key:68,code:32,down:true,extended:false} ] ) //` to send key events.
kb.send( [{key:68,code:32,down:false,extended:false} ] ) //` to send key events.
setTimeout( k2, 8 );
}

function k2() {
kb.send( [{key:65,code:30,down:true,extended:false} ] ) //` to send key events.
kb.send( [{key:65,code:30,down:false,extended:false} ] ) //` to send key events.
setTimeout( k1, 8 );
}

k1()
