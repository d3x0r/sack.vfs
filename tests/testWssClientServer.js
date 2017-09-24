
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


var server = sack.WebSocket.Server( { port: 8080, cert : cert, ca : signer, key: keys[2], passphrase:"password" } )

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

server.onaccept( function ( ws ) {
	console.log( "connected:", ws);
	console.log( "Connection received with : ", ws.headers['Sec-WebSocket-Protocol'], " path:", ws.url );
        if( process.argv[2] == "1" )
		this.reject();
        else
		this.accept();
		//this.accept( protocols );
} );

server.onconnect( function (ws) {
	console.log( "Connect:", ws );
	console.log( "Connect:", this );
	
	ws.onmessage( function( msg ) {
        	console.log( "Received data:", msg );
                ws.send( msg );
		//ws.close();
        } );
	ws.onclose( function() {
        	console.log( "Remote closed" );
        } );
} );


var clientOpts = { 
	ca : certRoot,
	key : sack.TLS.genkey( 1024, "password" ),
	passphrase : "password"
} 

var client = sack.WebSocket.Client( "wss://localhost:8080", clientOpts );
client.on( "message", (msg)=>{ console.log( "Got back:", msg ) } );
client.on( "open", ()=>{ client.send( "Echo This." ) } );
client.on( "error", (error)=>{ console.log( "Error:", err ) } );

var client2 = sack.WebSocket.Client( "wss://localhost:8080/", ["chat", "present"], clientOpts );
client2.on( "message", (msg)=>{ console.log( "Got back:", msg ) } );
client2.on( "open", function(){ console.log( "This?", this ); this.send( "Echo This." ) } );
client2.on( "error", (error)=>{ console.log( "Error:", err ) } );
