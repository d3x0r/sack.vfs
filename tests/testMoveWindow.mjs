
import {sack} from "sack.vfs"

const task = sack.Task( {bin:"cmd.exe", args:[ ], newConsole:true, end() {} });

function move(n) {
	task.moveWindow( {x:n*100, y:n*100, width: 500, height:350, timeout: 2500, cb( yesno ) {
		console.log( "Moved?:", yesno );
		if( n > 5 ) n = -1;
		setTimeout( ()=>move(n+1), 500 );
	} });
}
move(1);

