"use strict"

//console.log( "Core Key Service." );
const _debug = false;

const vfs = require( "sack.vfs" );
const TLS = vfs.TLS;
const JSON = vfs.JSOX;
const config = require( '../../config.js' );
const fc = require( "../../file_cluster.js" );
const idGen= require( "../id_generator.js");

const l = {
	chain : null,
	caCert : null,
	caKey : null,
	rootPass : null,
	certPass : null,
	ca_serial_0 : 1516,
	ca_serial : 2001,
	db : null,
        caUsers : []
}

module.exports = exports = {
	server : null,
	init( ) {
		initDb();
		var config = l.db.do( 'select * from state' );
		if( !config || !config.length )
			initKeys();
		else {
			config = config[0];
			var ca = null;
			try {
				ca = JSON.parse( config.ca );
			} catch( err ) { console.log("Failed to parse:", config.ca, "\n", err ) }
			if( ca ) {
				l.chain = ca[0] + ca[1];
				l.caCert = ca[0];
				l.rootCert = ca[1];
				l.caKey = config.keypair;
				l.certPass = config.password;
				l.ca_serial = config.serial;
				l.ca_serial_0 = config.ca_serial;
				if( !l.ca_serial_0 ) {
					l.ca_serial_0 = 1517;
					l.db.do( "update state set ca_serial=1517" );
				}
			}
			setTimeout( checkCA, 10 );
			//else initKeys();
		}
	},
        addUpdateCallback( cb ) {
        	l.caUsers.push(cb);
        },
        getRootCert() {
        	return l.rootCert;
        },
	registerProtocol( server ) {
		//this.server = server;
		_debug && console.log( 'new protocol on new server?', server );
		server.addProtocol( "tls.core", this.protocolHandler );
	},
	autoExpireCert : autoExpireCert,
	protocolHandler( ws) {
		_debug && console.log( "Received connection to key service", ws.clientAddress )
		if( !config.isLocal( ws.clientAddress ) ){
			ws.ban();
			return;
		}
		ws.on( "message", (_msg,m2)=>{
			var msg = JSON.parse( _msg );
			//console.log( "key protocol handler msg:", msg );
			if( msg.op === "hello" ) {
				ws.send( '{"op":"hello"}' );
			} else if( msg.op === "genCert" ) {
				checkCA();   // regeneration might be because CA has expired, will need a new client cert based on this new CA
				genCert( msg.opts, (keyinfo)=>{
					ws.send( JSON.stringify( {op:"replyCert", keyInfo:keyinfo} ) );
				} );
			} else {
				console.log( "Unhandled Message received in key service:", _msg );
				ws.close();
			}
		} );
		ws.on( "close", ()=>{
			//console.log( "Key service connection has ended; need to cleanup allocatd service." );
		} );
	},
	genCert : genCert,
}

function checkCA() {

	var expiration = TLS.expiration( l.caCert );
        if( (expiration.getTime() - (24*3600*1000)) < Date.now() ) {
        	regenCA();
                //l.caUsers.forEach( user=>user( l.caCert ) );
	}
	return;
	// check at one day before expiration
        var delay = ( expiration.getTime() - Date.now() );
        
        if( delay > (24*3600*1000) )
        	delay = delay - (24*3600*1000) + 1000;
        else
        	delay = 60 * 1000;
        if( delay > 0x1000000 )
        	delay = 0x1000000
        //console.log( "cert timeout should be in :", delay, delay/1000, delay/(1000*60), delay/(60*60*1000), delay.toString( 16 ) );
	setTimeout( checkCA, delay );
}

function autoExpireCert( genInfo, keyInfo, cb ) {

	function timer() {
		var expiration = TLS.expiration( keyInfo.cert );
		if( ( expiration.getTime() - (24*3600*1000)) < Date.now() ) {
			console.log( "forced regeneration?", keyInfo.address );
			//console.log( expiration.getTime() - Date.now());
			exports.genCert( genInfo, cb );
			console.log( "should have new auto expire on new socket (above?)");
			return;
		}
		var expiration2 = TLS.expiration( keyInfo.ca );
		//console.log( expiration2.getTime() - Date.now());
		if( (expiration2.getTime() - (24*3600*1000)) < Date.now() ) {
			exports.genCert( genInfo, cb );
			return;
		}
		//console.log( "expirations:", expiration, expiration2 );

		if( expiration.getTime() > expiration2.getTime() )
			expiration = expiration2;
		// check at one day before expiration
        	var delay = ( expiration.getTime() - Date.now() );
		
			// target one second after 1 day before expiration.
        	if( delay > (24*3600*1000) )
				delay = delay - (24*3600*1000) + 1000;
				
	        //console.log( "Delay for timeout is : ", delay, delay.toString( 16 ) );
        	if( delay > 0x1000000 )
				delay = 0x1000000
				
			//console.log( "cert timeout should be in :", delay, delay/1000, delay/(1000*60), delay/(60*60*1000), delay.toString( 16 ) );
			//console.log( "Delay for timeout is : ", delay, delay.toString( 16 ) );
        	//if( delay > 60000 )
				//delay = 60000
			setTimeout( timer, delay );
        }
        setTimeout( timer, 60000 );
}              


function initKeys(){
	//console.log( "INIT KEYS:", config.run, config );
	if( !config.run.defaults.allowRootCertGen )
        {
	        throw new Error( "Root certificate generation is disabled..." );
        }
	l.rootPass = idGen.generator();
	l.certPass = idGen.generator();
	var rootPass = idGen.regenerator(l.rootPass);
	console.log( "Generate root key (please wait)" );
	l.rootKey = TLS.genkey( 8192, rootPass );

	l.db.do( "delete from private" );
	l.db.do( `insert into private( keypair, password ) values('${l.rootKey}','${l.rootPass}')` );

	l.rootCert = TLS.gencert({
		org: "who it's for",
		unit: "Secure Services",
		country: "US",
		state: "NV",
		locality: "Las Vegas",
		name : "Domain CA Root",
		serial : 1514,
		expire : 36500,
		key : l.rootKey,
		password : rootPass
	});
	   
	var certPass = idGen.regenerator(l.certPass);
	console.log( "Generate cert key (please wait)" );
    //console.log( "certpass?", l.certPass );
	l.caKey = TLS.genkey( 4096, certPass );
	var caCertReq = TLS.genreq({
		org: "who it's for",
		unit: "Secure Services",
		country: "US",
		state: "NV",
		locality: "Las Vegas",
		name : "Domain CA Cert",
		key : l.caKey,
		password : certPass
	});
    //console.log( "root?", l.rootCert );
	l.caCert = TLS.signreq( { signer:l.rootCert, key:l.rootKey, password:rootPass, request: caCertReq, expire : 30,
		serial: l.ca_serial_0 } );

	l.chain = l.caCert + l.rootCert;

	l.db.do( "delete from state" );
	l.db.do( `insert into state(ca,keypair,password,serial) values ('${JSON.stringify( [l.caCert,l.rootCert] )}', '${l.caKey}', '${l.certPass}', ${l.ca_serial})` );
}

function regenCA() {
	if( !l.rootPass ) {
		var config = l.db.do( 'select * from private' );
		if( !config ) {
			throw new Error( "Private information is required. Send Administrator a request." );
		}
                //console.log( "Recovered private info?", config );
		l.rootPass = config[0].password;
		l.rootKey = config[0].keypair;
	}
        
	var rootPass = idGen.regenerator( l.rootPass );
	
	var certPass = idGen.regenerator(l.certPass);
	console.log( "Generate cert key (please wait)" );
	l.caKey = TLS.genkey( 4096, certPass );
	var caCertReq = TLS.genreq({
		org: "who it's for",
		unit: "Secure Services",
		country: "US",
		state: "NV",
		locality: "Las Vgeas",
		name : "Domain CA Cert",
		key : l.caKey,
		password : certPass
	});
	l.caCert = TLS.signreq( { signer:l.rootCert, key:l.rootKey, password:rootPass, request: caCertReq, expire : 30,
		serial: l.ca_serial_0 } );

	l.chain = l.caCert + l.rootCert;

	//l.db.do( "delete from state" );
	l.db.do( `update state set ca='${JSON.stringify( [l.caCert,l.rootCert] )}',keypair='${l.caKey}',password='${l.certPass}',ca_serial='${l.ca_serial_0}'` );
}

function genCert( opts, cb ) {
	if( !opts.name ) throw new Exception( "Required option 'name' was not specified" );
	opts.caRoot = l.rootCert;
	var key = opts.key || TLS.genkey( 1024, opts.password );
	//console.log( "key:", key );
	var certOpts = {
		org: "who it's for",
		unit: "Secure Services",
		country: "US",
		state: "NV",
		locality: "Las Vegas",
		subject: opts.subject,
		name : opts.name,
		key : key,
		password : opts.password,
		DNS : opts.DNS,
		IP : opts.IP
	}
	var caCertReq = TLS.genreq( certOpts );
	var certPass = idGen.regenerator(l.certPass);
	var caCert = TLS.signreq( { signer:l.caCert, key:l.caKey, expire : 7,
		password:certPass, request: caCertReq, serial: l.ca_serial } );

	l.db.do( `insert into keylog (cert,keypair) values ('${caCert}','${key}' )`);
	l.db.do( `update state set serial=${l.ca_serial++}` );
	var info = { cert: caCert, key: key, ca: l.caCert, password:opts.password };
	console.log( "Updated certificates....", opts.name );
	cb( info )
	//console.log( "Updated certificates...." );
	return undefined;
}


function initDb() {
	l.db = fc.cvol.Sqlite( "keyMaster.db" );
	l.db.makeTable( "create table private ( keypair char, password char )" );
	l.db.makeTable( "create table state ( ca char, keypair char, password char, serial int, ca_serial int )" );
	l.db.makeTable( "create table keylog ( created DATETIME default CURRENT_TIMESTAMP, cert char, keypair char" );
	l.db.makeTable( "create table keys ( keyid char( 45 ) PRIMARY KEY, cert char, keypair char" );
	
}
