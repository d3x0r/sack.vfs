
import {Events} from "../events/events.mjs"
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"


export class Protocol extends Events {

	static ws = null;
	protocol = null;
	constructor( protocol ){
		super();
		this.protocol = protocol;
		if( protocol )
			Protocol.connect(protocol, this);
	}

	static connect(protocol, this_) {
		Protocol.ws = new WebSocket( location.origin.replace("http","ws"), protocol );
		Protocol.ws.onmessage = (evt)=>Protocol.onmessage.call( this_, evt) ;
		Protocol.ws.onclose = (evt)=>Protocol.onclose.call( this_, evt) ;
		Protocol.ws.onopen = (evt)=>Protocol.onopen.call( this_, evt) ;
		return Protocol.ws;
	}
	
	connect() {
		return Protocol.connect( this.protocol, this );
	}

	static onopen( evt ) {
		this.on( "open", true );
	}

	static onclose( evt ){
		console.log( "close?", this, evt );
		this.on( "close", [evt.code, evt.reason] );
		Protocol.ws = null;
		if( evt.code === 1000 ) this.connect();
		else setTimeout( this.connect, 5000 );
	}

	static onmessage( evt ) {
	console.log( "got:", this, evt );
		const msg = JSOX.parse( evt.data );
		if( !this.on( msg.op, msg ) ){
			console.log( "Unhandled message:", msg );
		}
	}

	send( msg ) {
		if( Protocol.ws.readyState === 1 ) {
			if( "object" === typeof msg ) 
				Protocol.ws.send( JSOX.stringify(msg) ); 
			else
				Protocol.ws.send( msg );	
		} else {
			console.log( "Protocol socket is not in open readystate", Protocol.ws.readyState );
		}
	}

} 

