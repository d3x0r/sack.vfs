
//const tls = require('tls');
//console.log(tls.getCiphers());
const vfs = require( ".." );
const vol = vfs.Volume();
//vfs.TLS.seed( )

//--------------------------------------------------------------------

var key = vfs.TLS.genkey( 1024, "password" );
var pubkey = vfs.TLS.pubkey( { key:key, password:"password" } );
var cert = vfs.TLS.gencert( { country:"US"
		, state:"NV"
		, locality:"Las Vegas"
		, org:"Freedom Collective"
		, unit:"IT"
		, commonName:"Root Cert"
		, serial: 1001
		, issuer: "UniqueString1"
		, key:key, password:"password"
		, pubkey:pubkey } );

var keyAlt = vfs.TLS.genkey( 1024, "password" );
var pubkeyAlt = vfs.TLS.pubkey( { key:keyAlt, password:"password" } );
var certAlt = vfs.TLS.gencert( { country:"US"
		, state:"NV"
		, locality:"Las Vegas"
		, org:"Freedom Collective"
		, unit:"IT"
		, commonName:"Root Cert"
		, serial: 1001
		, issuer: "UniqueString1"
		, key:keyAlt, password:"password"
		, pubkey: pubkey
});

//--------------------------------------------------------------------

var key2 = vfs.TLS.genkey( 1024, "pass2" );
var pubkey2 = vfs.TLS.pubkey( { key:key2, password:"pass2" } );
var cert2 = vfs.TLS.genreq( { country:"US"
		, state:"NV"
		, locality:"Las Vegas"
		, org:"Freedom Collective"
		, unit:"IT"
		, commonName:"CA Cert"
		, serial: 1002
		, key:key2, password:"pass2" } );
var signedCert2 = vfs.TLS.signreq( { request:cert2
		, signer:cert
		, key:key, password:"password"
		, serial: 1003
		, issuer: "UniqueString1"
		, subject:" UniqueString2"
		 } );

var key2Alt = vfs.TLS.genkey( 1024, "pass2" );
var pubkey2Alt = vfs.TLS.pubkey( { key:key2Alt, password:"pass2" } );
var cert2Alt = vfs.TLS.genreq( { country:"US"
		, state:"NV"
		, locality:"Las Vegas"
		, org:"Freedom Collective"
		, unit:"IT"
		, commonName:"CA Cert"
		, serial: 1002
		, key:key2Alt, password:"pass2" } );
var signedCert2Alt = vfs.TLS.signreq( { request:cert2Alt
		, signer:certAlt
		, key:keyAlt, password:"password"
		, serial: 1003
		, issuer: "UniqueString1"
		, subject:" UniqueString2"
		, pubkey: pubkey2 
		 } );

//--------------------------------------------------------------------

var key3 = vfs.TLS.genkey( 1024, "pass4" );
var pubkey3 = vfs.TLS.pubkey({ key: key3, password: "pass4" } );
var cert3 = vfs.TLS.genreq( { country: "\0\0", state: "Conscious", locality: "C-316", org: "Yes", unit: "Super"
		, commonName:"www.common.name"
		, serial: 1004
		, key:key3, password:"pass4" } );
var signedCert3 = vfs.TLS.signreq( { request:cert3
		, signer:signedCert2
		, serial: 1005
		, key:key2, password:"pass2" } );

var key3Alt = vfs.TLS.genkey( 1024, "pass4" );
var cert3Alt = vfs.TLS.genreq( { country:"\0\0", state:"Conscious", locality:"C-316", org:"Yes", unit:"Super"
		, commonName:"www.common.name"
		, serial: 1004
		, key:key3Alt, password:"pass4" } );
var signedCert3Alt = vfs.TLS.signreq( { request:cert3Alt
		, signer:signedCert2Alt
		, serial: 1005
		, key:key2Alt, password:"pass2"
		, pubkey:pubkey3
});

if (false) {


    try {
        //console.log( "Valid:\n", signedCert3, signedCert2, cert );

        vol.write("cert.1", signedCert3);
        vol.write("chain.1", signedCert2 + cert);
        vol.write("chain.1.cert.1", signedCert2);
        vol.write("chain.1.cert.2", cert);
        console.log("Valid:", vfs.TLS.validate({ cert: signedCert3, chain: signedCert2 + cert }));
    } catch (err) {
        console.log("Valid chain is not valid?:", err);
    }
    try {
        //console.log( "AltValid:\n", signedCert3, signedCert2, certAlt );
        vol.write("cert.2", signedCert3);
        vol.write("chain.2", signedCert2 + certAlt);
        vol.write("chain.2.cert.1", signedCert2);
        vol.write("chain.2.cert.2", certAlt);
        console.log("AltValid2:", vfs.TLS.validate({ cert: signedCert3, chain: signedCert2 + certAlt }));
    } catch (err) {
        console.log("Invalid chain is not valid 2.:", err);
    }
}

try {
	//console.log( "AltValid2:\n", signedCert3, signedCert2Alt, certAlt );
	vol.write( "cert.3", signedCert3Alt );
	vol.write( "chain.3", signedCert2Alt+cert );
	vol.write( "chain.3.cert.1", signedCert2Alt );
	vol.write( "chain.3.cert.2", cert );
	console.log( "AltValid3:", vfs.TLS.validate( {cert:signedCert3Alt, chain:signedCert2Alt+cert} ) );
} catch( err ) {
	console.log( "Invalid chain is not valid 3.:", err );
}


try {
	//console.log( "AltValid3:\n", signedCert3Alt, signedCert2Alt, certAlt );
	vol.write( "cert.4", signedCert3Alt );
	vol.write( "chain.4", signedCert2+cert );
	vol.write( "chain.4.cert.1", signedCert2 );
	vol.write( "chain.4.cert.2", cert );
	console.log( "AltValid4:", vfs.TLS.validate( {cert:signedCert3Alt, chain:signedCert2+cert} ) );
} catch( err ) {
	console.log( "Invalid chain is not valid 4.:", err );
}

try {
	//console.log( "AltValid4:\n", signedCert3Alt, signedCert2Alt, certAlt );
	vol.write( "cert.5", signedCert3Alt );
	vol.write( "chain.5", signedCert2Alt+certAlt );
	vol.write( "chain.5.cert.1", signedCert2Alt );
	vol.write( "chain.5.cert.2", certAlt );
	console.log( "AltValid5:", vfs.TLS.validate( {cert:signedCert3Alt, chain:signedCert2Alt+certAlt} ) );
} catch( err ) {
	console.log( "Invalid chain is not valid 5.:", err );
}


try {
    //console.log( "AltValid4:\n", signedCert3Alt, signedCert2Alt, certAlt );
    vol.write("cert.6", signedCert3Alt);
    vol.write("chain.6", signedCert2Alt + cert);
    vol.write("chain.6.cert.1", signedCert2Alt);
    vol.write("chain.6.cert.2", cert );
    console.log("AltValid6:", vfs.TLS.validate({ cert: signedCert3Alt, chain: signedCert2Alt + cert }));
} catch (err) {
    console.log("Invalid chain is not valid 6.:", err);
}
