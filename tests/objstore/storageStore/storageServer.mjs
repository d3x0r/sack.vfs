import {sack} from "sack.vfs"

const JSOX = sack.JSOX;

const storage = sack.ObjectStorage( "storage-core.os" );

import {openServer} from "../../testWsHttp.mjs"

openServer( {
	port : 8088,
	accept(ws,server) {
		const protocol = ws.headers["Sec-WebSocket-Protocol"];
		const url = ws.url;
		console.log( "Connection for:", url, protocol );
		server.accept();
	},
	connect(ws) {
		new Connection(ws);
	},
        
} );        

class Connection {
	ws = null;
	constructor( ws ) {
		this.ws = ws;
		ws.on("message", msg_=>{
			const msg = JSOX.parse( msg_ );
			try {
				if( !storage.handleMessage( ws, msg ) )
					this[msg.op](ws,msg);
			} catch(err) {
				console.log( "protocol error:", err );
			}
		} );
		ws.on("close", this.handleClose.bind( this,ws ) );
	}

	handleClose( ws ) {
		this.ws = null;
	}

	

}

