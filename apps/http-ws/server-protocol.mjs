
import {sack} from "sack.vfs"
import {openServer} from "./server.mjs"
import {Events} from "../events/events.mjs";
const JSOX = sack.JSOX;

function loopBack( that, to ) {

	return function f( ws ) {
		to.call(that, this, ws);
	}
}

export class Protocol extends Events {

	protocol = null;
	server = null;
	constructor( opts ) {
		super();
		// resource path is from current working directory (where it ran from)
		if( opts && opts.protocol ) Protocol.protocol = opts.portocol;
		this.server = openServer( opts || { resourcePath:"ui", port:Number(process.env.PORT)||4321 }
					, loopBack( this, this.#accept ), loopBack( this, this.#connect ) );
	}

	#accept( server, ws ) {
		//console.log( "this, server, ws", this, server, ws );
		const results = this.on( "accept", ws );
		if( results && results.length > 0 ) {
			if( results.includes( true ) ) server.accept();
			else server.reject();
			return;			
		}
		const protocol = ws.headers['Sec-WebSocket-Protocol'] || (ws.headers['Sec-Websocket-Protocol'] /* horray for heroku*/);
		if( this.protocol && protocol != this.protocol ) {
			console.log( "protocol failed:", protocol. Protocol.protocol );
			this.reject();
			return;
		}

		server.accept();
	}

	#connect(ws) {
		const myWS = new WS( ws );
		const this_ = this;
		const results = this.on( "connect", [ws, myWS] );
		if( results && results.length ) {
			// assume the on-connect provdies its own open/close handlers
			if( results.includes( true ) ) return;
		}

		ws.on("message", handleMessage);
		ws.on("close", handleClose );
		const parser = sack.JSOX.begin( 
			(object)=>Protocol.dispatchMessage(this_, myWS,object) );
		function handleClose( code, reason ) {
			this_.on( "close", [code,reason] );
		}

		function handleMessage( msg ) {
			const result = this_.on( "message", [ws,msg])
			if( !result || ! result.reduce( (acc,val)=>acc|=val, false ) )
				parser.write( msg );
		}
	}
	static dispatchMessage(protocol, ws, msg ) {
		protocol.on( msg.op, [ws, msg] ); 
	}
}

class WS{
	ws = null;
	constructor(ws){
		this.ws = ws;
	}
	send( msg ) {
		if( "object" === typeof msg ) 
			this.ws.send( JSOX.stringify(msg) ); 
		else
			this.ws.send( msg );	
	}
}

//export const protocol = new Protocol();
