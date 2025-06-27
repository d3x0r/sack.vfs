
import {sack} from "sack.vfs"
if( !sack.system.isElevated() ) {
	const args =  process.argv.slice(1).map( arg=>arg=arg.includes(' ' )?'""'+arg+'""':arg ) ;
	if( !process.argv[2] )
		sack.Task( { bin:process.argv[0] , args: [args, "noretry"]
			, admin : true
			, end() { process.exit(0)} } );
	else console.log( "Already running?", process.argv );
}
else  {
	console.log( "Is Elevated?", sack.system.isElevated() );
	setTimeout( ()=>{}, 5000 );
}



