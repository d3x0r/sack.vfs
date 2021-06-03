import {sack} from "sack.vfs"

const JSOX = sack.JSOX;

const storage = sack.ObjectStorage( "storage-core.os" );

import {ObjecStorage} from "../../../object-storage-remote.mjs";

import {openServer} from "../../testWsHttp.mjs"

const config = sack.JSOX.parse( sack.Volume().read("config.jsox").toString()  );

const clients = [];
const stores = [];  
const waiting = []; // requests that are outstanding.

openServer( {
	port : 8088,
	accept(ws,server) {
		const protocol = ws.headers["Sec-WebSocket-Protocol"];
		const url = ws.url;
		console.log( "Connection for:", url, protocol );
		server.accept();
	},
	connect(ws) {

		const protocol = ws.headers["Sec-WebSocket-Protocol"];
		const url = ws.url;
		console.log( "Connection for:", url, protocol );
		if( protocol === "org.d3x0r.sack.vfs.storage" )
			clients.push( new ClientConnection(ws) );
	},
        
} );        

/*
		if( msg.op === "GET" )
					if( req ) req.res( data );
			if( msg.op === "Get" ) 
					if( req ) req.rej( msg.err );
			if( msg.op === "PUT" ) 
                                            req.res( msg.r );
			if( msg.op === "Put" ) 
                                            req.rej( msg.err );
*/


class StorageConnection {
	holdoff = 50;  
	lastreply = 0; // assume everyone is 0 at first
	lastreject = 0; // any response will be worse than this.

	constructor( addr ) {
		function open() {
			const client = sack.WebSocket.Client( addr, "org.d3x0r.sack.vfs.storage", { perMessageDeflate: false } );
			console.log( "connect to ", addr );
			client.on( "open", ()=>{
				//console.log( "Connected" );
				//console.log( "ws: ", this );
				this.on( "message", ( msg_ )=>{
					const now = Date.now();
					const msg = JSOX.parse( msg_ );
					let foundItem = null;
					let reject = false;
					let index = pending.findIndex( (item)=>{
						if( item.msg.id === msg.id ) {
							if( msg.op === "GET" ) {
								foundItem = item;
								this.lastreply = now - item.now;
							} else if( msg.op === "Get" ) {
								foundItem = item;
								this.lastreject = now - item.now;
								reject = true;
							} else if( msg.op === "Put" msg.op === "PUT" ) {
							}
							// this is the socket to send to...
							item.ws.send( msg_ );
							for( let store of item.stores ) {
								// holdoff timer had not yet fired; clear timer before sending.
								if( item.timer ) clearTimeout( item.timer );
							}
							return true;
						}
						return false;
					} );

					if( index >= 0 ) {
						pending.splice( index, 1 );
					}
					// the first best response is used... 
					// but we re-order storages next time
					// adjust holdoff?
					if( foundItem )  {
						let worse = -1;
						for( let i = 0; i < stores.length; i++ ) {
							const store = stores[i];
							if( store === this ) {
								if( worse >= 0 ) {
									stores[i] = stores[worse];
									stores[worse] = this;
								}
								else if( i < (stores.length-1) ) {
									// this was better, but might be behind a better still...
									// push this back one slot, if this is still better it'll get promoted again
									stores[i] = stores[i+1];
									stores[i+1]= this;
								}
								break;
							}
							else if( reject ) {
								if( worse < 0 && store.lastreject > this.lastreject  ) {
									worse = i;
								}
							} else {
								if( worse < 0 && store.lastreply > this.lastreply  ) {
									worse = i;
								}
							}
						}
					}
                        	} );
				this.on( "close", ( msg )=>{
                        		console.log( "opened connection closed" );
					let idx = stores.findIndex( c=>c===this );
					if( idx >=0 ) stores.splice( idx );
                        	        setTimeout( open, 3000 )
			        } );
				//client.send( "Connected!" );
				//client.send( msg );
				client.send( msgtext );
                                client.send( "." );
			} );
        
		}
		open();
	}	
}


class ClientConnection {
	ws = null;
	constructor( ws ) {
		this.ws = ws;
		ws.on("message", msg_=>{
			const msg = JSOX.parse( msg_ );
			if( msg.op === "get" 
			  || msg.op === "put" 
			  ) {
				const pending = { ws:ws, stores:[], msg:msg };
				stores.forEach( (store,idx)=>{					
					const request = { timer:0, store:store, now = Date.now(); };
					pending.stores.push( request );
					request.timer = setTimeout( ()=>{ request.timer = 0; pending.ws.send( msg_ ); }, store.holdoff*idx );
				} );
				if( stores.length )
					waiting.push( pending );
			}
		} );
		ws.on("close", this.handleClose.bind( this,ws ) );
	}

	handleClose( ws ) {
		this.ws = null;
		let idx = clients.findIndex( c=>c===this );
		if( idx >=0 ) clients.splice( idx );
	}

	

}

