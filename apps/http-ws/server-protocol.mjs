
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
		//console.log( "--------------- NEW CONNECTION ------------------" );
		const results = this.on( "connect", [ws, myWS] );
		ws.onmessage = handleMessage;
		ws.onclose = handleClose;

		const parser = sack.JSOX.begin( 
			(object)=>Protocol.#dispatchMessage(this_, myWS,object) );


		if( results && results.length ) {
			// assume the on-connect provdies its own open/close handlers
			if( results.includes( true ) ) return;
		}

		function handleClose( code, reason ) {
			this_.on( "close", [myWS,code,reason] );
			myWS.on("close", [code.reason]);
		}

		function handleMessage( msg ) {
			const result = this_.on( "message", [ws,msg])
			//console.log( "handle message:", result, msg );
			if( !result || ! result.reduce( (acc,val)=>acc|=!!val, false ) ) {
				const res2 = myWS.on("message",[ws,msg]);
				//console.log( "socket handler?", res2, msg )
				if( !res2 || ! res2.reduce( (acc,val)=>acc|=!!val, false ) )
					parser.write( msg );
			}
		}
	}
	static #dispatchMessage(protocol, ws, msg ) {
		console.log( "invoking handler for:", msg.op, msg )
		protocol.on( msg.op, [ws, msg] ); 
	}
	addFileHandler( ) {
		//console.log( "Adding websocket handler for 'get'" );
		this.on( "get", (myWS,msg)=>{
			console.log( "gotgot:", msg )
try {
			let response = {
				headers:null,
				content:null,
				status : 0,
				statusText : "Ok",
			}
			// this gets passed to 
			const url = new URL( msg.url );
			console.log( "url parts:", url, url.message );
			this.server.handleEvent ( {url:url.pathname,
						connection: {
							headers:{}, remoteAddress:"myRemote" }
				}, {
				set statusText(val) {
					response.statusText = val;
				},
				get statusText() {
					return response.statusText;
				},
				writeHead(A,B) {
					response.status = A;
					response.headers = B;
				},
				end( content ) {
					response.content = content;
					console.log( "Replywith got and content?", response );
					myWS.send( { op:"got", id:msg.id, response } );
				},
			} );
}catch(err) { console.log( "error?", err ); }
		} );
	}
}




class WS extends Events{
	ws = null;
	constructor(ws){
		super();
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
