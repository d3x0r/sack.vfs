
//console.log( "meta?", import.meta );

const colons = import.meta.url.split(':');
const where = colons.length===2?colons[1].substr(1):colons[2];
const nearIdx = where.lastIndexOf( "/" );
const nearPath = where.substr(0, nearIdx+1 );


import path from "path";
import {sack} from "sack.vfs"
const JSOX = sack.JSOX;
import {UserDb,User,Device,UniqueIdentifier,go} from "./userDb.mjs"

const storage = sack.ObjectStorage( "data.os" );
UserDb.hook( storage );

const methods = sack.Volume().read( nearPath+"userDbMethods.js" ).toString();
const methodMsg = JSON.stringify( {op:"addMethod", code:methods} );

const serviceMethods = sack.Volume().read( nearPath+"serviceDbMethods.js" ).toString();
const serviceMethodMsg = JSON.stringify( {op:"addMethod", code:serviceMethods} );

const serviceLoginScript = sack.Volume().read( nearPath+"serviceLogin.mjs" ).toString();

import {UserDbRemote} from "./serviceLogin.mjs";
//const methodMsg = JSON.stringify( {op:"addMethod", code:methods} );

const l = {
	newClients : [],
	services : new Map(),
}

// go is from userDb; waits for database to be ready.
go.then( ()=>{
        openServer( { port : 8089
                } );
} );


function openServer( opts, cb )
{
	const serverOpts = opts || {port:Number(process.argv[2])||8080} ;
	const server = sack.WebSocket.Server( serverOpts )
	const disk = sack.Volume();
	console.log( "serving on " + serverOpts.port, server );

	UserDbRemote.open( { server:"ws://localhost:"+serverOpts.port } );

	//console.table( disk.dir() );


class ServiceConnection {
	serviceId = sack.Id();
	ws = null;
	constructor() {
	}
}

server.onrequest( function( req, res ) {
	var ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
		 req.connection.remoteAddress ||
		 req.socket.remoteAddress ||
		 req.connection.socket.remoteAddress;
	//ws.clientAddress = ip;

	//console.log( "Received request:", req );
        // only redirect to something else.

        const parts = req.url.split( '/' );
        if( parts.length < 3 ) {
            	if( parts[1] === "serviceLogin.mjs" || parts[1] === "serviceLogin.js" ) {
                    res.writeHead( 200, { 'Content-Type': "text/javascript" } );
                    const allowService = new ServiceConnection();
				l.services.set( allowService.serviceId, allowService );
                    const content = [ 'const serviceId="'+ allowService.serviceId + '";\n'
                                     ,serviceLoginScript];
                	res.end( content.join('') );

                        console.log( "Service login script." );
                        return;
                }
	        res.writeHead( 301, { Location:"/node_modules/@d3x0r/popups/example/index-login.html#ws" } );
        	res.end();
                console.log( "redirect to long path" );
	        return;
        }
        if( parts[1] === "node_modules" ) {
            if( parts[2] !== "@d3x0r" && parts[2] !== "jsox" ) {
            	res.writeHead( 404 );
                res.end( "<html><head><title>404</title></head><body>Resource not found</body></html>" );
                return;
            }
		parts[0] = nearPath+"../../..";
        } else {
		parts[0] = nearPath.substr(0,nearPath.length-1);
	}
	
	if( parts[parts.length-1] == "" ) parts[parts.length-1] = "index.html";
        //    req.url = "/index.html";

        	// "node_modules/@d3x0r/popups/example/login.html"

	var filePath =  unescape(parts.join("/"));
	var extname = path.extname(filePath);
	var contentType = 'text/html';
	console.log( ":", extname, filePath )
	switch (extname) {
		case '.js':
		case '.mjs':
		case '.js.gz':
		case '.gz':
			contentType = 'text/javascript';
			break;
		  case '.css':
			  contentType = 'text/css';
			  break;
		  case '.json':
			  contentType = 'application/json';
			  break;
		  case '.png':
			  contentType = 'image/png';
			  break;
		  case '.jpg':
			  contentType = 'image/jpg';
			  break;
		  case '.wav':
			  contentType = 'audio/wav';
			  break;
                case '.crt':
                        contentType = 'application/x-x509-ca-cert';
                        break;
                case '.pem':
                        contentType = 'application/x-pem-file';
                        break;
                  case '.wasm': case '.asm':
                  	contentType = 'application/wasm';
                        break;
	}
	if( disk.exists( filePath ) ) {
		res.writeHead(200, { 'Content-Type': contentType });
		console.log( "Read:", "." + req.url );
		res.end( disk.read( filePath ) );
	} else {
		console.log( "Failed request: ", req );
		res.writeHead( 404 );
		res.end( "<HTML><HEAD>404</HEAD><BODY>404</BODY></HTML>");
	}
} );

server.onaccept( function ( ws ) {
	//console.log( "accept?", ws );
    if( ws.headers["Sec-WebSocket-Protocol"] === "login" )
        return this.accept();
    if( ws.headers["Sec-WebSocket-Protocol"] === "userDatabaseClient" ) {
	const parts = ws.url.split( "?" );
	if( parts.length > 1 ) {
		const sid = parts[parts.length-1];
		const service = l.services.get(sid);
		if( service ) {
		        return this.accept();
		} // otherwise it's an invalid connection... 		
	}
	else
	        return this.accept();
    }

    this.reject();
    //this.accept();
} );





function setKey( f, ws, val ) {
    if( !f || f === "undefined") {
        f = sack.Id();
        console.log( 'sending new id', f );
        ws.send( `{"op":"set","value":"${val}","key":${JSON.stringify(f)}}` );
    }
    return f;
}

function sendKey( ws, val, f ) {
    ws.send( `{"op":"set","value":"${val}","key":${JSON.stringify(f)}}` );
}


async function guestLogin( ws, msg ){

	const isClient = await UserDb.getIdentifier( msg.clientId );
	
	if( !isClient ) {
		//ws.send( JSON.stringify( { op:"login", success: false, ban: true } ) );
		//return;
	}
	
	//msg.deviceId = setKey( msg.deviceId,ws,"deviceId" );

	const user = await UserDb.getUser( msg.account );
	console.log( "user:", user );
	if( user && user.pass === msg.password ) {
                  ws.send( JSON.stringify( { op:"login", success: true } ));
	}
	//console.log( "sending false" );
	console.log( "guest password failure" );
    ws.send( JSON.stringify( { op:"login", success: false } ));
}

async function doLogin( ws, msg ){

	const isClient = await UserDb.getIdentifier( msg.clientId );
	// just need SOME clientID.
	if( !isClient ) {
		ws.send( JSON.stringify( { op:"login", success: false, ban: true } ) );
		return;
	}
	//console.log( "login:", msg );
	//console.log( "client:", isClient );
	

	const user = await UserDb.getUser( msg.account );
	
	if( user && user.unique !== isClient ) {
		// save meta relation that these clients used the same localStorage
		// reset client Id to this User.
		console.log( "User Doing Login with another client:", user, user.unique );
		sendKey( ws, "clientId", user.unique.key );
		// force deviceId to null?
		//msg.deviceId = null; // force generate new device for reversion
	}

	//console.log( "user:", user );
	if( !user || user.pass !== msg.password ) {
		ws.send( JSON.stringify( { op:"login", success: false } ) );
		return;
	}
	
	ws.state.user = user;
	ws.state.login = msg;
	const dev = await user.getDevice( msg.deviceId );
	if( !dev ) {
		ws.state.login = msg;
		// ask the device to add a device.
		ws.send( JSON.stringify( {op:"login", success:false, device:true } ) );
		return;
	}
	if( !dev.active ) {
		ws.send( JSON.stringify( {op:"login", success:false, inactive:true } ) );
		return;
	}
	//console.log( "sending false" );
	ws.send( JSON.stringify( { op:"login", success: true } ));
}

async function doCreate( ws, msg ) {
	const unique = await UserDb.getIdentifier( msg.clientId );//new UniqueIdentifier();
	console.log( "unique:", unique, msg  );
	if( !unique ) {                              
		ws.send( JSON.stringify( { op:"create", success: false, ban: true } ) );
		return;
	}

	const oldUser = await UserDb.User.get( msg.account );
	console.log( "oldUser:", oldUser );
	if( oldUser ) {
		ws.send( JSON.stringify( { op:"create", success: false, account:true } ) );
		return;
	}
	const oldUser2 = await UserDb.User.getEmail( msg.email );
	console.log( "oldUser:", oldUser2 );
	if( oldUser2 ) {                 
		ws.send( JSON.stringify( { op:"create", success: false, email:true } ) );
		return;
	}

	const user = await unique.addUser( msg.user, msg.account, msg.email, msg.password );
	console.log( "user created:", user );
	ws.state.user= user;
	// ask the client for a device id
	ws.send( JSON.stringify( {op:"create", success:false, device:true } ) );
}

async function addDevice(ws,msg) {
	const user = ws.state.user;
	if( user ) {	
		const dev = await user.addDevice( msg.deviceId, ws.state.user.devices.length < 10?true:false );
		//console.log( "dev:", dev );
		if( !dev.active ) {
			ws.send( JSON.stringify( {op:"login", success:false, inactive:true } ) );
			return;
		}
		ws.send( JSON.stringify( { op:"login", success: true } ));
		ws.state.user = null;
	} else {
		console.log( "Bannable failure." );
		// out of sequence - there should be a pending login in need of a device ID.
		ws.send( JSON.stringify( { op:"login", success: false, ban: true } ) );
	}
	
}


function LoginState(ws) {
	this.ws = ws;
	this.client = null;
	this.login = null;
	this.create = null;
	this.user = null;
}

async function newClient(ws,msg) {
	ws.state.client = await UserDb.getIdentifier();
	sendKey( ws, "clientId", ws.state.client.key );
	UserDb.addIdentifier( ws.state.client );
	l.newClients.push( { state:ws.state } );
}


async function handleServiceMsg( ws, msg ){
	// msg.org is 'org.jsox' from the client
	// sid is the last SID we assigned.
	if( msg.sid ) {
		UserDb.getService( msg.org );
	} else {
		const svc = await  UserDb.getService( msg.svc ).then( (s)=>{
			ws.send( JSOX.stringify( { op:"registered" }) )
			return s;
		} );
		if( svc ) {
			// register service finally gets a result... and sends my response.
			ws.send( JSOX.stringify( { op:"register", sid: svc.sid } ) );
		}else {
			console.log( "service will always exist or this wouldn't run.");
		}
		
		// waiting to be allowed...
	}
}
function handleBadgeDef( ws, msg ){
}

async function getService( ws, msg ) {
	// domain, service
	console.log( "Calling requestservice" );
	const svc = await UserDb.requestService( msg.domain, msg.service, ws.state.user );
	if( !svc ) {
		ws.send( JSOX.stringify( {op:"service", ok:false } ) );
	}else {
		//svc.request
		console.log( "got service...", svc );
		//ws.send( JSOX.stringify( {op:"service", ok:true, svc: svc } ) );

	}
}

server.onconnect( function (ws) {
	//console.log( "Connect:", ws );
	const protocol = ws.headers["Sec-WebSocket-Protocol"];
	console.log( "protocol:", protocol )
	ws.state = new LoginState( ws );
	if( protocol === "userDatabaseClient" ) {
		//console.log( "send greeting message, setitng up events" );
		
		ws.onmessage( handleService );
		console.log( "sending service fragment" );
		ws.send( serviceMethodMsg );
        }else if( protocol === "login" ){
		//console.log( "send greeting message, setting up events" );
		ws.onmessage( handleClient );
		ws.send( methodMsg );
        }

	function handleService( msg_ ) {
            	const msg = JSOX.parse( msg_ );
                console.log( 'userLocal message:', msg );
                if( msg.op === "register" ) {
			handleServiceMsg( ws, msg );
			//ws.send( methodMsg );
		} else if( msg.op === "badge" ) {
			handleBadgeDef( ws, msg );
			//ws.send( methodMsg );
		}		
	}

	function handleClient( msg_ ) {
		const msg = JSOX.parse( msg_ );
		console.log( 'message:', msg );
try {
		if( msg.op === "hello" ) {
	//ws.send( methodMsg );
		} else if( msg.op === "newClient" ){
			newClient( ws, msg );
		} else if( msg.op === "request" ){
			getService( ws, msg );
		} else if( msg.op === "login" ){
			doLogin( ws, msg );
		} else if( msg.op === "device" ){
			addDevice( ws, msg );
		} else if( msg.op === "guest" ){
			guestLogin( ws, msg );
		} else if( msg.op === "service" ){
			getService( ws, msg );
		} else if( msg.op === "Login" ){
			ws.send( JSON.stringify( { op:"login", success: true } ));
		} else if( msg.op === "create" ){
			doCreate( ws, msg );
		} else {
			console.log( "Unhandled message:", msg );
		}
} catch(err) {
	console.log( "Something bad happened processing a message:", err );
}
        	//console.log( "Received data:", msg );
		//ws.close();
        };

	ws.onclose( function() {
        	//console.log( "Remote closed" );
        } );
} );

}

