
import {sack} from "sack.vfs"
const disk = sack.Volume();
const procs = sack.Task.getProcessList( "node" );
const nodeProcs = [];
for( let proc of procs ) {
	if( proc.bin.endsWith( "bin/node" ) )
		nodeProcs.push( proc );
		
}
for( let proc of nodeProcs ) {
	sack.Task.kill( proc.id );
}
console.log( "procs:", nodeProcs );
