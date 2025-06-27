import {sack} from "sack.vfs"

const do_log = false

let open = 0;
let opened_socks = 0;
let outstanding_tick = 0;
const outstanding = [];

function connect( id ) {
	const ws = sack.WebSocket.Client( "http://localhost:4321/"+id, "protocol" );
	open++;
	let nbuffer = 0;
	const info = {id,open, nbuffer:-1}
	outstanding.push( info );
	ws.onopen = opened;
	ws.onmessage = messaged;
	ws.onclose = closed;
	ws.onerror = errored;


	function opened( ) {
//		do_log && 
console.log( "connected.", id, ++opened_socks);
		info.nbuffer = 0;
		ws.send( ''+nbuffer+','+id );
	}
	function messaged( msg ) {
		const arr = msg.split(',').map( Number );
		if( arr.length != 40000 ) console.log( "Fatal - buffer is wrong length:", arr.length );
		if( arr[0] === -1 ) nbuffer = -1;
		let errors= 0;
		for( let n = 0; errors < 17 && n < 40000; n++ ) {
			if( arr[n] != nbuffer )  {
				errors++;
				if( errors > 16 ) console.log( "More Errors....", id );
				console.log( "Fatal - buffer has wrong content: ", id, n, arr[n] );
			}
		}

		if( arr[0] === -1 ) nbuffer = 0;
		else {
			if( nbuffer < 4 )
				ws.send( '' + ( nbuffer+1 )+','+id );
			nbuffer++;
			info.nbuffer = nbuffer;
			do_log && console.log( "messaged.", id, msg.length );
		}
	}
	function closed(code,reason) {
		const index = outstanding.findIndex( (o)=>o.id === id );
		if( index >= 0 ) outstanding.splice( index, 1 );
		if( outstanding_tick ) {
			clearTimeout( outstanding_tick );
		}
		if( outstanding.length )
			outstanding_tick = setTimeout( ()=>{
				console.log( outstanding );
			}, 500 )
		else 
			console.log( "last outstanding? why are we waiting?")
		if( nbuffer < 4 ) console.log( "Closed, but expected more data...", id, nbuffer );
		do_log && console.log( "closed:", id, "code", code, reason, "remaining:", open );
		open--;
	}
	function errored( a ) {
		console.log( "Socket errored?", a );
	}
}

for( let n = 0; n < 4; n++ )
	connect( n )
