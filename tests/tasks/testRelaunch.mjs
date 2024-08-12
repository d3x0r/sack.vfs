
import {sack} from "sack.vfs"
import os from "os";

let launches = 0;
sack.system.dumpMemory(true);
sack.system.logMemory( true );
sack.system.debugMemory( true );
let tries = 0;

if( os.platform() === "win32" ) {
function start() {
	launches++;
	new sack.Task( {bin:"cmd.exe", args:["/c", "echo hi"], end() {
        	start();
			tries++;
			if( tries > 10000 ) {
				sack.system.dumpMemory(true);
				tries = 0;
			}
        } } );
}
start();

}else {

function start() {
	launches++;
	const task = new sack.Task( {bin:"echo", args:["Echo Param"]
		, usePty : true
		, input(buf) {
			console.log( "Input:", buf.toString() );
		}
		, end() {
        			start();
				tries++;
				if( tries > 10000 ) {
					sack.system.dumpMemory(true);
					tries = 0;
				}
	        	} 
		} );
	//setTimeout( ()=>task.end(), 500 );
	task.write( "Data?" );
}

start()

}

