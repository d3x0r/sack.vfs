import {sack} from "sack.vfs"

const server = sack.WebSocket.Server( {port:4321} );
server.onconnect = connected;


function connected( ws ) {
	//console.log( "New websock?", ws );
	ws.onmessage = messaged;
	ws.onclose = closed;

let nbuffer = 0;
let buffer = '';

function makebuffer() {
	const arr = [];
	for( let n = 0; n < 40000; n++ ) arr.push( nbuffer );
	nbuffer++;
	buffer = arr.join(',');
}

//	setTimeout( ()=>{ ws.close( 1002, "done." ); }, 100 );
	function messaged( msg ) {
		console.log( "got message", msg );
		nbuffer = Number(msg);
		if( nbuffer == 4 )
			setTimeout( ()=>{ ws.close( 1002, "done." ); }, 100 );
		makebuffer();
		ws.send( buffer );
	}

	function closed( code, reason ) {
		console.log( "Socket closed:", code, reason );
	}
}

