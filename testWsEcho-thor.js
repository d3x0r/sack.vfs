
var vfs = require( '.' );
console.log( "vfs:", vfs );
var server = vfs.WebSocket.Server( { port:8080 } );
server.on( "connect", function(ws){
	//console.log( "connected." );
        ws.on( "message", function(msg) {
        	//if( typeof msg == "String" )
        	//console.log( "got message", msg );
                ws.send( msg );
        } );
        ws.on( "close", function() {
        	//console.log( "client disconnected" );
        } );
        ws.on( "error", function(err,code) {
        	console.log( "error event (probably not)" );
        } );
} );
