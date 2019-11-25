
var sack = require("../.." );

console.log( "Thread started, and is going to accept a socket." );
sack.WebSocket.Thread.accept( "Accept A Socket", (id,ws)=>{
	console.log( "Got:", id, ws );
	ws.on( "message", (msg)=>{
		console.log( "Server in thread got:", msg );
	} );
	ws.resume();
} );
console.log( "Okay... wait..." );
