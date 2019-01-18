
var vfs = require( '..' );
const WS = vfs.WebSocket.readyStates;
var server = vfs.WebSocket.Server( { port:8080
	//, perMessageDeflate:false
	, perMessageDeflateAllow:true
	}
);
server.on( "connect", function(ws){
	//console.log( "connected." );
        ws.on( "message", function(msg) {
        	//if( typeof msg == "String" )
        	//console.log( "got message", msg );
try{
		if( ws.readyState == WS.OPEN ) 
                	ws.send( msg );
                else console.log( "Had to skip a write, not open" );
}catch(err){ console.log( "And it still failed becasue it clsoed" ) }
        } );
        ws.on( "close", function() {
        } );
        ws.on( "error", function(err,code) {
        	console.log( "error event (probably not)" );
        } );
} );
