
"use strict";
const _debug = false;

// build script requires this sort of path...
import {JSOX} from "../../../../../../node_modules/jsox/lib/jsox.mjs"
import {SaltyRNG} from "../../../../../../node_modules/@d3x0r/srg/salty_random_generator.js"

const generator = SaltyRNG.id;
const regenerator = SaltyRNG.id;
const short_generator = SaltyRNG.Id;

const JSON = JSOX;

const connections = new Map();


export {makeProtocol};

function makeProtocol( client ) {

	function send(msg) {
	    client.postMessage( msg );
	}

	function handleServiceMessage(e,msg) {
		//const msg = e.data;
		if( "string" === typeof msg ) {
			console.log( "String message??", msg );
			//return wsAuth.send( msg );
		}
                //console.log( "Worker received from main:", msg );
                if( msg.op === "connect" ) {
                	const connection = makeSocket()
			_debug && console.log( "SETTING PROTOCOL: ", connection.id );
			protocol_.connectionId = connection.id;
	        
			e.source.postMessage( {op:"connecting", id:connection.id } );
	        
			connection.ws = protocol.connect( msg.address, msg.protocol, 
				(msg)=>e.source.postMessage({op:"b",id:connection.id,msg:msg })
			);
			
		}else if( msg.op === "connected" ) {
			const socket = connections.get( msg.id );
			_debug && console.log( "this is on the service side and shouldn't happen" );
		}else if( msg.op === "send" ) {
			const socket = connections.get( msg.id );
			if( socket ) socket.ws.send( msg.msg );
			//else throw new Error( "Socket to send to is closed:"+msg.id );
		}else if( msg.op === "close" ) {
			const socket = connections.get( msg.id );
			if( socket ) socket.ws.close();
			//else throw new Error( "Socket to close to is closed:"+msg.id );
                }else if( msg.op === "serviceReply" ) {
			_debug && console.log( "New worker, handling new connection, direct" );
			const newSock = makeSocket();
			protocol_.connectionId = newSock.id;
			_debug && console.log( "SETTING PROTOCOL(SR): ", newSock.id );
					        		
			newSock.ws = openSocket( msg.service, (msg,ws)=>{
					if( msg.op === "status" ) { 
						// op.status
						if( ws ){
				                        send( {op:'b',id:ws.id,msg:msg} )
							//send( {op:'a',id:ws.id,msg:msg} );
                                                }
						return;
					}
					else if( msg === true ) {
						//console.log( "This should be a blank service: Auth was?", msg,ws );
				                send( {op:"connecting", id:ws.id} )
						//send( {op:"connecting", id:ws.id} );
					}
                                        else if( msg.op === "disconnect" ) {
                                            	send( msg );
                                        }
					else console.log( "Unhandled connect message:", msg );
					//console.log( "Socket reply(service side)", ws, msg, msg_ );
				}, msg.id, "wss://"+msg.address+":"+msg.port+"/" );
                }else {
			console.log( "Unhandled message:", msg );
			return false; 
		}
		return true;
	}





	const protocol = {
		connect : connect,
		//login : login,
		connectTo : connectTo,
		handleServiceMessage : handleServiceMessage,
		serviceLocal : null,
		connected : false,
		loggedIn : false,
		doneWithAuth : false,
		username : null,
		userkey : null,
		connectionId : null,
		resourceReply : null,
		requestKey(ident,cb) { wsAuth.requestKey( ident,cb )},
		closeAuth() { wsAuth.close(1000, "done"); },
                send(sock,msg){
                    	if( "object" === typeof msg ) msg = JSOX.stringify( msg );
                	const socket = connections.get( sock );
                        if( socket ) socket.ws.send( msg );
                },
		relogin( service, cb ) { 
			wsAuth.relogin( (user,message,reset)=>{
				if( user === false ) {
					cb( false, message );
					//pendingServiceRequest = false;
				} else {
				protocol.loggedIn = true;
				protocol.username = reset;
				protocol.userid = message;
	        
				requestService(service, null, null, (msg,data)=>{
					if( !msg ) {
						cb( false, data );
						return;
					} else {
						cb( msg, data );
					}
					//cb();
				})
				}
			} ); 
		},
		createUser(a,b,c,d,e ) {
			wsAuth.createUser(a,b,c,d,e);
		}

	};

	const protocol_ = protocol; // this is a duplicate because openSocket has parameter 'protocol'

	var connectEventHandler;


	function connect(addr,proto, cb) {
		_debug && console.log( "calling connect?" );
		connectEventHandler = cb;
		return openSocket( proto, cb, null, addr );
	}


	function makeSocket( ) {
		const sock = {
				ws : null, // wait until we get a config to actually something...
				id : short_generator()
			};
		connections.set( sock.id, sock );
		return sock;
	}


	function openSocket( protocol, cb, conId, peer ) {
		let redirected = false;
		//var https_redirect = null;
		let connected = false;
		let ws;

		cb( { op:"status", status:"connecting..." + " " + protocol } );

		ws = new WebSocket( peer, protocol );
		//console.log( "New connection ID:", protocol_.connectionId );
		
		ws.id = protocol_.connectionId;
		protocol_.connectionId = null;
	        
		cb( { op:"opening", ws:ws.id } );

                //console.log( "Got websocket:", ws, Object.getPrototypeOf( ws ) );
		ws.onopen = function() {
			connected = true;
			cb( { op:"open", status: "Opened...." }, ws);
		};
		ws.onmessage = function handleSockMessage(evt) {
			const msg_ = evt.data;
			if( msg_[0] === '\0' ) {
				const msg = JSOX.parse( msg_.substr(1) ); // kinda hate double-parsing this... 
				if( msg.op === 'GET' ) {
					if( protocol_.resourceReply )
						protocol_.resourceReply( client, msg );
					return;
				}
			} else
				send( {op:'a', id:ws.id, msg:msg_ } ); // just forward this.
		};
		ws.onclose = doClose;
		function doClose(status) {
	        
			if( protocol.serviceLocal ) {
				if( protocol.serviceLocal.uiSocket === ws.socket ) {
                                    	console.log( "clearing ui Socket so it doesn't send?" );
					protocol.serviceLocal.uiSocket = null;
                                }
			}
	        
			connections.delete( ws.id );
			console.log(" Connection closed...", status, ws.id );
	        
			cb( { op:"status", status: "Disconnected... waiting a moment to reconnect..." }, ws);
			cb( { op:"disconnect", id:ws.id }, ws )
                	// websocket is closed.
		};
		return ws;
	}

	function abortLogin( ) {
		if( loginCallback ) {
			loginCallback( false, "Timeout" );
			loginCallback = null;
		}
	}

	function connectTo( addr, service, sid, cb ) {
		openSocket( service, cb, sid, addr );
	}


	return protocol;

}

