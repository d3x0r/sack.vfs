


import {sack} from "sack.vfs"


function openServer( opts, cb )
{
	var serverOpts = {getMAC:true, port:Number(process.argv[2])||8080} ;
	var server = sack.WebSocket.Server( serverOpts )
	console.log( "serving on " + serverOpts.port );

	server.onrequest = function( req, res ) {
	        console.log( "handle Request:", req );
		res.writeHead( 404 );
		res.end( "<HTML><HEAD>404</HEAD><BODY>404</BODY></HTML>");
	};

}

openServer();
