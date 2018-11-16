
var sack = require( ".." );
var vol = sack.Volume();
//console.log( sack.TLS );

const baseSerial = 1051;

var keys = [ sack.TLS.genkey( 2048 ), sack.TLS.genkey( 2048 ), sack.TLS.genkey( 2048, "password" ) ];
var certRoot = sack.TLS.gencert( { key:keys[0]
	, country:"US"
	, state:"NV"
	, locality:"Las Vegas"
	, org:"Freedom Collective", unit:"Tests", name:"Root Cert", serial: baseSerial }  )

vol.write( "testcert.pem", certRoot );

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


console.log( certRoot+signer+cert );
var server = sack.WebSocket.Server( { port: 8085, cert : cert+signer+certRoot, key: keys[2], passphrase:"password" } )

console.log( "serving on 8085" );

server.onerror( function( failedConnection ) {
	console.log( "Failed SSL ConnectioN:", failedConnection );
        // ( failedConnection.remoteFamily, failedConnection.remoteAddress );
} );

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
