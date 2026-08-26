// Regression test for the uninitialized `struct internalCert` in ssl_BeginClientSession_
// (sack 6a9448483).  New() does not zero, nothing on the client path ever assigned
// cert->x509 / cert->chain, and ssl_DestroySession freed them unconditionally.
//
// NOTE ON WHAT ACTUALLY TRIGGERS IT: not rejectUnauthorized.  Both verdicts reach
// ssl_DestroySession - the reject path via ssl_ClosePipeSession -> ssl_RetireSession
// (SSL_CLOSE_REJECT), the accept path via the ordinary close - so the frees run either
// way.  What decides crash-vs-clean is whether New() handed back a fresh (zeroed) region
// or a RECYCLED block carrying stale bytes.  So the test is driven by session VOLUME,
// and runs both verdicts because they are two different routes into the same destructor.
//
//   node tests/tlsRejectUnauthorizedTest.js [port] [countPerArm]
//   REPS=n to loop the whole thing.

// SACK_NODE=<path to sack_vfs.node> loads one specific build directly.  Needed because
// vfs_module.cjs searches Debug -> RelWithDebInfo -> Release (the `major >= 16` test makes
// the Debug attempt unconditional), so a STALE build/RelWithDebInfo silently shadows a fresh
// build/Release and you end up testing the wrong binary - and a long-running service holding
// that file mapped makes it unwritable, so the rebuild fails while npm still reports success.
// Always confirm the path printed below is the build you meant to test.
var sack = process.env.SACK_NODE
         ? require( require( "path" ).resolve( process.env.SACK_NODE ) )
         : require( ".." );
try {
	const loaded = Object.keys( require.cache ).filter( ( f ) => f.endsWith( "sack_vfs.node" ) );
	if( loaded.length ) console.log( "module:", loaded.join( ", " ) );
} catch( e ) { /* not fatal - the assertions below still hold */ }

const port  = Number( process.argv[2] ) || 8443;
const COUNT = Number( process.argv[3] ) || Number( process.env.COUNT ) || 400;
const REPS  = Number( process.env.REPS ) || 1;

//--------------------------------------------------- untrusted chain, generated in-process
const baseSerial = 2051;
var keys = [ sack.TLS.genkey( 2048 ), sack.TLS.genkey( 2048 ), sack.TLS.genkey( 2048, "password" ) ];

var certRoot = sack.TLS.gencert( { key:keys[0]
	, country:"US", state:"NV", locality:"Las Vegas"
	, org:"Freedom Collective", unit:"Tests", name:"Root Cert", serial: baseSerial } );

var signer = sack.TLS.signreq( {
	  request: sack.TLS.genreq( { key:keys[1]
		, country:"US", state:"NV", locality:"Las Vegas"
		, org:"Freedom Collective", unit:"Tests", name:"CA Cert", serial: baseSerial+1 } )
	, signer: certRoot, serial: baseSerial+2, key:keys[0] } );

var cert = sack.TLS.signreq( {
	  request: sack.TLS.genreq( { key:keys[2], password:"password"
		, country:"US", state:"NV", locality:"Las Vegas"
		, org:"Freedom Collective", unit:"Tests", name:"localhost", serial: baseSerial+3
		, subject: { DNS:["localhost","*.localhost"], IP:["127.0.0.1"] } } )
	, signer: signer, serial: baseSerial+4, key:keys[1] } );

//--------------------------------------------------- server
var served = 0;
var server = sack.WebSocket.Server( { port: port
	, cert: cert + signer + certRoot
	, key: keys[2]
	, passphrase: "password" } );

server.onrequest = function ( req, res ) {
	served++;
	res.writeHead( 200, { 'Content-Type': 'text/plain' } );
	res.end( "ok" );
};
server.onerrorlow = function ( error, socket ) { /* handshake failures are expected here */ };

//--------------------------------------------------- client arms
function arm( reject, count ) {
	return new Promise( ( done ) => {
		let seen = 0, ok = 0, failed = 0;
		const errs = {};
		for( let i = 0; i < count; i++ ) {
			const opts = { hostname: "localhost", port: port, method: "GET", path: "/"
			             , timeout: 5000, ca: null, rejectUnauthorized: reject };
			opts.onReply = ( res ) => {
				if( res && !res.error && res.statusCode === 200 ) ok++;
				else {
					failed++;
					const k = res ? String( res.error || ( "status " + res.statusCode ) ) : "null result";
					errs[k] = ( errs[k] || 0 ) + 1;
				}
				if( ++seen === count ) done( { ok, failed, errs } );
			};
			try { sack.HTTPS.get( opts ); }
			catch( e ) {
				failed++;
				errs["throw: " + e.message] = ( errs["throw: " + e.message] || 0 ) + 1;
				if( ++seen === count ) done( { ok, failed, errs } );
			}
		}
	} );
}

( async () => {
	let bad = 0;
	for( let rep = 0; rep < REPS; rep++ ) {
		// rejectUnauthorized:false - the chain is untrusted, so this is the arm that
		// COMPLETES the handshake and runs a full session teardown per request.
		const off = await arm( false, COUNT );
		if( off.ok !== COUNT ) {
			bad++;
			console.log( "FAIL rejectUnauthorized=false: ok=" + off.ok + "/" + COUNT
			           + " failed=" + off.failed + " " + JSON.stringify( off.errs ) );
		}

		// rejectUnauthorized:true - every request MUST be refused (self-signed root the
		// client has no reason to trust).  Zero successes is the pass condition; this arm
		// exists to exercise the reject route into the same destructor, not to serve.
		const on = await arm( true, COUNT );
		if( on.ok !== 0 ) {
			bad++;
			console.log( "FAIL rejectUnauthorized=true: " + on.ok + "/" + COUNT
			           + " requests were served with an untrusted chain" );
		}

		console.log( "rep " + (rep+1) + "/" + REPS
		           + "  off: ok=" + off.ok + " failed=" + off.failed
		           + "  on: ok=" + on.ok + " failed=" + on.failed
		           + "  " + JSON.stringify( on.errs ) );
	}
	// Surviving to here at all is the real assertion - the defect was a hard crash in
	// X509_free/EVP_PKEY_free on a recycled block, not a wrong answer.
	console.log( "served=" + served + "  sessions=" + ( COUNT * 2 * REPS ) );
	console.log( bad === 0 ? "TLS REJECTUNAUTHORIZED PASS" : "TLS REJECTUNAUTHORIZED FAIL" );
	process.exit( bad === 0 ? 0 : 1 );
} )();
