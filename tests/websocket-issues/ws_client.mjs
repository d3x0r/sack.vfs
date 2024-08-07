import {sack} from "sack.vfs"


let open = 0;
let opened_socks = 0;

function connect( id ) {
	const ws = sack.WebSocket.Client( "http://localhost:4321/"+id, "protocol" );
	open++;
	ws.onopen = opened;
	ws.onmessage = messaged;
	ws.onclose = closed;
	ws.onerror = errored;

	let nbuffer = 0;

	function opened( ) {
		//console.log( "connected.", id, ++opened_socks);
		ws.send( ''+nbuffer+','+id );
	}
	function messaged( msg ) {
		const arr = msg.split(',').map( Number );
		if( nbuffer < 4 )
			ws.send( '' + ( nbuffer+1 )+','+id );
		if( arr.length != 40000 ) console.log( "Fatal - buffer is wrong length:", arr.length );
		let errors= 0;
		for( let n = 0; errors < 17 && n < 40000; n++ ) {
			if( arr[n] != nbuffer )  {
				errors++;
				if( errors > 16 ) console.log( "More Errors....", id );
				console.log( "Fatal - buffer has wrong content: ", id, n, arr[n] );
			}
		}

		nbuffer++;
		//console.log( "messaged.", id, msg.length );
	}
	function closed(code,reason) {
		if( nbuffer < 4 ) console.log( "Closed, but expected more data...", id );
		//console.log( "closed:", id, "code", code, reason, "remaining:", open );
		open--;
	}
	function errored( a ) {
		console.log( "Socket errored?", a );
	}
}

for( let n = 0; n < 64; n++ )
	connect( n )
