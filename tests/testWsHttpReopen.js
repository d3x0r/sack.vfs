

var sack = require( ".." );
var server 

function open() {
server = sack.WebSocket.Server( { port: 8081 } )

console.log( "serving on 8080" );

var script = "<SCRIPT> var ws = new WebSocket( 'ws://localhost:8081' ); </SCRIPT>";

server.onrequest( function( req, res ) {

	var ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
		 req.connection.remoteAddress ||
		 req.socket.remoteAddress ||
		 req.connection.socket.remoteAddress;
	//ws.clientAddress = ip;

	console.log( "Received request:", req );
	if( req.url.endsWith( ".html" ) || req.url == "/" ) {
		res.writeHead( 200 );
		res.end( `<HTML><BODY>Success.</BODY>${script}</HTML>` );
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


function reopen() {
	if( server )
		server.close();
	open();
	setTimeout( reopen, 2000 );
}
reopen();
