import {sack} from "sack.vfs"

const do_log = true;
const server = sack.WebSocket.Server( {port:4321} );
console.log( "Serving on 4321" );
server.onaccept = accepted;
server.onconnect = connected;

function accepted( ws ) {
	do_log && console.log( "Accepted websock?", ws.url );
	//this.accept( ws.connection.headers['Sec-WebSocket-Protocol']+ws.url );
	this.accept( );
}	

function connected( ws ) {
	do_log && console.log( "New websock?", ws.url );
	ws.onmessage = messaged;
	ws.onclose = closed;

	let nbuffer = -1;
	let buffer = '';

	function makebuffer() {
		const arr = [];
		for( let n = 0; n < 100; n++ ) arr.push( nbuffer );
		nbuffer++;
		buffer = arr.join(',');
		return buffer;
	}
	console.log( "Sending a buffer from server first" );
	ws.send( makebuffer() );
//	setTimeout( ()=>{ ws.close( 1002, "done." ); }, 100 );
	function messaged( msg ) {
		const parts = msg.split(',')
		do_log && console.log( "got message", msg );
		nbuffer = Number(parts[0]);
		if( nbuffer == 4 ) 
			setTimeout( ()=>{ 
				do_log && console.log( "issuing close: ", parts[1]);
				ws.close( 1002, "done:" + parts[1]  ); }, 100 );
		makebuffer();
		ws.send( buffer );
	}

	function closed( code, reason ) {
		do_log && console.log( "Socket closed:", code, reason );
	}
}

