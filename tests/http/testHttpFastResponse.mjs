
import {sack} from "sack.vfs";



	var server = sack.WebSocket.Server( {port:Number(process.argv[2])||8080} )

	server.onrequest = function( req, res ) {
			const headers = { 'Content-Type': "text/javascript", 'Access-Control-Allow-Origin' : req.connection.headers.Origin||"*" };
			res.writeHead(200, headers );
			res.end( '"abc"' );
	 	};
