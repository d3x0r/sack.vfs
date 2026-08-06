
import {sack} from "sack.vfs"
import {openServer} from "./server.mjs"
import {Events} from "../events/events.mjs";
const JSOX = sack.JSOX;
const debug_ = false;

// Heartbeat ops.  These are ordinary data messages, NOT websocket ping/pong
// control frames: a browser's WebSocket API gives script no access to control
// frames at all (the user agent answers pings itself, with no event and no way
// to send one), so a control-frame ping can never tell a browser client that the
// server has gone away.  Driven from the server, socket.io style - the client
// never has to originate liveness traffic, it just notices the absence of a ping
// it was promised.  Must match the values in client-protocol.js.
// Sent as a bare string rather than a JSOX object - it would decode to a string
// anyway, and this skips the parser entirely for heartbeat traffic.
// NOTE 🏓 is U+1F3D3, ABOVE the BMP, so it is a surrogate PAIR in a JS string:
// "🏓".length === 2 and msg[0] is only the high surrogate.  Test with startsWith
// (or compare code points), never msg[0] === PING.  ⚪ is U+26AA and IS in the
// BMP, so it is one code unit - the asymmetry is why an index test appears to
// half-work.
const PING = "🏓"; // server -> client (the serve)
const PONG = "⚪"; // client -> server (the ball comes back)
const DEFAULT_PING_INTERVAL = 25000; // how often the server serves
const DEFAULT_PING_TIMEOUT = 20000;  // extra grace before declaring the peer gone
// Cadence rides along as code points.  Biased into the astral planes so the value
// can never land in the surrogate block (U+D800-U+DFFF): a lone surrogate is not
// valid UTF-8 and comes back as U+FFFD, which would silently mangle any interval
// between 55296 and 57343 ms.  Biasing also keeps every payload character a
// surrogate pair, so decoding by code point is uniform.
const HB_BIAS = 0x10000;
const hbEncode = ( ms )=>String.fromCodePoint( ms + HB_BIAS );
// Compare first code point rather than msg[0] (surrogate pairs) and rather than
// spreading every message - [...msg] is O(length) and would allocate an element
// per character of a 200KB body just to look at its first one.  Spread only
// inside the heartbeat branch, where the string is three characters.
const PING_CP = PING.codePointAt( 0 );
const PONG_CP = PONG.codePointAt( 0 );

function loopBack( that, to ) {

	return function f( ws ) {
		to.call(that, this, ws);
	}
}

export class Protocol extends Events {
	protocol = null;
	server = null;
	#opts = null;
	get opts() { return this.#opts }
	#keepAlive = false;
	static #WS = null;
	/** 
	 * @param {object} opts - options for the protocol
	 */ 
	constructor( opts ) {
		super();
		this.#opts = opts|| { resourcePath:"ui", port:Number(process.env.PORT)||4321 };
		// resource path is from current working directory (where it ran from)
		if( opts && opts.protocol ) Protocol.protocol = opts.portocol;
		if( "WS" in opts ) {
			Protocol.#WS = opts.WS;
		}
		this.server = openServer( this.#opts 
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
		const myWS = Protocol.#WS?new Protocol.#WS(ws, this) : new WS( ws, this );
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
			if( heartbeat ) { clearInterval( heartbeat ); heartbeat = null; }
			this_.on( "close", [myWS,code,reason] );
			myWS.on("close", [code,reason]);
		}

		function handleMessage( msg ) {
			// must be myWS, not ws: the heartbeat interval above tests myWS.lastPong,
			// and since these return before parser.write() the message never reaches
			// #dispatchMessage - stamping the raw socket instead would leave
			// myWS.lastPong frozen and close every client at interval+timeout.
			if( myWS.keepAlive ) {
				myWS.lastPong = Date.now(); 
				const cp = msg.codePointAt( 0 );
				if( cp === PONG_CP ){ return; }
				if( cp === PING_CP ){ myWS.send( PONG ); return; }
			}
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
		// heartbeats are intercepted in handleMessage before the parser ever sees
		// them, so nothing heartbeat-shaped reaches here; msg is a parsed object.
		debug_ && console.log( "invoking handler for:", msg.op, msg )
		protocol.on( msg.op, [ws, msg] );
	}
	addFileHandler( ) {
		//console.log( "Adding websocket handler for 'get'" );
		this.on( "get", (myWS,msg)=>{
			let response = {
				headers:null,
				content:null,
				status : 0,
				statusText : "Ok",
			}
			// this gets passed to 
			const url = new URL( msg.url );
			debug_ && console.log( "url parts:", url, url.message );
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
					//console.log( "Reply with got and content?", response );
					myWS.send( { op:"got", id:msg.id, response } );
				},
			} );
		} );
	}
}




export class WS extends Events{
	ws = null;
	#protocol = null; 
	#keepAlive = false;
	#heartbeat = 0;
	constructor(ws,protocol){
		super();
		this.ws = ws;
		this.#protocol = protocol;
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


	set keepAlive( val ) {
		this.#keepAlive = val;
		// begin/clear timer loop
		if( val ) {
			// heartbeat; pingInterval:0 (or false) disables it entirely.
			const pingInterval = ( "pingInterval" in this.#protocol.opts )
				? this.#protocol.opts.pingInterval : DEFAULT_PING_INTERVAL;
			const pingTimeout = ( "pingTimeout" in this.#protocol.opts )
				? this.#protocol.opts.pingTimeout : DEFAULT_PING_TIMEOUT;
			if( this.#keepAlive )
				if( pingInterval > 0 ) {
					myWS.lastPong = Date.now();
					this.#heartbeat = setInterval( ()=>{
						if( ( Date.now() - myWS.lastPong ) > ( pingInterval + pingTimeout ) ) {
							debug_ && console.log( "peer missed the heartbeat; closing" );
							clearInterval( this.#heartbeat );
							this.#heartbeat = null;
							try { ws.close( 1001, "no response to heartbeat" ); } catch( err ) { }
							return;
						}
						// tell the client the cadence so it can arm its own watchdog without
						// being configured separately
						try { myWS.send( PING + hbEncode( pingInterval ) + hbEncode( pingTimeout ) ); }
						catch( err ) { }
					}, pingInterval );
					if( this.#heartbeat.unref ) this.#heartbeat.unref(); // don't hold the process open
				}
		} else {
			if( this.#heartbeat ) { clearInterval( this.#heartbeat ); this.#heartbeat = null; }
		}
	}
	get keepAlive( ) {
		return this.#keepAlive;
	}

}

//export const protocol = new Protocol();
