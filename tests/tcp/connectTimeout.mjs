// TCP connect / connect-timeout exerciser.
//
//   node --experimental-loader=sack.vfs/import.mjs tests/tcp/connectTimeout.mjs [options]
//
// With no options it runs a self-contained suite: opens a listener, probes it
// (expects one connect), probes a port nothing listens on (expects exactly one
// error, no duplicate), and probes a black-hole address (expects exactly one
// error, ETIMEDOUT, at about the requested timeout).
//
//   --address <host[:port]>  address to bind a listener to (default 127.0.0.1)
//   --port <n>               port to listen on (default 40123)
//   --toAddress <host>       address to connect to (default: the listener above)
//   --toPort <n>             port to connect to
//   --timeout <ms>           connect timeout passed to sack.Network.TCP (default 1500)
//   --blackhole <host>       unroutable address for the timeout case (default 10.255.255.1)
//   --count <n>              repeat each probe n times (default 1)
//   --listen                 only open the listener and stay up (for the other machine)
//   --probe                  only probe --toAddress/--toPort and report
//
// Exit code is 0 when every expectation held, 1 otherwise.

import { sack } from "sack.vfs";

const args = process.argv.slice( 2 );
const opt = { address: "127.0.0.1", port: 40123, toAddress: null, toPort: 0
            , timeout: 1500, blackhole: "10.255.255.1", count: 1, listen: false, probe: false };
for( let i = 0; i < args.length; i++ ) {
	const a = args[i];
	if( a === "--listen" ) opt.listen = true;
	else if( a === "--probe" ) opt.probe = true;
	else if( a.startsWith( "--" ) ) opt[a.slice( 2 )] = args[++i];
	else console.log( "Unknown argument:", a );
}
for( const k of [ "port", "toPort", "timeout", "count" ] ) opt[k] = Number( opt[k] ) || 0;

const isWin = process.platform === "win32";
const ECONNREFUSED = isWin ? 10061 : 111;
const ETIMEDOUT    = isWin ? 10060 : 110;

let failures = 0;
function expect( cond, what ) {
	console.log( ( cond ? "  ok   " : "  FAIL " ) + what );
	if( !cond ) failures++;
}

const t0 = Date.now();
const stamp = ()=>String( Date.now() - t0 ).padStart( 6 ) + "ms ";

// One connection attempt.  Resolves after the socket has gone quiet for
// `settle` ms past its first event, so late duplicate callbacks are counted.
function probe( host, port, timeout, settle = 750 ) {
	return new Promise( ( resolve )=>{
		const r = { host, port, timeout, connect: 0, error: 0, close: 0, errors: [], first: 0, started: Date.now() };
		let timer = null;
		const settleSoon = ()=>{
			if( !r.first ) r.first = Date.now() - r.started;
			clearTimeout( timer );
			timer = setTimeout( ()=>{ try { socket.close(); } catch( e ) {} resolve( r ); }, settle );
		};
		const socket = sack.Network.TCP( { toAddress: host, toPort: port, timeout
			, connect( err ) { r.connect++; console.log( stamp() + "connect  " + host + ":" + port + ( err === undefined ? "" : " arg=" + err ) ); settleSoon(); }
			, error( err )   { r.error++;   r.errors.push( err ); console.log( stamp() + "error    " + host + ":" + port + " code=" + err );
				// close from inside the error callback, as task-manager's probePort does:
				// this deadlocked the connect-timeout timer (client lock held, waiting
				// for the global lock) against RemoveClient (global held, spinning on
				// the client lock).  The socket is already closed by then; this must
				// be a harmless no-op.
				try { socket.close(); } catch( e ) { console.log( "close() threw:", e ); }
				settleSoon(); }
			, close()        { r.close++;   console.log( stamp() + "close    " + host + ":" + port ); settleSoon(); }
		} );
		// belt and braces: never hang if nothing at all comes back
		timer = setTimeout( ()=>{ console.log( stamp() + "no event " + host + ":" + port + " after " + ( timeout + 5000 ) + "ms" ); try { socket.close(); } catch( e ) {} resolve( r ); }, timeout + 5000 );
	} );
}

function listen( address, port ) {
	const server = sack.Network.TCP( { address, port, readStrings: true
		, connect( client ) {
			console.log( stamp() + "listener accepted " + JSON.stringify( client.connection ) );
			client.on( "message", ( msg )=>client.send( msg ) );
			client.on( "close", ()=>console.log( stamp() + "listener client closed" ) );
		}
	} );
	console.log( stamp() + "listening on " + address + ":" + port );
	return server;
}

async function repeat( label, host, port, timeout, check ) {
	for( let i = 0; i < opt.count; i++ ) {
		console.log( "\n[" + label + ( opt.count > 1 ? " #" + ( i + 1 ) : "" ) + "] " + host + ":" + port + " timeout=" + timeout );
		const r = await probe( host, port, timeout );
		check( r );
	}
}

async function main() {
	if( opt.listen ) {
		listen( opt.address, opt.port );
		return; // stay alive
	}
	if( opt.probe ) {
		if( !opt.toAddress ) { console.log( "--probe needs --toAddress and --toPort" ); process.exit( 2 ); }
		await repeat( "probe", opt.toAddress, opt.toPort, opt.timeout, ( r )=>{
			expect( r.connect + r.error === 1, "exactly one outcome callback (connect=" + r.connect + " error=" + r.error + " close=" + r.close + ")" );
			console.log( "  result: " + ( r.connect ? "OPEN" : r.error ? "CLOSED/UNREACHABLE (" + r.errors.join( "," ) + ")" : "NO RESPONSE" ) + " after " + r.first + "ms" );
		} );
		process.exit( failures ? 1 : 0 );
	}

	const server = listen( opt.address, opt.port );
	const target = opt.toAddress || opt.address;
	await new Promise( ( r )=>setTimeout( r, 250 ) );

	await repeat( "listening port", target, opt.toPort || opt.port, opt.timeout, ( r )=>{
		expect( r.connect === 1, "connect fired once (" + r.connect + ")" );
		expect( r.error === 0, "no error (" + r.error + ")" );
	} );

	const closedPort = ( opt.toPort || opt.port ) + 1;
	await repeat( "refused port", target, closedPort, opt.timeout, ( r )=>{
		expect( r.connect === 0, "connect never fired (" + r.connect + ")" );
		expect( r.error === 1, "error fired exactly once (" + r.error + ") codes=" + r.errors.join( "," ) );
		expect( r.errors[0] === ECONNREFUSED, "error is ECONNREFUSED " + ECONNREFUSED + " (got " + r.errors[0] + ")" );
		expect( r.close <= 1, "close fired at most once (" + r.close + ")" );
	} );

	await repeat( "black hole", opt.blackhole, 40125, opt.timeout, ( r )=>{
		expect( r.connect === 0, "connect never fired (" + r.connect + ")" );
		expect( r.error === 1, "error fired exactly once (" + r.error + ") codes=" + r.errors.join( "," ) );
		expect( r.errors[0] === ETIMEDOUT, "error is ETIMEDOUT " + ETIMEDOUT + " (got " + r.errors[0] + ")" );
		expect( r.first >= opt.timeout - 50 && r.first < opt.timeout + 1000, "timed out near " + opt.timeout + "ms (" + r.first + "ms)" );
	} );

	console.log( "\n" + ( failures ? failures + " expectation(s) FAILED" : "all expectations passed" ) );
	server.close();
	process.exit( failures ? 1 : 0 );
}

main().catch( ( e )=>{ console.log( e ); process.exit( 1 ); } );
