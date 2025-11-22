import {sack} from "sack-gui"

//console.log( "SACK?", sack );

process.env.PATH=process.env.PATH+";M:\\javascript\\sack-gui\\build\\RelWithDebInfo\\share\\SACK\\plugins"

sack.Dekware.start( "" );

setTimeout( ()=>{console.log( "tick" )}, 100000 );


