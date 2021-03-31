import {sack} from "sack.vfs"


{
	var serverOpts = {port:Number(process.argv[2])||8080} ;
	var server = sack.WebSocket.Server( serverOpts )
	console.log( "serving on " + serverOpts.port );


	server.onrequest( function( req, res ) {
        	console.log( "Received Request:", req );
                res.end( 200 );
        } )
}

