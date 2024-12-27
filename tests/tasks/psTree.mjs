import {sack} from "sack.vfs"
const disk = sack.Volume();
const procs = sack.Task.getProcessList( sack.Task.processId() );

for( let proc of procs ) {
	show( proc, 0 );
}
function show( proc, level ) {
	console.log( Array(level*3+1).join(' ') + "proc:", proc );
	const parent = sack.Task.parentId( proc.id );
	console.log( "has parent?", parent );
	if( parent )
	show( sack.Task.getProcessList( parent ), level+1 );
	//const threads = disk.dir( `/proc/${proc.id}/task` );
	//const threadids = threads.reduce( (acc,thread)=>{if( thread.name[0] !== '.') acc.push( thread.name); return acc} , [] );
	//console.log( proc.id.toString().padStart( 8, ' ' ), proc.bin, proc.args.join(' ' ), "\n", "\t", threadids.join(',' ) );
}
