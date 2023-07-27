
import {Events} from "../events/events.mjs"
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"


export class Protocol extends Events {
	//static ws = null;
	static debug = true;
	protocol = null;

	get debug() {
		return Protocol.debug;
	}
	set debug(val) {
		Protocol.debug = val;
	}
	constructor( protocol ){
		super();
		this.protocol = protocol;
		if( protocol )
			Protocol.connect(protocol, this);
	}

	static connect(protocol, this_) {
		const ThisProtocol = Object.getPrototypeOf( this ).constructor;
		ThisProtocol.ws = new WebSocket( location.origin.replace("http","ws"), protocol );
		ThisProtocol.ws.onmessage = (evt)=>Protocol.onmessage.call( this_, evt) ;
		ThisProtocol.ws.onclose = (evt)=>Protocol.onclose.call( this_, evt) ;
		ThisProtocol.ws.onopen = (evt)=>Protocol.onopen.call( this_, evt) ;
		return ThisProtocol.ws;
	}
	
	connect() {
		return Protocol.connect( this.protocol, this );
	}

	static onopen( evt ) {
		const ThisProtocol = Object.getPrototypeOf( this ).constructor;
		ThisProtocol.on( "open", true );
	}

	static onclose( evt ){
		const Protocol = Object.getPrototypeOf( this ).constructor;
		Protocol.debug && console.log( "close?", this, evt );
		const event = this.on( "close", [evt.code, evt.reason] );
		Protocol.ws = null;
		if( evt.code === 1000 ) this.connect();
		else setTimeout( this.connect, 5000 );
	}

	static onmessage( evt ) {
		console.log( "got:", this, evt );
		const msg = JSOX.parse( evt.data );
		if( !this.on( msg.op, msg ) ){
			Protocol.debug && console.log( "Unhandled message:", msg );
		}
	}

	send( msg ) {
		if( Protocol.ws.readyState === 1 ) {
			if( "object" === typeof msg ) {
				Protocol.ws.send( JSOX.stringify(msg) ); 
			} else
				Protocol.ws.send( msg );	
		} else {
			Protocol.debug && console.log( "Protocol socket is not in open readystate", Protocol.ws.readyState );
		}
	}
	close( code, reason ) {
		return this.ws.close( code, reason );
	}

} 

