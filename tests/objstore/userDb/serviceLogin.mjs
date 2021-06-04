
const sack = (await Import( "sack.vfs" )).sack;

open( { protocol: "userDatabaseClient"
      , server : "ws://localhost:8089/"
    } );

function open( opts ) {
    	const protocol = opts?.protocol || "protocol";
	const server = opts.server;
        console.log( "connect with is:", server, protocol );
	var client = sack.WebSocket.Client( server, protocol, { perMessageDeflate: false } );
        console.log( "client result:", client );
	
	client.onmessage = function( msg ) {
	        	console.log( "message Received:", msg );
                	//this.close();
        };
	client.onopen = function ()  {
		console.log( "Connected", serviceId );
		console.log( "ws: ", this );
		this.on( "message", function( msg ) {
	        	console.log( "message Received:", msg );
                	//this.close();
        	} );
		this.on( "close", function( msg ) {
        		console.log( "opened connection closed" );
        	        //setTimeout( ()=> {console.log( "waited" )}, 3000 )
	        } );
		//client.send( "Connected!" );
		//client.send( msg );
	       	//client.send( msgtext );
                //client.send( "." );
	} ;

	client.on( "close", function( msg ) {
      		console.log( "unopened connection closed" );
	} );

} 




function handleMessage( ws, msg ) {
	if( msg.op === "addMethod" ) {
		
	}
}


// return
return "this usually isn't legal?";
