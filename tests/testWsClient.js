

var sack = require( ".." );

var server = `ws://${process.argv[2]?process.argv[2]:"192.168.173.13"}:3000/`;
console.log( "connect to ", server );
//var client = sack.WebSocket.Client( "ws://DESKTOP-EMFQ8QQ:8080/", ["a","b"], { perMessageDeflate: true } );

var msg = new Buffer(10);
for( var n = 0; n < 10; n++ )
	msg[n] = 1;
var msgtext = msg.toString('utf8');
var connects = 0;

function open() {
	var client = sack.WebSocket.Client( server, ["a","b"], { perMessageDeflate: false } );
	client.on( "open", function ()  {
		console.log( "Connected" );
		console.log( "ws: ", this );
			this.on( "message", function( msg ) {
	        	console.log( "message Received:", msg );
                	this.close();
        	} );
		this.on( "close", function( msg ) {
        		console.log( "opened connection closed" );
        	        //setTimeout( ()=> {console.log( "waited" )}, 3000 )
	        } );
		//client.send( "Connected!" );
		//client.send( msg );
		client.send( msgtext );
                client.send( "." );
	} );

	client.on( "close", function( msg ) {
      		console.log( "unopened connection closed" );
	} );

} 
open()