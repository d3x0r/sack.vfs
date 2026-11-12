// SSH relay - joins two SSH sessions.
//
//   client --> [listen server]:port ==(session A: tcpip-forward)==> this process
//          ==(session B: direct-tcpip)==> [target server] --> host:port
//
// Session A asks its server to listen on a port (like `ssh -R`).  Every connection that
// arrives there is handed to us as a channel; for each one we open a direct-tcpip
// channel through session B (like `ssh -L`) and copy bytes both ways until either side
// closes.  Neither server has to be able to reach the other.
//
// usage: node ssh-relay.mjs [config.json]
//   (defaults to ssh-relay.config.json next to this script; see ssh-relay.config.example.json)

import {sack} from "sack.vfs";
import fs from "node:fs";
import path from "node:path";
import url from "node:url";

const here = path.dirname( url.fileURLToPath( import.meta.url ) );
const configPath = path.resolve( process.argv[2] || path.join( here, "ssh-relay.config.json" ) );
const config = JSON.parse( fs.readFileSync( configPath, "utf8" ) );
const configDir = path.dirname( configPath );

function readKey( file ) {
	return file ? fs.readFileSync( path.resolve( configDir, file ), "utf8" ) : null;
}

async function connect( name, opts ) {
	const ssh = sack.SSH();
	await ssh.connect( { address: opts.address
	                   , user: opts.user
	                   , password: opts.password || ""
	                   , privKey: readKey( opts.privKeyFile )
	                   , pubKey: readKey( opts.pubKeyFile ) // optional; can be derived from the private key
	                   , trace: !!opts.trace
	                   } );
	console.log( `[${name}] connected to ${opts.address} as ${opts.user}` );
	return ssh;
}

// RFC 4254 7.2 direct-tcpip open payload:
//   string host, uint32 port, string originator address, uint32 originator port
function directTcpip( host, port, origHost, origPort ) {
	const enc = new TextEncoder();
	const h = enc.encode( host ), o = enc.encode( origHost );
	const bytes = new Uint8Array( 4 + h.length + 4 + 4 + o.length + 4 );
	const dv = new DataView( bytes.buffer );
	let at = 0;
	dv.setUint32( at, h.length ); at += 4;
	bytes.set( h, at );           at += h.length;
	dv.setUint32( at, port );     at += 4;
	dv.setUint32( at, o.length ); at += 4;
	bytes.set( o, at );           at += o.length;
	dv.setUint32( at, origPort );
	return bytes.buffer;
}

let nextId = 1;
let active = 0;

// Flow control: when more than this many bytes are queued toward one side (its peer's
// SSH window is full - e.g. a slow browser), stop refunding the other side's receive
// window so it stops sending; resume when the queue drains.
const HIGH_WATER = config.highWater || 0x100000;

// copy data from `src` to `dst`, pausing `src` while `dst` is backed up.
// `paused` says whether src is already paused (it's resumed on dst's next drain).
function pipe( src, dst, onBytes, isClosed, paused = false ) {
	src.read( ( buf, stream ) => {
		if( stream || isClosed() ) return;  // tcpip channels only carry stream 0
		onBytes( buf.byteLength );
		dst.send( buf );
		if( !paused && dst.bufferedAmount > HIGH_WATER ) {
			paused = true;
			src.pause();
		}
	} );
	dst.drain( () => {
		if( paused && !isClosed() ) {
			paused = false;
			src.resume();
		}
	} );
}

function relay( target, inbound, targetCfg ) {
	const id = nextId++;
	active++;
	console.log( `[#${id}] accepted; opening ${targetCfg.host}:${targetCfg.port} (active ${active})` );

	let outbound = null;  // direct-tcpip channel on the target session, once open
	let pending = [];     // inbound data that arrived before outbound was open
	let pendingEof = false;
	let closed = false;
	let bytesIn = 0, bytesOut = 0;

	function closeBoth( why ) {
		if( closed ) return;
		closed = true;
		active--;
		try { inbound.close(); } catch( err ) {}
		if( outbound ) try { outbound.close(); } catch( err ) {}
		console.log( `[#${id}] closed (${why}); in ${bytesIn} B, out ${bytesOut} B (active ${active})` );
	}

	// hook inbound immediately - data can arrive before the outbound channel is open.
	// Until it is, hold inbound's window so at most one window's worth is buffered here.
	inbound.binary = true;
	inbound.pause();
	inbound.read( ( buf, stream ) => {
		if( stream || closed ) return;  // tcpip channels only carry stream 0
		bytesIn += buf.byteLength;
		pending.push( buf );
	} );
	inbound.eof( () => {
		if( outbound ) outbound.eof();
		else pendingEof = true;
	} );
	inbound.close( () => closeBoth( "client side" ) );

	target.Channel( { type: "direct-tcpip"
	                , message: directTcpip( targetCfg.host, targetCfg.port, targetCfg.originHost || "127.0.0.1", targetCfg.originPort || 0 )
	                , windowSize: targetCfg.windowSize || 0x200000
	                , packetSize: targetCfg.packetSize || 0x8000
	                } ).then( ( ch ) => {
		if( closed ) { ch.close(); return; }
		outbound = ch;
		outbound.binary = true;
		for( const buf of pending ) outbound.send( buf );
		pending = null;
		if( pendingEof ) outbound.eof();
		// inbound is still paused from above; if the early data already backed up the
		// target, leave it paused for outbound's drain to resume.
		const backedUp = outbound.bufferedAmount > HIGH_WATER;
		const isClosed = () => closed;
		pipe( inbound, outbound, ( n ) => { bytesIn += n; }, isClosed, backedUp );   // replaces the buffering reader
		pipe( outbound, inbound, ( n ) => { bytesOut += n; }, isClosed );
		if( !backedUp ) inbound.resume();
		outbound.eof( () => { if( !closed ) inbound.eof(); } );
		outbound.close( () => closeBoth( "target side" ) );
	} ).catch( ( err ) => {
		console.log( `[#${id}] target open failed:`, err?.message || err );
		closeBoth( "target open failed" );
	} );
}

async function main() {
	const listenCfg = config.listen;
	const targetCfg = config.target;
	// connect both sessions in parallel
	const [ listenSsh, targetSsh ] = await Promise.all( [ connect( "listen", listenCfg ), connect( "target", targetCfg ) ] );

	const listener = await listenSsh.reverse( listenCfg.bind || "localhost", listenCfg.port || 0
		, ( channel ) => relay( targetSsh, channel, targetCfg ) );
	console.log( `[listen] ${listenCfg.address} is listening on ${listenCfg.bind || "localhost"}:${listener.port}`
	           + ` -> [target] ${targetCfg.address} -> ${targetCfg.host}:${targetCfg.port}` );
}

main().catch( ( err ) => {
	console.log( "ssh-relay failed:", err?.message || err );
	process.exit( 1 );
} );
