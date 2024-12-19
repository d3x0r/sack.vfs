import {sack} from "sack.vfs"
const disk = sack.Volume();
const pid = sack.Task.processId();
const procs = sack.Task.getProcessList(  );
const self = indexProcs( procs );

function indexProcs( procs ) {
	let self = null;
	for( let proc of procs ) {
		proc.pid = sack.Task.parentId( proc.id );
		if( proc.pid )
			proc.parent = procs.find( (test)=>test.id === proc.pid );
		if( proc.parent )
			if( !proc.parent.children ) proc.parent.children = [proc];
			else proc.parent.children.push( proc );
		if( proc.id == pid ) 
			self = proc;
	}

	return self;
}

console.log( "Self:", self );
console.log( "Self:", self.parent.parent.children );
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
