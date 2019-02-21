
var sack = require( ".." );
const path = require( "path" );
var vol = sack.Volume();
//console.log( sack.TLS );
var disk = sack.Volume();

//---------------------------------------------
const baseSerial = 1051;

var keys = [ sack.TLS.genkey( 2048 ), sack.TLS.genkey( 2048 ), sack.TLS.genkey( 2048, "password" ) ];
var certRoot = sack.TLS.gencert( { key:keys[0]
	, country:"US"
	, state:"NV"
	, locality:"Las Vegas"
	, org:"Freedom Collective", unit:"Tests", name:"Root Cert", serial: baseSerial }  )

//vol.write( "testcert.pem", certRoot );

var signer = ( sack.TLS.signreq( { 
	request: sack.TLS.genreq( { key:keys[1]
		, country:"US", state:"NV", locality:"Las Vegas"
		, org:"Freedom Collective", unit:"Tests"
		, name:"CA Cert", serial: baseSerial+1 } )
	, signer: certRoot, serial: baseSerial+2, key:keys[0] } ) );

var cert = sack.TLS.signreq( { 
	request: sack.TLS.genreq( { key:keys[2], password:"password"
		, country:"US", state:"NV", locality:"Las Vegas"
		, org:"Freedom Collective", unit:"Tests", name:"localhost", serial: baseSerial + 3
		, subject: { DNS:["localhost","*.localhost"], IP:["127.0.0.1"] } 
	} )
	, signer: signer, serial: baseSerial+4, key:keys[1] } );


//console.log( certRoot+signer+cert );

//---------------------------------------------

var serverOpts;
var server = sack.WebSocket.Server( serverOpts = { port: Number(process.argv[2])||8085, cert : cert+signer+certRoot, key: keys[2], passphrase:"password" } )

console.log( "serving on", serverOpts.port );

server.onerror( function( failedConnection ) {
	console.log( "Failed SSL ConnectioN:", failedConnection );
        // ( failedConnection.remoteFamily, failedConnection.remoteAddress );
} );

server.onrequest( function( req, res ) {
	var ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
		 req.connection.remoteAddress ||
		 req.socket.remoteAddress ||
		 req.connection.socket.remoteAddress;
	//ws.clientAddress = ip;

	//console.log( "Received request:", req );
	if( req.url === "/" ) req.url = "/index.html";
	var filePath = "." + unescape(req.url);
	var extname = path.extname(filePath);
	var contentType = 'text/html';
	console.log( ":", extname, filePath )
	switch (extname) {
		  case '.js':
		  case '.mjs':
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
	if( disk.exists( filePath ) ) {
		res.writeHead(200, { 'Content-Type': contentType });
		console.log( "Read:", "." + req.url );
		res.end( disk.read( filePath ) );
	} else {
		console.log( "Failed request: ", req );
		res.writeHead( 404 );
		res.end( "<HTML><HEAD>404</HEAD><BODY>404</BODY></HTML>");
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
