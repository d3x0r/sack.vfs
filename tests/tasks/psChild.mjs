import {sack} from "sack.vfs"
const disk = sack.Volume();
const pid = sack.Task.processId();
const procs = sack.Task.getProcessList(  );
const self = indexProcs( procs );

function indexProcs( procs ) {
	let self = null;
	for( let proc of procs ) {
		proc.pid = sack.Task.parentId( proc.id );
		proc.title = sack.Task.getTitle( proc.id );
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

for( let proc of procs ) {
	console.log( proc );
}