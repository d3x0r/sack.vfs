
import {sack} from "sack.vfs";

process.on( "SIGPIPE", ()=>{
	console.log( "SIGPIPE caught by JS and ignored..." );
} )

function start( n ) {
	const task = new sack.Task( { bin:"cat"
	                            , input( buf ) {
	                                //console.log( "Got Buffer:", buf );
	                                if( buf.includes( "asdf" ) )
	                                    task.end();
	                            }
	                            , end() {
					start( n+1 );
	                            }
	                            } );
	setTimeout( ()=>{
		task.send( "extra send" );
	}, 2 );
	task.send( n );
	task.send( "asdf" );
	//setTimeout( task.end.bind(task), 1 );
}

start(0);
start(1000 );
start(2000);
start( 3000 );
