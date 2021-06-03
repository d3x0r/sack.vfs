
console.log( "meta?", import.meta );

const colons = import.meta.url.split(':');
const where = colons===1?colons[1].substr(1):colons[2];
const nearIdx = where.lastIndexOf( "/" );
const nearPath = where.substr(0, nearIdx+1 );

console.log( "missed?", nearIdx, where, nearPath );
import {sack} from "sack.vfs"
const JSOX = sack.JSOX;
import {UserDb,User,Device,UniqueIdentifier,go} from "./userDb.mjs"

const storage = sack.ObjectStorage( "data.os" );
UserDb.hook( storage );

const methods = sack.Volume().read( nearPath+"userDbMethods.js" ).toString();
const methodMsg = JSON.stringify( {op:"addMethod", code:methods} );

const l = {
	newClients : [],
}


async function makeUsers() {
	for( let i = 1; i < count; i++ ) {
		const unique = new UniqueIdentifier();
		unique.key = sack.Id();
		//console.log( "user:", i );
		await unique.store().then( ((i)=> ()=>{
	 			const user = unique.addUser( i, "User "+i, '' + i + "@email.com", Math.random()*(1<<54) );
				return user.addDevice( sack.Id(), true ).then( ()=>{
					//console.log( "storing user", i );
					return user.store();
				} );
			} )(i+from) );
	}
}	


function getUsers() {
	console.log( "Getting..." );
	User.get( 3 ).then( (user)=>{
		console.log( "Got 3 :", user );
	} );
	User.get( 203 ).then( (user)=>{
		console.log( "Got 203:", user );
	} );
	User.get( 835 ).then( (user)=>{
		console.log( "Got 835:", user );
	} );
}

go.then( ()=>{
	console.log( "waited until initialized..." );

        openServer( { port : 8089
                } );

	if(0)
	makeUsers().then( ()=>{			
		// after creating all users, get some users
		getUsers();
	} );
} );




import path from "path";

function openServer( opts, cb )
{
var serverOpts = opts || {port:Number(process.argv[2])||8080} ;
var server = sack.WebSocket.Server( serverOpts )
var disk = sack.Volume();
console.log( "serving on " + serverOpts.port );
console.table( disk.dir() );


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
	        res.writeHead( 301, { Location:"/node_modules/@d3x0r/popups/example/index-login.html#ws" } );
        	res.end();
	        return;
        }
        if( parts[1] === "node_modules" ) {
            if( parts[2] !== "@d3x0r" ) {
            	res.writeHead( 404 );
                res.end( "<html><head><title>404</title></head><body>Resource not found</body></html>" );
                return;
            }
        }
        //    req.url = "/index.html";

        	// "node_modules/@d3x0r/popups/example/login.html"

	var filePath = "." + unescape(req.url);
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
    if( ws.headers["Sec-WebSocket-Protocol"] === "login" )
        return this.accept();

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
		ws.send( JSON.stringify( { op:"login", success: false, ban: true } ) );
		return;
	}
	
	msg.deviceId = setKey( msg.deviceId,ws,"deviceId" );

	const user = await UserDb.getUser( msg.account );
	console.log( "user:", user );
	if( user && user.pass === msg.password ) {
                  ws.send( JSON.stringify( { op:"login", success: true } ));
	}
	console.log( "sending false" );
        ws.send( JSON.stringify( { op:"login", success: false } ));
}

async function doLogin( ws, msg ){

	const isClient = await UserDb.getIdentifier( msg.clientId );
	if( !isClient ) {
		ws.send( JSON.stringify( { op:"login", success: false, ban: true } ) );
		return;
	}
	console.log( "login:", msg );
	console.log( "cilent:", isClient );
	

	const user = await UserDb.getUser( msg.account );

	if( user && user.unique !== isClient ) {
		// save meta relation that these clients used the same localStorage
		// reset client Id to this User.
console.log( "User Doing Login:", user, user.unique );
		sendKey( ws, "clientId", user.unique.key );
		// force deviceId to null?
		msg.deviceId = null; // force generate new device for reversion
	}

	//console.log( "user:", user );
	if( !user || user.pass !== msg.password ) {
		ws.send( JSON.stringify( { op:"login", success: false } ) );
		return;
	}
	
	ws.state.user = user;
	ws.state.login = msg;
	const dev = await user.getDevice( msg.deviceId );
	//console.log( "dev:", dev );
	if( !dev ) {
		// console.log( "Device failure of login" );
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
		console.log( "Can add device to user..." )
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


server.onconnect( function (ws) {
	//console.log( "Connect:", ws );
	const protocol = ws.headers["Sec-WebSocket-Protocol"];
	console.log( "protocol:", protocol )
	ws.state = new LoginState( ws );
	
	
	ws.send( methodMsg );

	ws.onmessage( function( msg_ ) {

            	const msg = JSOX.parse( msg_ );
                console.log( 'message:', msg );
                if( msg.op === "hello" ) {
			//ws.send( methodMsg );
                } else if( msg.op === "newClient" ){
			newClient( ws, msg );
                } else if( msg.op === "login" ){
			doLogin( ws, msg );
                } else if( msg.op === "device" ){
			addDevice( ws, msg );
                } else if( msg.op === "guest" ){
			guestLogin( ws, msg );
                } else if( msg.op === "Login" ){
                        ws.send( JSON.stringify( { op:"login", success: true } ));
                } else if( msg.op === "create" ){
			doCreate( ws, msg );
                } else {
			console.log( "Unhandled message:", msg );
		}
        	//console.log( "Received data:", msg );
		//ws.close();
        } );
	ws.onclose( function() {
        	//console.log( "Remote closed" );
        } );
} );

}

