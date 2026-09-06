
import {Events} from "../events/events.mjs"
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"

// Heartbeat ops - ordinary data messages, not websocket control frames, because
// a browser cannot see control-frame ping/pong at all.  The server drives; this
// side answers, and separately watches for the ping it was promised - that
// watchdog is what lets a browser notice a server that has gone away without
// closing the socket.  Must match the values in server-protocol.mjs.
// Sent as a bare string, not JSOX - skips the parser for heartbeat traffic.
// NOTE 🏓 is U+1F3D3, above the BMP, so it is a surrogate PAIR: "🏓".length === 2
// and data[0] is only the high surrogate.  ⚪ is U+26AA and IS in the BMP, so it
// is a single code unit - that asymmetry makes an index test look half-working.
// Compare code points, and use [...data] when indexing by character.
const PING = "🏓"; // server -> client
const PONG = "⚪"; // client -> server
const PING_CP = PING.codePointAt( 0 );
const PONG_CP = PONG.codePointAt( 0 );
// the server biases the cadence into the astral planes so it can never be a lone
// surrogate on the wire (which would come back as U+FFFD); undo that here
const HB_BIAS = 0x10000;


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
		const source = new URL( import.meta.url ).origin
		ThisProtocol.ws = new WebSocket( this_.server, protocol );
		ThisProtocol.ws.onmessage = (evt)=>Protocol.onmessage.call( this_, evt) ;
		ThisProtocol.ws.onclose = (evt)=>Protocol.onclose.call( this_, evt) ;
		ThisProtocol.ws.onerror = (evt)=>Protocol.onerror.call( this_, evt) ;
		ThisProtocol.ws.onopen = (evt)=>Protocol.onopen.call( this_, evt) ;
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

	static onopen( evt ) {
		const ThisProtocol = Object.getPrototypeOf( this ).constructor;
		ThisProtocol.on( "open", true );
		this.on( "open", true );
	}

	static onclose( evt ){
		const Protocol = Object.getPrototypeOf( this ).constructor;
		Protocol.clearHeartbeat( this );
		Protocol.debug && console.log( "close?", this, evt );
		const event = this.on( "close", [evt.code, evt.reason] );
		Protocol.ws = null;
		if( evt.code === 1000 ) this.connect();
		else setTimeout( this.connect.bind(this), 5000 );
	}

	static onerror( evt ){
		const Protocol = Object.getPrototypeOf( this ).constructor;
		Protocol.debug && console.log( "error?", this, evt );
		//const event = this.on( "close", [evt.code, evt.reason] );
		//Protocol.ws = null;
		//if( evt.code === 1000 ) this.connect();
		//else setTimeout( this.connect.bind(this), 5000 );
	}

	static onmessage( evt ) {
		Protocol.debug && console.log( "got:", this, evt );
		const cp = evt.data.codePointAt( 0 );
		if( cp === PING_CP ) {
			// answer, then re-arm; the cadence comes from the server so this side
			// needs no configuration of its own.  Not dispatched to the app.
			this.send( PONG );
			// spread to index by character - the ping and both payload values are
			// surrogate pairs, so data[1]/data[2] would land mid-pair.  Only done
			// here, where the string is three characters.
			const parts = [...evt.data];
			Protocol.armHeartbeat( this, parts[1].codePointAt(0) - HB_BIAS
			                           , parts[2].codePointAt(0) - HB_BIAS );
			return;
		}
		if( cp === PONG_CP ) return; // server answering our ping; nothing to do

		const msg = JSOX.parse( evt.data );
		if( !this.on( msg.op, msg ) ){
			Protocol.debug && console.log( "Unhandled message:", msg );
		}
	}

	/**
	 * (Re)start the watchdog that expects the next server ping.  Only ever armed by
	 * receiving a ping, so a server that doesn't send them never trips this and
	 * older servers keep working unchanged.
	 */
	static armHeartbeat( this_, interval, timeout ) {
		Protocol.clearHeartbeat( this_ );
		const budget = ( interval || 25000 ) + ( timeout || 20000 );
		this_.hbTimer = setTimeout( ()=>{
			this_.hbTimer = null;
			Protocol.debug && console.log( "server missed its heartbeat; closing" );
			// close rather than pretend we are connected - onclose then runs the
			// existing reconnect path.
			const ws = this_.ready ? Object.getPrototypeOf( this_ ).constructor.ws : null;
			if( ws ) ws.close( 1001, "no heartbeat from server" );
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

