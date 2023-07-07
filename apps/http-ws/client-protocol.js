
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
} 

export const protocol = new Protocol();
