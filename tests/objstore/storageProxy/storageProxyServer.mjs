import {sack} from "sack.vfs"

const JSOX = sack.JSOX;

const storage = sack.ObjectStorage( "storage-proxy.os" );

import {ObjecStorage} from "../../../object-storage-remote.mjs";

import {openServer} from "../../../apps/ws-http/server.mjs"

const config = sack.JSOX.parse( sack.Volume().read("config.jsox").toString()  );
const runConfig = {
	version: 1,
	stores : [],
	"4n_stores" : [],
}
let configFile = null;
const clients = [];
const stores = [];  
const waiting = []; // requests that are outstanding.



async function init() {

	openServer( {
		port : 8088,
		accept(ws,server) {
			const protocol = ws.headers["Sec-WebSocket-Protocol"];
			const url = ws.url;
			console.log( "Connection for:", url, protocol );
			server.accept();
		},
		connect(ws, myWS) {
			const protocol = ws.headers["Sec-WebSocket-Protocol"];
			const url = ws.url;
			console.log( "Connection for:", url, protocol );
			if( protocol === "org.d3x0r.sack.vfs.storage" )
				clients.push( new ClientConnection(ws) );
		},
        
	} );        


		const root = await storage.getRoot();
		try {
			const file = await root.open( "running.config.jsox" )
			configFile = file;
			const obj = await file.read()
			Object.assign( runConfig, obj );
			
			let changed = false;
			config.stores.forEach( store =>{
				let serverConfig = null;
				for( let i = 0; i < runConfig.stores.length; i++ ) {
					if( runConfig.stores[i].store === store ){
						serverConfig = runConfig.stores[i];
						break;
					}
				}
		
				if(i === runConfig.stores.length){
					changed = true;
					serverConfig = {
						store:store,
						fails : 0,
						puts : [],
						holdoff: 50,
						lost:new Map(),
					}
					runConfig.stores.push( serverConfig );
				} else {
					// this was found in the recovered configuration already.
					
				}
				new StorageConnection( serverCOnfig, false );
			} );
			for( let i = 0; i < runConfig.stores.length; i++ ) {
				// for old stores, check to see if they are still in the config file
				// if not, then move them to a 4n_server; they will eventually die off
				// when they fail for (days?)
				const s = runConfig.stores[i].store;
				if( !config.stores.find( s=>s.store===s ) ) {
					changed = true;
					if( !runConfig["4n_stores"].find( _4n=>4n.store===s.store ) )
						runConfig["4n_stores"].push(s); 
					runConfig.stores.splice( i,1 );
					break;
				}
			}
			runConfig["_4n_stores"].forEach( store =>{
				new StorageConnection( store, true );
			} );
			if( changed )
				file.write( runConfig );
		} catch( err){
			console.log( "User Db Config ERR:", err );
			const file = await root.create( "running.config.jsox" );
			// if initial, there's no foriegn stores...
			configFile = file;
			config.stores.forEach( store =>{
				const serverConfig = {
					store:store,
					fails : 0,
					puts : [],
					holdoff: 50,
					lost:new Map(),
				}
				runConfig.stores.push(serverConfig);
				new StorageConnection( serverConfig, false );
			} );
			file.write( runConfig );
		}
}

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
	lastreply = 0; // assume everyone is 0 at first
	lastreject = 0; // any response will be worse than this.
	ws = null; 
	config = null;
	waiting = [];

	constructor( config, foriegn ) {
		this.config = config;
		function open() {
			const ws = sack.WebSocket.Client( config.store, "org.d3x0r.sack.vfs.storage", { perMessageDeflate: false } );
			console.log( "connect to ", addr );
			client.on( "open", ()=>{
				//console.log( "Connected" );
				//console.log( "ws: ", this );
				this.ws = ws;
				if( config.put.length ) {
					//console.log(  );         
					const id = config.put[0];
					console.log( "retry put" );
					stores.forEach( store=>{
						const request = { timer:0, store:store, now = Date.now(); };
						pending..push( request );
						store.ws.send( JSOX.stringify( {op:"get", id:id} ) );
					} ) 
				}

				ws.on( "message", ( msg_ )=>{
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
				ws.on( "close", ( msg )=>{
                        		console.log( "opened connection closed" );
					this.ws = null;
					if( foreign ) {
						config.fails++;
						// failed foriegn servers are slow to reconnect
	                        	        setTimeout( open, 300000 );
					}else {
						// servers in the config.jsox file reconnect 'quickly'
	                        	        setTimeout( open, 3000 );
					}
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
				const isPut = ( msg.op === "put" );
				const pending = { ws:ws, stores:[], msg:msg };
				stores.forEach( (store,idx)=>{					
					if( !store.ws ) {
						if( isPut ) {							
							store.config.puts.push( msg.id );
							configFile.write( runConfig );
						}
						// gets just quietly fail on a closed connection.
						
					} else {
						const request = { timer:0, store:store, now = Date.now(); };
						pending.stores.push( request );
						// there is a chance it might disconnect during holdoff.
						request.timer = setTimeout( ()=>{ request.timer = 0; if( pending.ws ) pending.ws.send( msg_ ); }, store.config.holdoff*idx );
					}
				} );
				if( pending.stores.length )
					waiting.push( pending );
				else {
					// this put didn't actually get sent anywhere...
					// so noone has to know.
					if( isPut ) {
						stores.forEach( (store,idx)=>{					
							if( !store.ws && isPut ) {							
									store.config.puts.length--;
							}
						} );
						configFile.write( runConfig );
						// reject put to client.
						ws.send( {op:"Put", id:msg.id, err:"No Storage" } );
					} else {
						ws.send( {op:"Get", id:msg.id, err:"No Storage" } );
					}
				}
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




