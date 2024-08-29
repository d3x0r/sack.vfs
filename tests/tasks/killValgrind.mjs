
import {sack} from "sack.vfs"
const disk = sack.Volume();
const procs = sack.Task.getProcessList( "valgrind" );
const nodeProcs = [];
for( let proc of procs ) {
		nodeProcs.push( proc );
		
}
for( let proc of nodeProcs ) {
	sack.Task.kill( proc.id );
}
console.log( "procs:", nodeProcs );
