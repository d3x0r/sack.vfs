
import {sack} from "sack.vfs"

export function findTask( task ) {
	let b1 = '';
	let b2 = '';
	let resolve = null;
	sack.Task( { bin: "schtasks.exe",
	           args: ["/query", "/tn", (task.service || "@d3x0r\\sack.vfs\\")+task.name
	           ]
			   , input(buffer ) {
				b1 += buffer;
			   }
	           , end() {
					console.log( "B:", b1 );
					if( b1.includes( "cannot find the path specified." ) 
						|| b1.includes( "cannot find the file specified." ) ) {
						resolve( false );
					}else {
						resolve( true );
					}
	           }
	} )
	return new Promise( res=>{resolve=res} );
	//sack.Task( { bin: "sc", args, } )
}

export function createTask( task ) {
	let resolve = null;
	sack.Task( { bin: "schtasks.exe", args: ["/create", "/tn", (task.service || "@d3x0r\\sack.vfs\\")+task.name, "/tr", task.bin, "/sc", "ONCE", "/st", "00:00", "/sd", "01/01/2000" ],
		input( buffer ) {
			console.log( "buffer:", buffer );
		}
	, end() {
		console.log( "Task created" );
		resolve( true );
	} } )
	return new Promise( res=>{resolve=res} );

}

export function triggerTask( task ) {
	let b1 = '';
	let resolve = null;
	const base_procs = sack.Task.getProcessList( task.bin );
	console.log( "Shouldn't there already be some?", base_procs );
	sack.Task( { bin: "schtasks.exe",
	           args: ["/run", "/tn", (task.service || "@d3x0r\\sack.vfs\\")+task.name
	           ]
			   , input(buffer ) {
				b1 += buffer;
			   }
	           , end() {
					const new_procs = sack.Task.getProcessList( task.bin );
					for( let o = 0; o < base_procs.length; o++ ) {
						for( let n = 0; n < new_procs.length; n++ ) {
							if( base_procs[o].id === new_procs[n].id ){
								base_procs.splice(o,1);
								new_procs.splice( n,1 );
								o--;
								break;
							}
						}
					}
					console.log( "Compare old and new?", base_procs, new_procs );
					console.log( "B:", b1 );
					resolve( true );
	           }
	} )
	return new Promise( res=>{resolve=res} );
}

export function termTask( task ) {
	let b1 = '';
	let resolve = null;
	sack.Task( { bin: "schtasks.exe",
	           args: ["/end", "/tn", (task.service || "@d3x0r\\sack.vfs\\")+task.name
	           ]
			   , input(buffer ) {
					b1 += buffer;
			   }
	           , end() {
					console.log( "B:", b1 );
					resolve( true );
	           }
	} )
	return new Promise( res=>{resolve=res} );
}

export function deleteTask( task ) {
	let b1 = '';
	let resolve = null;
	sack.Task( { bin: "schtasks.exe", args: ["/delete", "/tn", (task.service || "@d3x0r\\sack.vfs\\")+task.name, "/F" ],
			input( buffer ) {
				b1 += buffer;
			}
		, end() {
			console.log( "Task deleted?", b1 );
			resolve( true );
		} } )
	return new Promise( res=>{resolve=res} );
	//sack.Task( { bin: "sc", args, } )
}
