
import {sack} from "sack.vfs";
import {Events} from "../events/events.mjs"
const JSOX = sack.JSOX;


export class Protocol extends Events {
	static debug = true;
	ws = null;
	protocol = null;
	constructor( protocol ){
		super();
		this.protocol = protocol;
		if( protocol )
			Protocol.connect(protocol, this);
	}
	set debug(val) { Protocol.debug = val; }
	get debug() { return Protocol.debug; }

	static connect(protocol, this_) {
		if( this.ws ) this.ws.close( 1006, "close duplicate socket" );
		this.ws = new sack.WebSocketClient( location.origin.replace("http","ws"), protocol );
		this.ws.onmessage = (evt)=>Protocol.onmessage.call( this_, evt) ;
		this.ws.onclose = (evt)=>Protocol.onclose.call( this_, evt) ;
		this.ws.onopen = (evt)=>Protocol.onopen.call( this_, evt) ;
		return this.ws;
	}
	
	connect() {
		return this.connect( this.protocol, this );
	}

	static onopen( evt ) {
		this.on( "open", true );
	}

	static onclose( evt ){
		Protocol.debug && console.log( "close?", this, evt );
		this.on( "close", [evt.code, evt.reason] );
		this.ws = null;
		if( evt.code === 1000 ) this.connect();
		else setTimeout( this.connect, 5000 );
	}

	static onmessage( evt ) {
		Protocol.debug && console.log( "got:", this, evt );
		const msg = JSOX.parse( evt.data );
		if( !this.on( msg.op, msg ) ){
			console.log( "Unhandled message:", msg );
		}
	}

	send( msg ) {
		if( Protocol.ws.readyState === 1 ) {
			if( "object" === typeof msg ) 
				this.ws.send( JSOX.stringify(msg) ); 
			else
				this.ws.send( msg );	
		} else {
			console.log( "Protocol socket is not in open readystate", this.ws.readyState );
		}
	}
} 

