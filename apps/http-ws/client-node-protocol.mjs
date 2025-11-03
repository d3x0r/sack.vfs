
import {sack} from "sack.vfs";
import {Events} from "../events/events.mjs"
const JSOX = sack.JSOX;


export class Protocol extends Events {
	static debug = true;
	ws = null;
	protocol = null;
	constructor( address, protocol ){
		super();
		this.address = address;
		this.protocol = protocol;
		if( protocol )
			this.connect(protocol, this);
	}
	set debug(val) { Protocol.debug = val; }
	get debug() { return Protocol.debug; }

	connect(protocol, this_) {
		if( this_.ws ) this_.ws.close( 1006, "close duplicate socket" );
		this_.ws = new sack.WebSocket.Client( this_.address, protocol );
		this_.ws.onmessage = (msg)=>this_.onmessage(msg) ;
		this_.ws.onclose = (code,reason)=>this_.onclose(code,reason) ;
		this_.ws.onopen = (evt)=>this_.onopen( evt) ;
		this_.ws.onerror = (evt)=>this_.onerror(evt) ;
		return this_.ws;
	}
	
	onopen( evt ) {
		this.on( "open", true );
	}

	onerror( evt ) {
		//console.log( "Socket error:", evt );
		this.on( "error", evt );
	}
	onclose( evt,reason ){
		//console.log( "Socket close:", evt );
		Protocol.debug && console.log( "close?", this, evt );
		this.on( "close", [evt, reason] );
		this.ws = null;
		if( evt.code === 1000 ) this.connect();
		else setTimeout( ()=>this.connect( this.protocol,this), 5000 );
	}

	onmessage( evt ) {
		Protocol.debug && console.log( "got:", this, evt );
		const msg = JSOX.parse( evt.data );
		if( !this.on( msg.op, msg ) ){
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

