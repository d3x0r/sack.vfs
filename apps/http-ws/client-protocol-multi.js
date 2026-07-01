
import {Events} from "../events/events.mjs"
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"


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
		Protocol.debug && console.log( "close?", this, evt );
		const event = this.on( "close", [ws, evt.code, evt.reason] );
		Protocol.ws = null;
		if( evt.code === 1000 ) this.connect();
		else setTimeout( this.connect.bind(this), 5000 );
	}

	static onmessage( ws, evt ) {
		Protocol.debug && console.log( "got:", this, evt );
		const msg = JSOX.parse( evt.data );
		if( !this.on( msg.op, [ws, msg] ) ){
			Protocol.debug && console.log( "Unhandled message:", msg );
		}
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

