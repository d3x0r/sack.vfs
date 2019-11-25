
var sack = require("../.." );

console.log( "Thread started, and is going to accept a socket." );
sack.WebSocket.Thread.accept( "Accept A Socket", (id,ws)=>{
	console.log( "Got:", id, ws );
} );
console.log( "Okay... wait..." );
