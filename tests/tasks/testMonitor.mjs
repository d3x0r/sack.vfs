import {sack} from "sack.vfs"

sack.Task.onEnd( Number( process.argv[2] ), ( exitCode )=>{
	if( exitCode === null ) 
		console.log( "NO such process." );
	else
		console.log( "Exit code:", exitCode );
	console.log( "Task Ended." );
//        process.exit(0);
} );
