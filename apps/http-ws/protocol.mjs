
import {sack} from "sack.vfs"
import {openServer} from "../http-ws/serve.mjs"
import {Events} from "../events/events.mjs";


export class Protocol extends Events {

	protocol = null;
	server = null;
	constructor( { opts} ) {
		super();
		// resource path is from current working directory (where it ran from)
		if( opts && opts.protocol ) Protocol.protocol = opts.portocol;
		this.server = openServer( opts || { resourcePath:"ui", port:Number(process.env.PORT)||4321 }
					, Protocol.accept, Protocol.connect);
	}

	static accept( server ) {
		console.log( "(Debug)Argument at all?", server );
		const ws = this;
		if( protocol.on( "accept" ) ) {
			if( protocol.on( "accept", ws ).includes( true ) ) this.accept();
			else this.reject();
			return;			
		}
		const protocol = ws.headers['Sec-WebSocket-Protocol'] || (ws.headers['Sec-Websocket-Protocol'] /* horray for heroku*/);
		if( Protocol.protocol && protocol != Protocol.protocol ) {
			this.reject();
			return;
		}

		this.accept();
	}

	static connect(ws) {
		const myWS = new WS( ws );

		if( protocol.on( "connect" ) ) {
			if( protocol.on( "connect", [ws,myWS] ).includes( true ) ) this.accept();
			else this.reject();
			return;			
		}

		ws.on("message", handleMessage);
		ws.on("close", handleClose );
		const parser = sack.JSOX.begin( 
			(object)=>Protocol.dispatchMessage(myWS,object) );
		function handleClose( code, reason ) {
			this.on( "close", [code,reason] );
		}

		function handleMessage( msg ) {
			if( !this.on( "message", [ws,msg] ).reduce( (acc,val)=>acc|=val, false ) )
				parser.write( msg );
		}
	}
	static dispatchMessage(ws, msg ) {
		protocol.on( msg, [ws, msg] ); 
	}
}

class WS{
	ws = null;
	constructor(ws){
		this.ws = ws;
	}
	send( msg ) {
		if( "object" === typeof msg ) 
			ws.send( JSOX.stringify(msg) ); 
		else
			ws.send( msg );	
	}
}

//export const protocol = new Protocol();
