
//const tls = require('tls');
//console.log(tls.getCiphers());
const vfs = require( "." );
//vfs.TLS.seed( )

var key = vfs.TLS.genkey( 1024, "password" );
console.log( key );

//var pubkey = vfs.TLS.pubkey( { key:key, password:"password" } );
//console.log( pubkey );

var cert = vfs.TLS.gencert( { country:"US", state:"NV", locality:"Las Vegas", org:"Freedom Collective", unit:"IT", commonName:"Root Cert", serial: 1001, key:key, expire: 7, password:"password" } );
console.log( cert );

var key2 = vfs.TLS.genkey( 1024, "pass2" );

//var pubkey2 = vfs.TLS.pubkey( { key:key2, password:"pass2" } );
//console.log( pubkey2 );

var cert2 = vfs.TLS.genreq( { country:"US", state:"NV", locality:"Las Vegas", org:"Freedom Collective", unit:"IT", commonName:"CA Cert", serial: 1002, key:key2, password:"pass2" } );
//console.log( cert2 );

//console.log( "sign request..." );
var signedCert2 = vfs.TLS.signreq( { request:cert2, signer:cert, key:key, serial: 1003, expire: 100, password:"password" } );
console.log( signedCert2 );


var key3 = vfs.TLS.genkey( 1024, "pass4" );

//var pubkey3 = vfs.TLS.pubkey( { key:key3, password:"pass4" } );
//console.log( pubkey3 );

//console.log( "genreq2" );
var cert3 = vfs.TLS.genreq( { country:"\0\0", state:"Conscious", locality:"C-316", org:"Yes", unit:"Super", commonName:"www.common.name", serial: 1004, key:key3, password:"pass4" } );
//console.log( cert3 );

//console.log( "sign request2..." );
var signedCert3 = vfs.TLS.signreq( { request:cert3, signer:signedCert2, key:key2, serial: 1005, expire: 100, password:"pass2" } );
console.log( signedCert3 );

//var pubkey4 = vfs.TLS.pubkey( {cert: signedCert3} );
//console.log( pubkey4 )

if( vfs.TLS.validate( {cert:signedCert3, chain:signedCert2+cert} ) )
	console.log( "Chain is valid." );
