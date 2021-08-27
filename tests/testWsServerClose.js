
var sack = require( ".." );
console.log( "Open one server..." );
var server = sack.WebSocket.Server( { port: 8080 } )
setupServer( server );
server.close();
console.log( "Closed, should be able to open..." );
server = sack.WebSocket.Server( { port: 8080 } );
console.log( "Duplicate (should fail)" );
server = sack.WebSocket.Server( { port: 8080 } );
console.log( "result: ", server );
setupServer( server );


function setupServer( server ) {
server.onrequest( function( req, res ) {

	var ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
		 req.connection.remoteAddress ||
		 req.socket.remoteAddress ||
		 req.connection.socket.remoteAddress;
	//ws.clientAddress = ip;

	console.log( "Received request:", req );
	if( req.url.endsWith( ".html" ) || req.url == "/" ) {
		res.writeHead( 200 );
		res.end( "<HTML><BODY>Success.</BODY></HTML>" );
	} else {
		res.writeHead( 404 );
		res.end();
	}
} );

server.onaccept( function ( protocols, resource ) {
	console.log( "Connection received with : ", protocols, " path:", resource );
        if( process.argv[2] == "1" )
		this.reject();
        else
		this.accept();
		//this.accept( protocols );
} );

server.onconnect( function (ws) {
	//console.log( "Connect:", ws );

	ws.onmessage( function( msg ) {
        	console.log( "Received data:", msg );
                ws.send( msg );
		//ws.close();
        } );
	ws.onclose( function() {
        	console.log( "Remote closed" );
        } );
} );
}

