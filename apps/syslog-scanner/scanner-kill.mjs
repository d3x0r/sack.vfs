
import {sack} from "sack.vfs"

const nodeTasks = sack.Task.getProcessList( "node" );
for( let task of nodeTasks ) {	
	if( task.args[0].endsWith( "scanner.mjs" ) ){
		//console.log( "Found task:", task );
		sack.Task.kill( task.id );
	}
}
