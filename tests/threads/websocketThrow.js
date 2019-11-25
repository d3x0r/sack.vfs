
const sack = require("../.." );
const server = require( "../testWsHttp.js" );
const wt = require('worker_threads');

var thread = new wt.Worker( "./websocketListenThread.js", { stdout:true} )
thread.stdout.pipe( process.stdout );


server.open( {port:9912}, (ws)=>{
	console.log( "Accepted a connection?" ); 
	
	ws.post( "Accept A Socket" );
} )


const serverAddr = `ws://localhost:9912/`;

function open() {
	console.log( "connect to ", serverAddr );
	
	var client = sack.WebSocket.Client( serverAddr, ["a","b"], { perMessageDeflate: false } );

	client.on( "open", function ()  {
		this.on( "message", function( msg ) {
	        	console.log( "message Received:", msg );
        	} );
		this.on( "close", function( msg ) {
        		console.log( "opened connection closed" );
	        } );
	} );

	client.on( "close", function( msg ) {
      		console.log( "unopened connection closed" );
	} );

} 

setTimeout( open, 500 );
