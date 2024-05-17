
import {sack} from "sack.vfs"
import {isTopLevel} from "sack.vfs/isTopLevel"

//WMIC path win32_process where "name like '%cmd%'" get Caption,Processid,Commandline
let done = null;


export function findProcess( program, args ) {
	const buffer = [];
	const buffer2 = [];
	
   //console.log( "plat?", process.platform );
	const test = RegExp(args );
    if( process.platform === "win32" ) {
	const taskList = sack.Task.getProcessList( program );
	//console.log( "Got:", program, taskList );
	const waits = [];
	for( let task of taskList ) {
		if( test.test( task.args.join(' ') ) ) {
			//const parts = line.split(/\s\s+/ );
			 //console.log( "Found:", line );
			waits.push( killTask( task ) );
		}
	}
	Promise.all( waits ).then( done );

	function killTask( info ) {
		const p = new Promise( (res,rej)=>{
			//console.log( "Found:", info );
			sack.Task.kill( info.id, 0xd1e );
		} );
		return p;
	}
	

		//console.log( "Task?", task, opts.args.join(' ' ) );
	    //WMIC path win32_process where "name like '%cmd%'" get Caption,Processid,Commandline
    }
	else
		done();
}

setTimeout( ()=>{}, 1000 );

export function go() {
	const p = new Promise( (res,rej)=>{
		done = res;
	} );

    findProcess( "chrome.exe", "kiosk.*user-data-dir" );
    findProcess( "nw.exe", "kiosk.*user-data-dir" );
	return p;
}

if( isTopLevel( import.meta.url ) ) {
	go();
}

