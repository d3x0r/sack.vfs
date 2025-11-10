
import {sack} from "sack.vfs";
import {Events} from "../events/events.mjs"
const JSOX = sack.JSOX;


export class Protocol extends Events {
	static debug = false;
	ws = null;
	protocol = null;
	#processMessage = null;
	constructor( address, protocol ){
		super();
		this.address = address;
		this.protocol = protocol;
		if( protocol && address )
			this.connect(protocol, this);
	}
	set debug(val) { Protocol.debug = val; }
	get debug() { return Protocol.debug; }

	static connect(protocol, this_) {
		Protocol.debug && console.trace( "ws is already set?", this_.ws, this_.address, protocol );
		if( this_.ws ) this_.ws.close( 3000, "close duplicate socket" );
		this_.ws = new sack.WebSocket.Client( this_.address, protocol );
		this_.ws.onmessage = this_.onmessage.bind( this_, this_.ws );
		this_.ws.onopen = this_.onopen.bind( this_, this_.ws ) ;
		this_.ws.onerror = this_.onerror.bind( this_, this_.ws ) ;
		// onclose might have already happened, and will get called while this is dispatched.
		this_.ws.onclose = this_.onclose.bind( this_, this_.ws ) ;
		return this_.ws;
	}

	connect() {
		Protocol.debug && console.log( "Calling connect...", this.protocol, this.address );	
		return Protocol.connect( this.protocol, this );
	}
	
	onopen( ws, evt ) {
		this.on( "open", ws );
	}

	onerror( ws, evt ) {
		//console.log( "Socket error:", evt );
		this.on( "error", [ws,evt] );
	}
	onclose( ws, evt,reason ){
		//console.log( "Socket close:", evt );
		Protocol.debug && console.log( "close?", this, evt );
		this.on( "close", [ws, evt, reason] );
		if( this.ws === ws ) {
			this.ws = null;
			if( evt.code === 1000 ) this.connect();
			else setTimeout( ()=>this.connect( this.protocol,this), 5000 );
		}
	}

	set processMessage( val ) { this.#processMessage = val }
	onmessage( ws, buffer ) {
		Protocol.debug && console.log( "got:", this, ws, buffer );
		const msg = JSOX.parse( buffer );
		if( this.#processMessage && this.#processMessage( ws, msg, buffer ) ) return;
		//console.log( "this was overridden?", msg, buffer );
		if( !this.on( msg.op, [ws,msg] ) ){
			console.log( "Unhandled message:", msg );
		}
	}

	send( msg ) {
		if( this.ws.readyState === 1 ) {
			if( "object" === typeof msg ) 
				this.ws.send( JSOX.stringify(msg) ); 
			else
				this.ws.send( msg );	
		} else {
			console.log( "Protocol socket is not in open readystate", this.ws.readyState );
		}
	}
} 

