
var sack = require( ".." );

var keys = [ sack.TLS.genkey( 1024 ), sack.TLS.genkey( 1024 ), sack.TLS.genkey( 1024, "password" ) ];
var certRoot = sack.TLS.gencert( { key:keys[0]
	, country:"US"
	, state:"NV"
	, locality:"Las Vegas"
	, org:"Freedom Collective", unit:"Tests", name:"Root Cert", serial: 1001 }  )
console.log( sack.TLS );
var signer = ( sack.TLS.signreq( { 
	request: sack.TLS.genreq( { key:keys[1]
		, country:"US", state:"NV", locality:"Las Vegas"
		, org:"Freedom Collective", unit:"Tests"
		, name:"CA Cert", serial: 1002 } )
	, signer: certRoot, serial: 1003, key:keys[0] } ) );

var cert = sack.TLS.signreq( { 
	request: sack.TLS.genreq( { key:keys[2], password:"password"
		, country:"US", state:"NV", locality:"Las Vegas"
		, org:"Freedom Collective", unit:"Tests", name:"Cert", serial: 1004
		//, subject: { DNS:dnsNames, IP:internal?config.run.internal_addresses:config.run.addresses } 
	} )
	, signer: signer, serial: 1005, key:keys[1] } );

//console.log(  cert+signer+certRoot );
function open() {
	var server = sack.WebSocket.Server( { port: 8080, ca : cert+signer+certRoot, key: keys[2], passphrase:"password" } )

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
setTimeout( ()=>{
	console.log( "server:", server )
	server.close() 
	//setTimeout( ()=>{
		open(); 
	//}, 2000 );
}, 2000 );
	return server;
} 

var server = open();

