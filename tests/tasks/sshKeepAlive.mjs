import {sack} from "sack.vfs"

start()
function start() {
	let lastSend = Date.now();
	let sent = false;
	let lastRecv = 0;
	const task = sack.Task( {bin:"ssh", args:[ "-tt", "-R", "8080:localhost:80", "-L4444:localhost:80", "jcoronel17@sideplayr.com" ]
			, input( buf ){
				console.log( "Stdin:", buf );
				lastRecv = Date.now();
				sent = false;
			}
			, errorInput( buf ){
				console.log( "Stderr:", buf );
			}
			, end() {
				console.log( "task Ended... restarting" );
				start();
			}
		}
			);
	const keepAlive = ()=>{
		if( !task.isRunning() ) return;

		const now = Date.now();
		//console.log( "Test:", lastSend, ((now - (1*60*1000)) > lastSend )  );
		if( !lastSend || ((now - (1*60*1000)) > lastSend ) ) {
			//console.log( "Sending ','" );
			task.write( "." );
			lastRecv = lastSend = now;
			sent = true;
		}
		if( sent && ( lastRecv < (now - 5000 ) ) ) {
			console.log( "Connection is stalled...", now - lastRecv );
			task.end(); return;
		}
		setTimeout( keepAlive, 500 );
	}
	setTimeout( keepAlive, 500 );
}
