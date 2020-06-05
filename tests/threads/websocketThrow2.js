
const sack = require("../.." );
const server = require( "../testWsHttp.js" );
const wt = require('worker_threads');

var thread = new wt.Worker( "./websocketListenThread.js", { stdout:true} )
thread.stdout.pipe( process.stdout );


server.open( {port:9912}, async (ws)=>{
	console.log( "Accepted a connection?" ); 
	ws.block();
	
	return new Promise( (res,rej)=>{
		setTimeout( ()=>{
			ws.post( "Accept A Socket" );
			ws.on( "message", (data)=>{
				console.log( "Got message in thread:", data );
			} );
			res( true );
		}, 1000 );
	} );
	
} )


const serverAddr = `ws://localhost:9912/`;

function open() {
	console.log( "connect to ", serverAddr );
	
	var client = sack.WebSocket.Client( serverAddr, ["a","b"], { perMessageDeflate: false } );

	client.on( "open", function ( ws )  {
		this.on( "message", function( msg ) {
	        	console.log( "message Received:", msg );
        	} );
		this.on( "close", function( msg ) {
        		console.log( "opened connection closed" );
	        } );
		this.send( "test" );
	} );

	client.on( "close", function( msg ) {
      		console.log( "unopened connection closed" );
	} );

} 

setTimeout( open, 500 );
