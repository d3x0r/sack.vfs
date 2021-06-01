

const from = Number( process.argv[2] ) || 1
const count = Number( process.argv[3] ) || 100

console.log( "Making from;", from );
import {sack} from "../../../vfs_module.mjs"
import {UserDb,User,Device,UniqueIdentifier,go} from "./userDb.mjs"

const storage = sack.ObjectStorage( "data.os" );
UserDb.hook( storage );

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

console.log( "Go:", go );
go.then( ()=>{
	console.log( "waited until initialized..." );

        openServer( { port : 8089
                } );

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
console.log( "with:", disk.dir() );


server.onrequest( function( req, res ) {
	var ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
		 req.connection.remoteAddress ||
		 req.socket.remoteAddress ||
		 req.connection.socket.remoteAddress;
	//ws.clientAddress = ip;

	//console.log( "Received request:", req );
	if( req.url === "/" ) req.url = "/index.html";

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

server.onconnect( function (ws) {
	//console.log( "Connect:", ws );
	ws.onmessage( function( msg_ ) {
            	const msg = JSOX.parse( msg_ );
                if( msg.op === "login" ){



                }

                if( msg.op === "create" ){
			const unique = msg.clientId;//new UniqueIdentifier();
			//unique.key = sack.Id();
			//console.log( "user:", i );

			unique.store().then( ((i)=> ()=>{
	 			const user = unique.addUser( i, "User "+i, '' + i + "@email.com", Math.random()*(1<<54) );
				return user.addDevice( sack.Id(), true ).then( ()=>{
					//console.log( "storing user", i );
					return user.store();
				} );
			} )(i+from) ).then( ()=>{
                                ws.send( JSOX.stringify( { op:"created", id:"success"  } ) );
                        } );;

                }
        	//console.log( "Received data:", msg );
		//ws.close();
        } );
	ws.onclose( function() {
        	//console.log( "Remote closed" );
        } );
} );

}

