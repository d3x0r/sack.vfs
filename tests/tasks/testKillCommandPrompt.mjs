

import {sack} from "sack.vfs"

const cmd = new sack.Task( {bin:"cmd.exe", args:["/k", "pause" ], newConsole:true,newGroup:true, noKill:true } );
 new sack.Task( {bin:"cmd.exe", args:["/k", "pause" ], newConsole:true,newGroup:true, noKill:true } );
 new sack.Task( {bin:"cmd.exe", args:["/k", "pause" ], newConsole:true,newGroup:true, noKill:true } );
 new sack.Task( {bin:"cmd.exe", args:["/k", "pause" ], newConsole:true,newGroup:true, noKill:true } );

setTimeout( ()=>{
	const procs = sack.Task.getProcessList( "cmd.exe" );
	for( let proc of procs ) {
		if( proc.args[0] === '/k' && proc.args[1] === "pause" ) {
			console.log( "Proc id is ?", proc );
			sack.Task.stop( proc.id, 0 );  // use ctrl-c  (SIGINT)
			//sack.Task.stop( proc.id, 1 );  // use Break (SIGHUP)
			//sack.Task.stop( proc.id, 2, "cmd" ); // use signal  (no support? SIGUSR?)
			//sack.Task.kill( proc.id );
		}
	}
	
}, 1000 );




