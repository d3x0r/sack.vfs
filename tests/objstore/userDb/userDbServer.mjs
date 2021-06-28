
//console.log( "meta?", import.meta );
import DNS from 'dns';

const colons = import.meta.url.split(':');
const where = colons.length===2?colons[1].substr(1):colons[2];
const nearIdx = where.lastIndexOf( "/" );
const nearPath = where.substr(0, nearIdx+1 );
//console.log( "environment:", process.env );

import path from "path";
import {sack} from "sack.vfs"


const withLoader = true;//process.env.SELF_LOADED;
// make sure we load the import script
if(0)
	if( !withLoader ) {        
		new sack.Task( { 
			work : process.cwd(),
                        bin:process.argv[0],
                        args:[ "--experimental-loader=../../../import.mjs" ,...(process.argv.slice(1))],
                        env:{
                        	SELF_LOADED:"Yup",
                        }, // extra environment.
			input( buffer ) {
				console.log( buffer.substr(0,buffer.length-1) );
			},
			newGroup : false,
			newConsole : false,
			end() { 
				//console.log( "ended.." );
				return process.exit() 
			},
		} );
	}

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
//import {UserDbServer} from "./userDbLoginService.mjs";
//const methodMsg = JSON.stringify( {op:"addMethod", code:methods} );



const l = {
	newClients : [],
	services : new Map(),
	states : [],
	expect : new Map(),
}

const mimeTypes = {
	js:'text/javascript',
	mjs:'text/javascript',
	css:'text/css',
	html:'text/html',
	jpg:'image/jpeg',
	png:'image/png',
	wav:'audio/wav',
	json : 'application/json',
	wasm : 'application/wasm',
	asm:'application/wasm',
	pem:  'application/x-pem-file',
	crt: 'application/x-x509-ca-cert',
}

const resourcePerms22 = [
	{  file:"ui/admin/adminForm.html",  perm:"edit",   fallback:"ui/admin/noPerm.html" }
	,{ file:"ui/admin/adminForm.js",    perm:"edit",   fallback:null  }
]

const resourcePerms = {
	"ui": {
		admin: {
			"adminForm.html": { perm:"edit",   fallback:"ui/admin/noPerm.html" },
			"adminForm.js": { perm:"edit",   fallback:null },
		}
	}
}


// go is from userDb; waits for database to be ready.
if( withLoader ) go.then( ()=>{
        openServer( { port : 8089 } );
} );
else {
	function doNothing() { setTimeout( doNothing, 10000000 ); } doNothing();
}


UserDb.on( "pickSash", (user, choices)=>{
	for( let state of l.states ) {
		if( state.user === user
		  && !state.connected 
		  && !state.picking ) {
			state.picking = true;
			const p = { p:null, res:null, rej:null};
			p.p = new Promise( (res,rej)=>{ p.res = res; p.rej= rej } );
			state.waits.pickSash = p;
			state.ws.send( JSOX.stringify( { op:"pickSash", choices: choices } ) );
			return p.p;
		}
	}
	throw new Error( "How are you picking a sash for a user that's not connected?" );
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

	function getResource( req_url, user ) {
		const res = {
			code: 404,
			content : null,
                        headers : { 'Content-Type': "text/html" },
		};
		const parts = req_url.split( '/' );
		if( parts.length < 3 ) {
				if( parts[1] === "serviceLogin.mjs" || parts[1] === "serviceLogin.js" ) {
                                	res.code = 200;
                                        res.headers["Content-Type"] = "text/javascript";
					const allowService = new ServiceConnection();
					l.services.set( allowService.serviceId, allowService );
					const content = [ 'const serviceId="'+ allowService.serviceId + '";\n'
									,serviceLoginScript];
					res.content = content.join('');

					console.log( "Service login script." );
					return res;
				}
                                
			res.code = 301;
                        res.headers = { Location:"/ui/profile/" };
			return res;
		}
		if( parts[1] === "node_modules" ) {
			if( parts[2] !== "@d3x0r" && parts[2] !== "jsox" ) {
                        	res.code = 404;
				res.content = "<html><head><title>404</title></head><body>Resource not found</body></html>";
				return res;
			}
			parts[0] = nearPath+"../../..";
		} else {
			if( user ) {
				let perm = resourcePerms;
				for( let part = 1; part < parts.length; part++ ) {
					if( parts[part] in perm ) {
						perm = perm[parts[part]];
					}else {
						perm = null;
						break;
					}
				}
				if( perm ) {
					if( !user.badges[perm.perm] ) {
						console.log( "File is permissioned...", perm, parts );
						parts[1] = perm.fallback;
						console.log( "", parts );
						parts.length = 2;
					} // otherwise, load the natural file.
				}
			}
			parts[0] = nearPath.substr(0,nearPath.length-1);
		}
		
		if( parts[parts.length-1] == "" ) parts[parts.length-1] = "index.html";


		const filePath =  unescape(parts.join("/"));
		const extensions = path.extname(parts[parts.length-1]).split('.');
		//extensions.splice(0,1);
		const extname = extensions[extensions.length-1];

		console.log( ":", extname, filePath )
		res.headers["Content-Type"] = mimeTypes[extname];
		if( disk.exists( filePath ) ) {
			console.log( "Read:", req_url, "as", filePath  );
			res.code = 200; 
			res.content = disk.read( filePath )
		} else {
			console.log( "Failed request: ", req_url );
			res.code = 404; 
			res.content = "<HTML><HEAD><title>404</title></HEAD><BODY>404</BODY></HTML>";
		}
		return res;
	}

	server.onrequest = function( req, res ) {
		var ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
			req.connection.remoteAddress ||
			req.socket.remoteAddress ||
			req.connection.socket.remoteAddress;

		const resource = getResource( req.url, null );
		res.writeHead(resource.code, resource.headers);
		res.end( resource.content );
	} ;

	server.onaccept = function ( ws ) {
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
	};





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
		debugger;
		const validEMail = await checkEmail( ws.email );
		if( !validEMail ) {
			ws.send( JSON.stringify( { op:"create", success: false, email:true } ) );
			return;
		}
		const unique = await UserDb.getIdentifier( msg.clientId );//new UniqueIdentifier();
		//console.log( "unique:", unique, msg  );
		if( !unique ) {                              
			//console.log( "Resulting with a reset of client ID." );
			ws.send( JSON.stringify( { op:"create", success: false, ban: true } ) );
			return;
		}

		const oldUser = await UserDb.User.get( msg.account );
		if( oldUser ) {
			ws.send( JSON.stringify( { op:"create", success: false, account:true } ) );
			return;
		}
		const oldUser2 = await UserDb.User.getEmail( msg.email );
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
		this.connected = false;
		this.picking = false;
		this.waits = {
			pickSash : null,  // 
		}
		l.states.push( this );
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
			//const srvc = await UserDb.getService( msg.svc );
			
		} else {
			const svc = await  UserDb.getService( msg.svc ).then( (s)=>{
				console.log( "Ahh Hah, finall, having registered my service, I connect this socket", s, ws );
				s.connect( ws );
				//ws.send( JSOX.stringify( { op:"registered" }) )
				return s;
			} );
			if( svc ) {
				// register service finally gets a result... and sends my response.
				console.log( "Service resulted, and is an instance?", svc );
				ws.send( JSOX.stringify( { op:"register", ok:true, sid: svc.sid } ) );
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
		//console.log( "Calling requestservice", ws.state );
		const svc = await UserDb.requestService( msg.domain, msg.service, ws.state.user );
		if( svc ) {
			//console.log( "Service result:", svc, "for", msg );
			svc.authorize( ws.state.user );
		} else {
			ws.send( JSOX.stringify( {op:"service", ok:false, probe:true } ) );
		}
	}

	function pickedSash(ws,msg ) {
		if( msg.ok )  state.waits.pickSash.res( msg.sash );
		else          state.waits.pickSash.rej( msg.sash );
	}

	server.onconnect = function (ws) {
		//console.log( "Connect:", ws );
		const protocol = ws.headers["Sec-WebSocket-Protocol"];
		let user = null;
		console.log( "protocol:", protocol )
		ws.state = new LoginState( ws );
		if( protocol === "userDatabaseClient" ) {
			//console.log( "send greeting message, setitng up events" );
			
			ws.onmessage = handleService;
			console.log( "sending service fragment" );
			ws.send( serviceMethodMsg );
		} else if( protocol === "admin" ){
			ws.onmessage = handleAdmin;		
		} else if( protocol === "userDatabasePeer" ){
			ws.onmessage = handlePeer;
			negotiatePeer();
		} else if( protocol === "login" ){
			//console.log( "send greeting message, setting up events" );
			ws.onmessage = handleClient;
			ws.send( methodMsg );
		}

		function handlePeer( msg_ ) {
			const msg = JSOX.parse( msg_ );
			if( msg.op === "" ) {
			}
			
		}

		function negotiatePeer() {
			// tell peer some information about me?
			// give the peer the script to be my peer?

		}


		function handleAdmin( msg_ ) {
			console.log( 'admin Socket message:', msg );
			if( !user ) {
				user = l.expect.get( msg_ );
				if( !user ) {
					ws.send( JSOX.stringify( {op:"badIdentification"}));
					ws.close( );
					return;
				}else
					l.expect.delete( msg_ );
			}else {
				const is_ll = msg_[0] === "\0";
				const msg = is_ll?JSOX.parse( msg_.substr(1) ):JSOX.parse( msg_ );
				if( is_ll && msg.op === "get" ){
					//, {op:"get", url:url, id:newEvent.id } );
					if( msg.url ){
			                	const res = getResource( msg.url, user );
						ws.send( JSOX.stringify( {op:"GET", id:msg.id, res:res } ) );
					}
					else
						ws.send( JSOX.stringify( {op:"GET", id:msg.id, res:{code:0,content:"bad request",contentType:"text/plain"} } ) );
					return true;
				}
				else if( msg.op === "" ){
					if( !user.badges.edit ) {

					}else {

					}
				}
			}
		}

		function doAuthorize( msg ) {
			// msg.addr
			// msg.key
			
		}

		function handleService( msg_ ) {
			const msg = JSOX.parse( msg_ );
			console.log( 'userLocal message:', msg );
			if( msg.op === "register" ) {
				handleServiceMsg( ws, msg );
				//ws.send( methodMsg );
			} else if( msg.op === "expect" ) {
				// user connection expected on this connection...
				console.log( "Authorize sent - now e need to send back UID and IP")
				const id = sack.Id();
				l.expect.set( id, msg );
				
				ws.send( JSOX.stringify( { op:"authorize", id:msg.id, addr:addr, key:id } ) );

			} else {
				console.log( "unhandled client admin/profile message:", msg_ );
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
				} else if( msg.op === "authorize" ){
					doAuthorize( ws, msg );
				} else if( msg.op === "Login" ){
					ws.send( JSON.stringify( { op:"login", success: true } ));
				} else if( msg.op === "create" ){
					doCreate( ws, msg );
				} else if( msg.op === "pickSash" ){
					pickedSash( ws, msg );
				} else {
					console.log( "Unhandled message:", msg );
				}
			} catch(err) {
				console.log( "Something bad happened processing a message:", err );
			}
				//console.log( "Received data:", msg );
			//ws.close();
			};

		ws.onclose = function() {
				//console.log( "Remote closed" );
			for( let s = 0; s < l.states.length; s++ ) {
				const st = l.states[s];
				if( st.ws === ws ) {
					l.states.splice( s, 1 );
				}
			}
		};
	};

}







function checkEmail( email ) {
	const p = {p:null,res:null};
	p.p = new Promise( (res,rej)=>p.res =res );
	console.log( "CHECK:", email);
	validateEmail( email, ( valid )=> {
		if( !valid ) p.res( true );
		else {
			var user = db.do( `select 1 from users3 where email='${email}' COLLATE NOCASE`);
			if( user && user.length )
				p.res( true );
			p.res( false );
		}
	} );
	return p.p;
}

const domainAllowedChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-"
const allowedChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&'*+-/=?^_`{|}~"
const allowedChars2 = ' .(),:;<>@[]' ; // \ and " can be quoted too; but handled separtely

// this needs to handle just IP addresses also.

function validateEmail( email, cb ) {
	if( !email ) return cb( false );
	function lookupDomain( domain, cb ) {
		DNS.lookup( domain, (err,address,family)=>{
			_debug && console.log( "test domain:",domain, err);
			if( err ) cb( false );
			else cb( true );
		})
	}

	function stripComment( field ) {
		if( field[0] == '(' ) {
			for( var n = 1; n < field.length; n++ )
				if( field[n] == ')' ) {
					return field.substr( n+1 );
				}
			return '';
		}

		if( field[field.length-1] == ')' ) {
			for( var n = field.length-1; n >= 0; n-- )
				if( field[n] == '(' ) {
					return field.substr( 0, n );
				}
			return '';
		}
	return field;
	}

	function quotedAtSplit( email ) {
		var parts = [];
		var quoted = false;
		var escape = false;
		for( var n = 0; n < email.length; n++ ) {
			if( email[n] == '"' ) {
				if( escape ) { escape = false; continue; }
				if( quoted ) { quoted = false; continue; } else { quoted = true; continue; }
			}
			if( email[n] == '\\' )
				if( escape ) { escape = false; continue }
				else if( quoted ) { escape = true; continue; }
			if( escape ) { escape = false; continue; };
			if( email[n] == '@' ) {
				if( quoted ) continue;
				parts.push( email.substr( 0, n ) );
				parts.push( email.substr( n+1 ) );
				return parts;
			}
		}
		return parts;
    }

	function quotedDotSplit( email ) {
		var parts = [];
		var lastPart = 0;
		var quoted = false;
		var escape = false;
		for( var n = 0; n < email.length; n++ ) {
			if( email[n] == '"' ) {
				if( escape ) { escape = false; continue; }
				if( quoted ) { quoted = false; continue; } else { quoted = true; continue; }
			}
			if( email[n] == '\\' )
				if( escape ) { escape = false; continue }
				else if( quoted ) { escape = true; continue; }
			if( escape ) { escape = false; continue; };
			if( email[n] == '.' ) {
				_debug&&console.log( "found a dot...", quoted, parts );
				if( quoted ) continue;
				parts.push( email.substr( lastPart, n-lastPart ) );
				lastPart = n+1;
			}
		}
		_debug&&console.log( "Tail:", lastPart, email, "=", email.substr( lastPart ) );
		parts.push( email.substr( lastPart ) );
		return parts;
	}
	var parts = quotedAtSplit( email );
	_debug&&console.log( "Split:", parts );
	if( parts.length != 2 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
	if( parts[0].length > 64 || parts[0].length < 1 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
	parts[0] = stripComment( parts[0] );
	if( !parts[0] ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
	parts[1] = stripComment( parts[1] );
	if( !parts[1] ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
	_debug&&console.log( "domain comment-stripped:", parts[1] );

	var local = quotedDotSplit( parts[0] );
	_debug&&console.log( "local dot split:", local );
	for( n = 0; n < local.length; n++ ) {
		local[n] = [...local[n]];
	}
	if( parts[1].length > 253 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
	var domain = parts[1].split( "." );
	if( domain.length > 127 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
	var n;
	for( n = 0; n < local.length; n++ ) {
		if( !local[n].length ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
		var len = local[n].length;
		var escape = false;
		if( local[n][0] == '"' ) {
			if( local[n][local[n].length-1] !== '"' ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
			len--;
			for( var m = 1; m < len; m++ ) {
				if( local[n][m].codePointAt(0) > 0x7f ) continue;
				if( local[n][m] == "\\" )
					if( escape ) {
						escape = false;
						continue;
					}
					else {
						escape = true;
						continue;
					}
				if( escape ) {
					if( local[n][m] == '"' ) {
						escape = false;
						continue;
					}
				}
				if( !allowedChars.includes( local[n][m] ) )
					if( !allowedChars2.includes( local[n][m] ) )
						{ _debug&&console.log( "Fail at char:", m, local[n], local[n][m] ); _debug&&console.trace( "FAIL" ); return false; }
			}
		}
		else {
			for( var m = 0; m < len; m++ ) {
				if( local[n][m].codePointAt(0) > 0x7f ) continue;
				if( !allowedChars.includes( local[n][m] ) )
					{ _debug&&console.trace( "FAIL" ); cb(false);return false; }
			}
		}
	}

	if( parts[1][0] == '[' && parts[1][parts[1].length-1] == ']' ) {
		parts[1] = parts[1].substr( 1, parts[1].length-2 );
		if( parts[1].startsWith( "IPv6:" ) ) {
			var addrparts = parts[1].split(':' );
			var words = [];
			var zero = 0;
			for( var n = 1; n < addrparts.length; n++ ) {
				if( !addrparts[n].length ) {
					if( zero ) { _debug&&console.trace( "FAIL" ); cb(false);return false; } // already found a zero filler
					zero = n;
					words.push( 0 );
				} else {
					var val = parseInt(addrparts[n], 16);
					if( val.toString(16).toUpperCase() !== addrparts[n].toUpperCase() )
						{ _debug&&console.trace( "FAIL" ); cb(false);return false; }
					words.push( val );
				}
			}
			function zeroFill( words ) {
				var newwords = [];
				for( var n = 0; n < zero-1; n++ )
					newwords.push( words[n] );

				for( var m = 0; m < 8-( (words.length-1) ); m++ )
					newwords.push(0);
				n++;
				for( ; n < words.length; n++ )
					newwords.push( words[n] );
				return newwords;
			}
			_debug&&console.log( "words:", words );
			words = zeroFill( words );
			_debug&&console.log( "words:", words );
			if( words.length !== 8 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
			if( !words[0] ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
			if( words[0] > 0xFF00 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; } // cannot send to mutlicast email
			if( words[0] == 0xfec0 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; } // cannot send to site local
			if( words[0] == 0x0100 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; } // cannot send to trash
			if( ( words[0] & 0xFF30 ) == 0xfe80 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; } // cannot send to site local
			if( ( words[0] & 0xFC00 ) == 0xfc00 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; } // unique local
			if( words[0] == 0x2001 && words[1] == 0xdb8 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; } // cannot send to example IP

		}
		else {
			var addrparts = parts[1].split('.');
			var words = [];
			if( addrparts.length != 4 )
				{ _debug&&console.trace( "FAIL" ); cb(false);return false; }
			for( var n = 0; n < addrparts.length; n++ ) {
				var val = parseInt( addrparts[n] );
				if( val.toString() !== addrparts[n] )
					{ _debug&&console.trace( "FAIL" ); cb(false);return false; }
				if( val > 255 || val < 0 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
				words.push( val );
			}
			// disallow localhost addresses
			if( words[0] == 192 && words[1] == 168 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
			if( words[0] == 172 && words[1] >= 16 && words[1] < 32 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
			if( words[0] == 10 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
			if( words[0] == 127 && words[1] == 0 && words[2] == 0 && words[3] == 1 ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }

		}
		return true;  // assume the IP in brackets is valid?
	}
	for( n = 0; n < domain.length; n++ ) {
	_debug&&console.log( "domain part:", domain[n] );

		if( domain[n].length < 1 || domain[n].length > 63 ) {
			if( n == (domain.length-1) && domain[n].length === 0 )
				continue;
			{ _debug&&console.trace( "FAIL" ); cb(false);return false; }
		}
		if( domain[n][0] == '-' ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
		if( domain[n][domain[n].length-1] == '-' ) { _debug&&console.trace( "FAIL" ); cb(false);return false; }
		var len = domain[n].length;

 		for( var m = 0; m < len; m++ ) {
			if( !domainAllowedChars.includes( domain[n][m] ) )
				{ _debug&&console.trace( "FAIL" ); cb(false);return false; }
		}
	}
	lookupDomain( domain.join('.'), cb );
/*
Uppercase and lowercase English letters (a-z, A-Z)
Digits 0 to 9
Characters ! # $ % & ' * + - / = ? ^ _ ` { | }
Character . (dot, period, full stop) provided that it is not the first or last character,
		and provided also that it does not appear two or more times consecutively.
*/
}
