
"use strict";
const _debug = false;

import {JSOX} from "../../../../../node_modules/jsox/lib/jsox.mjs"
import {SaltyRNG} from "../../../../../node_modules/@d3x0r/srg/salty_random_generator.js"
const regenerator = SaltyRNG.Id;
const generator = SaltyRNG.Id;
const short_generator = generator;

const JSON = JSOX;

const connections = new Map();

var wsAuth;

var loginCallback;
var loginTimer = null;
var secureChannel = false;

var stage = 0;
var connectTimer = null;
var pendingServiceRequest = null;
var currentProtocol = "";
var requestTimer = null;
var timeoutAuth;

import {getStorage} from "./localStorageProxy.js"

//import {send} from "./sw.js"

export {makeProtocol};

function makeProtocol( client ) {

const storage = getStorage( send );
const localStorage = storage.localStorage;
const config = storage.config;

function send(msg) {
    client.postMessage( msg );
}

function handleMessage(e,msg) {
	//const msg = e.data;
	if( "string" === typeof msg ) {
		return wsAuth.send( msg );
	}
        //console.log( "Worker received from main:", msg );
        if( msg.op === "connect" ) {
        	const connection = makeSocket()
		_debug && console.log( "SETTING PROTOCOL: ", connection.id );
		protocol_.connectionId = connection.id;

		e.source.postMessage( {op:"connecting", id:connection.id } );

		if( !config.run.devkey )
			localStorage.getItem( "devkey" ).then( val=>{
				if( !val ) {
					config.run.devkey = generator();
					localStorage.setItem( "devkey", config.run.devkey )
				}
				config.run.devkey = val
				localStorage.getItem( "clientKey" ).then( val=>{ config.run.devkey = val;
					localStorage.getItem( "sessionKey" )
						.then( val=>config.run.devkey = val )
						.then( finishSocket )
				} );
			} )
		else
			localStorage.getItem( "clientKey" ).then( val=>{ config.run.devkey = val;
				localStorage.getItem( "sessionKey" ).then( val=>config.run.devkey = val ).then( finishSocket )
			} );

		function finishSocket() {
			_debug && console.log( "Doing actual connection..." );
			protocol.connectionId = connection.id;
			_debug && console.log( "SETTING PROTOCOL(FS): ", connection.id );
			connection.ws = protocol.connect( (msg)=>{
				e.source.postMessage({op:"a",id:connection.id,msg:msg })
			} );
		}
	}else if( msg.op === "connected" ) {
		const socket = connections.get( msg.id );
		_debug && console.log( "this is on the service side and shouldn't happen" );
		//socket.ws.send( msg.msg );
	}else if( msg.op === "send" ) {
		const socket = connections.get( msg.id );
		if( socket ) socket.ws.send( msg.msg );
		else throw new Error( "Socket is closed:"+msg.id );
        }else if( msg.op === "login" ) {
        	protocol.login( msg.user,msg.pass,(status)=>{
                	_debug && console.log( "Result of login:", status );
                        send( { op:"login", status:status } )
                } );
        }else if( msg.op === "serviceReply" ) {
		_debug && console.log( "New worker, handling new connection, direct" );
		const newSock = makeSocket();
		protocol_.connectionId = newSock.id;
		_debug && console.log( "SETTING PROTOCOL(SR): ", newSock.id );
				        		
		newSock.ws = openSocket( msg.service, 2, (msg,ws)=>{
				if( msg.op === "status" ) { 
					// op.status
					if( ws ){
			                        send( {op:'a',id:ws.id,msg:msg} )
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
    localStorage: localStorage,
	connect : connect,
	login : login,
	connectTo : connectTo,
	request : requestService,
	handleMessage : handleMessage,
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
	return openSocket( proto, 0, cb, null, addr );
}


function makeSocket( ) {
	const sock = {
			ws : null, // wait until we get a config to actually something...
			id : short_generator()
		};
	connections.set( sock.id, sock );
	return sock;
}


function openSocket( protocol, _stage, cb, passkey, peer ) {
	var redirected = false;
	//var https_redirect = null;
	var mykey = { key:generator(), step:0 }
	var myreadkey = { key:mykey.key, step:1 }
	if( !_stage )
		stage = 0;
	var connected = false;
	var ws;
	if( stage && !redirect )
		console.log( "Need to re-request service....", protocol, stage)
	//connections++;
	cb( { op:"status", status:"connecting..."+stage + " " + protocol } );
	try {
		ws = new WebSocket( (_stage == 0?peer:redirect)
			, protocol
			, _stage>0?{
				perMessageDeflate: false,
				//ca : config.caRoot
			}:null
		);
		//console.log( "New connection ID:", protocol_.connectionId );
		
		ws.id = protocol_.connectionId;
		protocol_.connectionId = null;
		redirect = null;

		if( _stage === 1 ) {
			wsAuth = ws;
		} else if( _stage > 1 ) {
			cb( true, ws );
		}
	} catch( err ) {
		console.log( "CONNECTION ERROR?", err );
		return null;
	}
        //console.log( "Got websocket:", ws, Object.getPrototypeOf( ws ) );

	function startup() {
		localStorage.getItem( "clientKey" ).then( key=>{
			
			//console.log( "key is:", typeof key, key );
			var skey = config.run.sessionKey;
			if( !key && _stage === 0 ) {
				console.log( "need key..." );
				ws.send( '{op:"getClientKey"}' );
			} else {
				if( _stage == 0 ) {
					//console.log( "request auth0" );
					ws.send( "AUTH" );
					_debug && console.log( "setup timeout auth.")
					timeoutAuth = setTimeout( ()=>{
				        	cb( { op:"status", status: " AUTH not responding..." }, ws);
						console.log( "Auth timed out..." );
					}, 5000 );
				} else {
					ws.send( passkey );
					//ws.send( `{op:"hello"}` );
				}
			}
		} );
	}

	ws.onopen = function() {
			connected = true;
			if( _stage == 0 )
				cb( { op:"status", status: "Opened...." }, ws);
			else if( _stage == 1 ) {
				cb( { op:"status", status: "Ready to login..." }, ws);
				send( {op:'a', id:ws.id, msg:{op:"connected"} } ); // just forward this.
			} else
				cb( { op:"status", status: "Connecting..." }, ws);

		// Web Socket is connected. You can send data by send() method.
		//ws.send("message to send");
		//console.log( "key is", mykey );
		//console.log( "keys:", key, skey );
		ws.send( mykey.key );
		ws.send = ((orig)=>(msg)=>{ 
			if( ws.readyState !== 1 ) return; // protect sending closed
			if( "object" === typeof msg )
				orig( JSOX.stringify(msg ) );
			else
				orig( msg );
		})(ws.send.bind(ws));
		startup();
	};
	ws.onmessage = function (evt) {
		//var tmpmsg = u8xor( evt.data, myreadkey );
		var msg = JSON.parse( evt.data );
		if( !msg ) return;
		//_debug && 
		//console.log( "got message:", protocol, _stage, msg );
		if( _stage > 0 ) {
			//console.log( "Forwarding extension message ");
			if( _stage < 3 ) {
				if( msg.op === "addMethod" ) {
					stage = _stage;
				}
				if( msg.op === 'GET' ) {
					if( protocol_.resourceReply )
						protocol_.resourceReply( client, msg );
					return;
				}
				/*
				if( msg.op === "serviceReply" ) { // really needs to go back to protocol client code...
				
					if( !msg ) {
						cb( false, data );
						return;
					}
					// {op:"serviceReply", id:"B3D2Z$EvTox_9Pf$VAot8i6wC$JZPV0rHlW8zWAjIHQ=",port:32678,address:"198.143.191.26",service:"KCHATAdmin"}
					//redirect = "wss://"+msg.address+":"+msg.port+"/";
					//https_redirect = "https://"+msg.address+":"+msg.port+"/";
					currentProtocol = msg.service;
					secureChannel = true;

					const newSock = makeSocket();
					
					//protocol_.connectionId = newSock.id;
				         
					newSock.ws = openSocket( msg.service, 3, serviceConnected, msg.id, "wss://"+msg.address+":"+msg.port+"/" );
					
					function serviceConnected( data ) {
						
						console.log( "Service connection:", data );
					}
				}
				*/
			}
			send( {op:'a', id:ws.id, msg:msg } ); // just forward this.
		} else if( _stage == 0 ) {
			//console.log( "Layer0 message", msg );
			if( msg.op === "setClientKey" ) {
				//console.log( "Got key:", msg );
				config.run.clientKey = msg.key;
				localStorage.setItem( "clientKey", msg.key );
				startup();
				return;
			}
		}
	};
	ws.onerror = function(err) {
		console.log( "Can I get anything from err?", err );
		if( _stage == 1 ) {
			//location.href=https_redirect;
		}
		if( !err.target.url.includes( "chatment.com" ) ) {
			//location.href="https://www.chatment.com"
		}
	}
	ws.onclose = doClose;
	function doClose(status) {
		if( ws === wsAuth ) wsAuth = null;
		if( protocol.serviceLocal ) {
                    console.log( "protocol ui socket also?", protocol.serviceLocal );
			if( protocol.serviceLocal.uiSocket === ws.socket ) {
                            	console.log( "clearing ui Socket so it doesn't send?" );
				protocol.serviceLocal.uiSocket = null;
                        }
		}
		connections.delete( ws.id );
		console.log(" Connection closed...", status, ws.id );
		if( status.code == 1000 ) return;

		if( !connected ) {
			//console.log( "Aborted WEBSOCKET!", step, status.code, status.reason )
			cb( { op:"status", status:"connection failing..." }, ws);
			_debug && console.log( "Setup initial connection timer." );
			setTimeout( ()=>{openSocket(protocol,_stage,cb,null,peer )}, 5000 );
			return;
	        }
		connected = false;
		
        	if( ( _stage == 0 || _stage == 2 ) && pendingServiceRequest ) {
			pendingServiceRequest(null);
			if( requestTimer ) { clearTimeout( requestTimer ); requestTimer = null; }
			pendingServiceRequest = null;
	        }
		//console.log( "CLOSED WEBSOCKET!", protocol, stage, status )
		if( redirected && _stage == 0 ) {
			// success; nothiing to do.
			return;
		} else if( redirect && _stage >= 1 ) {
			if( _stage > 1 )
				console.log( "Cannot auto-reconnect; need to re-request service" );
			else
				openSocket( currentProtocol, stage = ++_stage, cb, null, peer );
			redirect = null;
		} else {
			// stage 2 connection or non-redirect loss resets back to initial connect.			
			secureChannel = false;
			// reconnect this same protocol...
			protocol_.loggedIn = false;
			protocol_.doneWithAuth = false;
		        cb( { op:"status", status: "Disconnected... waiting a moment to reconnect..." }, ws);
			cb( { op:"disconnect", id:ws.id }, ws )
			if( !connectTimer ) {
				_debug && console.log( "reconnect from start timeout" );
				connectTimer = setTimeout( ()=>{connectTimer = null; 
					openSocket( proto, 0, connectEventHandler, null, peer )}, 5000 );
			}

		}
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
	openSocket( service, 3, cb, sid, addr );
}

function login(user,pass, cb) {
	if( !loginCallback ) {
		if( stage !== 1 ) {
			if( stage > 1 )
				console.log( "already logged in?" );
			console.log( "Login is not ready yet..." );
			cb( false, "Login is not ready yet..." );
			return;
		}
		loginCallback = cb;
		if( wsAuth && stage == 1 ) {
			//console.log( "Send login to auth0" );
			wsAuth.login( user, pass, (a,b,c)=>{
				clearTimeout( loginTimer ) ;
				loginCallback=null;
				cb(a,b,c,wsAuth) 
			} );
			loginTimer = setTimeout( abortLogin, 5000 );
		}
	} else {
		console.log( "login already in progress" );
	}
}

function timeoutRequest() {
	if( pendingServiceRequest ) {
		pendingServiceRequest( { op:"status", status:"Service not available..." } );
		wsAuth.abortRequest();
		if( requestTimer ) { clearTimeout( requestTimer ); requestTimer = null; }
		pendingServiceRequest = null;
	}
}

function requestService( service, user_id, password, cb ) {
	if( !pendingServiceRequest ) {
		currentProtocol = service;
		// callback after addMethod of anther connection happens.
		pendingServiceRequest = cb;
		
		// { msg: "login", user:user_id, pass:password, devkey: localStorage.getItem("clientKey") }

		function doRequest() {
			_debug && console.log( "SETUP TIMEOUT REQUEST" );
			requestTimer = setTimeout( timeoutRequest, 5000 );
			wsAuth.request( service, function(msg,data) {
				//console.log( "got requested service:", service, msg )
				if( !msg ) {
					cb( false, data );
					return;
				}
				// {op:"serviceReply", id:"B3D2Z$EvTox_9Pf$VAot8i6wC$JZPV0rHlW8zWAjIHQ=",port:32678,address:"198.143.191.26",service:"KCHATAdmin"}
				//redirect = "wss://"+msg.address+":"+msg.port+"/";
				//https_redirect = "https://"+msg.address+":"+msg.port+"/";
				currentProtocol = msg.service;
				secureChannel = true;
				openSocket( msg.service, 2, cb, msg.id, "wss://"+msg.address+":"+msg.port+"/" );
				//ws.close(); // trigger connect to real service...
			} );
		}

		if( user_id && password ) {
			_debug && console.log( "DOING LOGIN" );
			wsAuth.login( user_id, password, ( success, userid, username )=>{
				protocol.username = username;
				protocol.userid = userid;
				if( success ) {
					doRequest();
				} else {
					cb( { op:"status", status:userid } )
					pendingServiceRequest  = null;
				}
			} )
		} else {
			if( wsAuth ) {
				_debug && console.log( "USING EXSITING AUTH AND GETTING MORE" );
				doRequest();
			} else
				cb( { op:"status", status:"Not Logged In" } );
		}
	} else {
		pendingServiceRequest( { op:"status", status:"Service request pending..." } );
	}
	}


	return protocol;
}

//export {protocol}