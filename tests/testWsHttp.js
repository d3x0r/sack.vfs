

var sack = require( ".." );
var server = sack.WebSocket.Server( { port: 8080 } )

console.log( "serving on 8080" );


server.onrequest( function( req, res ) {
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
