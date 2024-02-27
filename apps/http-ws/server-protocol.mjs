
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
	/** 
	 * @param {object} opts - options for the protocol
	 */ 
	constructor( opts ) {
		super();
		// resource path is from current working directory (where it ran from)
		if( opts && opts.protocol ) Protocol.protocol = opts.portocol;
		this.server = openServer( opts || { resourcePath:"ui", port:Number(process.env.PORT)||4321 }
					, loopBack( this, this.#accept ), loopBack( this, this.#connect ) );
		//this.on( "close", )
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
			(object)=>Protocol.#dispatchMessage(this_, myWS,object) );

		function handleClose( code, reason ) {
			this_.on( "close", [myWS,code,reason] );
		}

		function handleMessage( msg ) {
			const result = this_.on( "message", [ws,msg])
			if( !result || ! result.reduce( (acc,val)=>acc|=!!val, false ) )
				parser.write( msg );
		}
	}
	static #dispatchMessage(protocol, ws, msg ) {
		protocol.on( msg.op, [ws, msg] ); 
	}
}

class WS{
	ws = null;
	constructor(ws){
		this.ws = ws;
	}
	/**
	 * send a message - with automatic JSOX encoding if the message is an object.
	 * @param {*} msg - message to send to the server, if an object, it will be sent as a JSOX object, otherwise it will be sent as a literal string.
	 */
	send( msg ) {
		if( "object" === typeof msg ) 
			this.ws.send( JSOX.stringify(msg) ); 
		else
			this.ws.send( msg );	
	}
	/**
	 * emit an event to the server
	 * @param {*} cmd - string command to genereate
	 * @param {*} data - data to send with the command, if an object, it will be sent as a JSOX object,
	 *                   otherwise it will be sent as a JSOX object with the key being the command.
	 */
	emit( cmd, data ){
		if( "object" === typeof data ) {			
			this.ws.send( JSOX.stringify(Object.assign( {op:cmd}, data )) );
		} else
			this.ws.send( JSOX.stringify({ op:cmd, [cmd]:data }) );
	}
}

//export const protocol = new Protocol();
