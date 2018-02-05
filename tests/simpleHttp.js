
const path = require( 'path' );

const sack = require( ".." );
const disk = sack.Volume();
const server = sack.WebSocket.Server( { port: 8080 } )

console.log( "serving on 8080" );


server.onrequest( function( req, res ) {
	var ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
		 req.connection.remoteAddress ||
		 req.socket.remoteAddress ||
		 req.connection.socket.remoteAddress;
	//ws.clientAddress = ip;

	var extname = path.extname(req.url);
	var contentType = 'text/html';
	switch (extname) {
		  case '.js':
			  contentType = 'text/javascript';
			  break;
		  case '.css':
			  contentType = 'text/css';
			  break;
		  case '.json':
			  contentType = 'application/json';
			  break;
		  case '.png':
			  contentType = 'image/png';
			  break;
		  case '.jpg':
			  contentType = 'image/jpg';
			  break;
		  case '.wav':
			  contentType = 'audio/wav';
			  break;
	}

	console.log( "Received request:", req );
	if( disk.exists( req.url ) ) {
		var data = disk.readFile( req.url );        	
		res.writeHead( 200, { 'Content-Type': contentType } );
		res.end( data );
        }
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
