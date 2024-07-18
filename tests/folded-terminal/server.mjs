
import {sack} from "sack.vfs"
const JSOX = sack.JSOX;

import {Protocol} from "sack.vfs/protocol"

const clientMap = new WeakMap();

class Client {
	ws = null;
        task = null;
}

class FoldingServer extends Protocol {

	constructor() {
        	super( { port:Number(process.env.PORT) || 5555
			       , protocol:"FoldingTerminal"
			       , npmPath:"../.."
			       } );
                this.on( "connect", this.connect.bind( this ) );
                this.on( "close", this.close.bind( this ) );
                this.on( "run", this.run.bind( this ) );
                this.on( "input", this.input.bind( this ) );
        }
        
		close( code, reason ) {
		}
		connect( ws, wrappedWS ){
			const client = new Client();
			client.ws = wrappedWS;
			clientMap.set( wrappedWS, client );
		}
        
		run( ws, msg ) {
			const client = clientMap.get( ws );
			console.log( "msg:", msg );
			client.task = sack.Task( { bin:msg.bin
                	, input( buf ) {
                        	ws.send( JSOX.stringify( {op:"data", data:buf } ) );
                        }
                        , end() {
                        	ws.send( JSOX.stringify( {op:"end"} ) );
                        	client.task = null;
                        }
                } );
        }
        input( ws, msg ) {
        	const client = clientMap.get( ws );
			console.log( "Write ?", msg );
			if( client.task ) client.task.write( msg.input );        	
        }
}

const server = new FoldingServer();

console.log( "blah?" );