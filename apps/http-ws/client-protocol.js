
import {Events} from "../events/events.mjs"
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"


export class Protocol extends Events {

	static ws = null;
	this.protocol = protocol;
	constructor( protocol ){
		super();
		this.protocol = protocol;
		if( protocol )
			Protocol.connect(protocol);
	}

	static connect(protocol) {
		Protocol.ws = new WebSocket( location.origin.replace("http","ws"), protocol );
		Protocol.ws.onmessage = Protocol.handleMessage;
		Protocol.ws.onclose = Protocol.onclose;
		Protocol.ws.onopen = Protocol.onopen;
		return Protocol.ws;
	}
	
	connect() {
		return Protocol.connect( this.protocol );
	}

	static onopen( evt ) {
		protocol.on( "open", true );
	}

	static close( evt ){
		protocol.on( "close", [evt.code, evt.reason] );
		Protocol.ws = null;
		if( evt.code === 1000 ) Protocol.connect();
		else setTimeout( Protocol.connect, 5000 );
	}

	static handleMessage( evt ) {
		const msg = JSOX.parse( evt.message );
		if( !protocol.on( msg.op, msg ) ){
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

