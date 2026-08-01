
import {Events} from "../events/events.mjs"
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"

// Heartbeat ops - ordinary data messages, not websocket control frames, since a
// browser cannot observe control-frame ping/pong.  Must match server-protocol.mjs.
// Bare string, not JSOX.  🏓 is U+1F3D3 (above the BMP) so it is a surrogate
// PAIR - data[0] is only the high surrogate; ⚪ is U+26AA and is one code unit.
// Compare code points, and use [...data] when indexing by character.
const PING = "🏓"; // server -> client
const PONG = "⚪"; // client -> server
const PING_CP = PING.codePointAt( 0 );
const PONG_CP = PONG.codePointAt( 0 );
const HB_BIAS = 0x10000; // server biases the cadence out of the surrogate block


export class Protocol extends Events {
	static debug = false;
	protocol = null;
	server = new URL(import.meta.url).origin.replace("http","ws") || location.origin.replace("http","ws");
	#Protocol = Protocol; // this is the proper class container of the implemented protocol
	get debug() {
		return Protocol.debug;
	}
	set debug(val) {
		Protocol.debug = val;
	}
	// server defaults to location this script ws loaded from
	// normally only have to specify the protocol.
	constructor( protocol, server ){
		super();
		if( server ) this.server = server.replace("http","ws");
		this.#Protocol = Object.getPrototypeOf( this ).constructor;
		this.#Protocol.ws = null; // allocate static ws member.
		this.protocol = protocol;
		if( protocol )
			Protocol.connect(protocol, this);
	}

	static connect(protocol, this_) {
		const ThisProtocol = this_.#Protocol;//Object.getPrototypeOf( this ).constructor;
		ThisProtocol.ws = new WebSocket( this_.server, protocol );
		ThisProtocol.ws.onmessage = Protocol.onmessage.bind( this_, ThisProtocol.ws );
		ThisProtocol.ws.onclose = Protocol.onclose.bind( this_, ThisProtocol.ws );
		ThisProtocol.ws.onopen = Protocol.onopen.bind( this_, ThisProtocol.ws );
		return ThisProtocol.ws;
	}

	get ready() {
		if( this.#Protocol.ws )
			if( this.#Protocol.ws.readyState == 1 ) return true;
		return false;
	}
	
	connect() {
		return Protocol.connect( this.protocol, this );
	}

	static onopen( ws, evt ) {
		const ThisProtocol = Object.getPrototypeOf( this ).constructor;
		ThisProtocol.on( "open", true );
		this.on( "open", true );
	}

	static onclose( ws, evt ){
		const Protocol = Object.getPrototypeOf( this ).constructor;
		Protocol.clearHeartbeat( this );
		Protocol.debug && console.log( "close?", this, evt );
		const event = this.on( "close", [ws, evt.code, evt.reason] );
		Protocol.ws = null;
		if( evt.code === 1000 ) this.connect();
		else setTimeout( this.connect.bind(this), 5000 );
	}

	static onmessage( ws, evt ) {
		Protocol.debug && console.log( "got:", this, evt );
		const cp = evt.data.codePointAt( 0 );
		if( cp === PING_CP ) {
			// answer, then re-arm; the cadence comes from the server so this side
			// needs no configuration of its own.  Not dispatched to the app.
			this.send( PONG );
			const parts = [...evt.data]; // index by character, not code unit
			Protocol.armHeartbeat( this, ws, parts[1].codePointAt(0) - HB_BIAS
			                              , parts[2].codePointAt(0) - HB_BIAS );
			return;
		}
		if( cp === PONG_CP ) return; // server answering our ping; nothing to do
		const msg = JSOX.parse( evt.data );
		if( !this.on( msg.op, [ws, msg] ) ){
			Protocol.debug && console.log( "Unhandled message:", msg );
		}
	}

	/**
	 * Watchdog for the ping the server promised; only ever armed by receiving one,
	 * so servers without a heartbeat are unaffected.
	 */
	static armHeartbeat( this_, ws, interval, timeout ) {
		Protocol.clearHeartbeat( this_ );
		const budget = ( interval || 25000 ) + ( timeout || 20000 );
		this_.hbTimer = setTimeout( ()=>{
			this_.hbTimer = null;
			Protocol.debug && console.log( "server missed its heartbeat; closing" );
			if( ws && ws.readyState === 1 ) ws.close( 1001, "no heartbeat from server" );
		}, budget );
	}

	static clearHeartbeat( this_ ) {
		if( this_.hbTimer ) { clearTimeout( this_.hbTimer ); this_.hbTimer = null; }
	}

	send( msg ) {
		const ws = this.#Protocol.ws;
		if( ws && ws.readyState === 1 ) {
			if( "object" === typeof msg ) {
				ws.send( JSOX.stringify(msg) ); 
			} else
				ws.send( msg );	
		} else {
			Protocol.debug && console.log( "Protocol socket is not in open readystate", ws.readyState );
		}
	}
	close( code, reason ) {
		return this.ws.close( code, reason );
	}

} 

