
require( ".." )((vfs)=>{
//vfs.TLS.seed( )



var key = vfs.TLS.genkey( 1024, "password" );
var cert = vfs.TLS.gencert( { country:"US", state:"NV", locality:"Las Vegas", org:"Freedom Collective", unit:"IT", name:"Root Cert", serial: 1001, key:key, expire: 7, password:"password" } );
//var cert = vfs.TLS.gencert( { country:"US", state:"NV", locality:"Las Vegas", org:"Freedom Collective", unit:"IT", name:"Root Cert", serial: 1001, key:key, expire: 7, password:"password" } );




var key2 = vfs.TLS.genkey( 1024, "pass2" );
var cert2 = vfs.TLS.genreq( { country:"US", state:"NV", locality:"Las Vegas", org:"Freedom Collective", unit:"IT", name:"CA Cert", key:key2, password:"pass2" } );
var signedCert2 = vfs.TLS.signreq( { request:cert2, signer:cert, key:key, serial: 1003, expire: 100, password:"password" } );


var key3 = vfs.TLS.genkey( 1024, "pass4" );
var cert3 = vfs.TLS.genreq( { country:"\0\0", state:"Conscious", locality:"C-316", org:"Yes", unit:"Super"
	, name:"www.common.name"
	, key:key3, password:"pass4"
	, subject:{ DNS:["localhost"], IP:["127.0.0.1","192.168.173.13", "2001:db8::1"] } } );

var signedCert3 = vfs.TLS.signreq( { request:cert3, signer:signedCert2, key:key2, serial: 1005, expire: 100, password:"pass2" } );

console.log( signedCert3 );
console.log( signedCert2 );
console.log( cert );

if( vfs.TLS.validate( {cert:signedCert3, chain:signedCert2+cert} ) )
	console.log( "Chain is valid." );

});
