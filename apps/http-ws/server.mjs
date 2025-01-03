
//const sack = require( "sack.vfs" );
//const path = require( "path" );

import {sack} from "../../vfs_module.mjs";
import {Events} from "../events/events.mjs";
import {uExpress} from "./uexpress.mjs";
import path from "path";
const disk = sack.Volume();

const myPath = import.meta.url.split(/\/|\\/g);
const tmpPath = myPath.slice();
tmpPath.splice( 0, 3 );
tmpPath.splice( tmpPath.length-1, 1 );
const parentRoot = (process.platform==="win32"?"":'/')+tmpPath.slice(0,-2).join( '/' );

let packageRoot = null;
{
	let here = process.cwd().split(/[\/\\]/);
	let leader = here.length;
	while( !packageRoot && leader > 0 ) {
		const root = here.slice(0,leader).join('/');
		if( disk.exists( root + "/package.json" ) ) {
			if( disk.isDir( root + "/node_modules" ) ) {
				packageRoot = root;
				break;
			}
		}
	}
}

function read( name ) {
	try {
		const data = sack.Volume.readAsString( name );
		return data;
	} catch(err) {
		console.log( "Failed to load cert:", name );
		return undefined;
	}
}

function getCertChain( ) {
	//SSLCertificateFile /etc/letsencrypt/live/d3x0r.org/fullchain.pem
	//SSLCertificateKeyFile /etc/letsencrypt/live/d3x0r.org/privkey.pem
	if( process.env.SSL_PATH ) {
		//if( !process.env.SSL_HOST ) {
		//	console.log( "ssl host name not specified..." )
		//	return null;
		//}
		return process.env.SSL_PATH + "/fullchain.pem"
	}
	return  parentRoot + "/certgen/cert-chain.pem"
}
function getCertKey( ) {
	if( process.env.SSL_PATH ) {
		if( !process.env.SSL_HOST ) {
			console.log( "ssl host name not specified..." )
			return null;
		}
		return process.env.SSL_PATH + "/privkey.pem"
	}
	return  parentRoot + "/certgen/rootkeynopass.prv"
}

const certChain = read( getCertChain() );
if( certChain )
	if( !process.env.SSL_HOST ) {
		process.env.SSL_HOST = sack.TLS.hosts( certChain ).join("~");
		console.log( "Host not specified, using certificate hosts:", process.env.SSL_HOST );
	}
//console.log( "certChain loaded?", sack.TLS.hosts( certChain ) );
const certKey = read( getCertKey() );

const encMap = {
		'.gz':'gzip'
};

const extMap = { '.js': 'text/javascript'
              ,  '.mjs':'text/javascript'
              ,  '.css':'text/css'
              ,'.json':'text/javascript'
              ,'.png':'image/png'
              ,'.html':'text/html'
              ,'.htm':'text/html'
              ,'.jpg':'image/jpg'
              ,'.webm':'video/webm'
              ,'.mp4':'video/mp4'
              ,'.svg':'image/svg+xml'
              ,'.wav':'audio/wav'
              ,'.crt':'application/x-x509-ca-cert'
              ,'.pem':'application/x-pem-file'
              ,'.wasm': 'application/wasm'
              , '.asm': 'application/wasm' 
			, '.bat':'application/x-msdownload'
			, '.dll':'application/x-msdownload'
			, '.exe':'application/x-msdownload'
			, '.cmd':'application/x-msdownload'
			, '.com':'application/x-msdownload'
			, '.msi':'application/x-msdownload'
		}

const requests = [];
let reqTimeout = 0;
let lastFilePath = null;
function logRequests() {
	const log = requests.join(', ');
	requests.length = 0;
	console.log( "Requests:", log );
}




//exports.getRequestHandler = getRequestHandler;
export function getRequestHandler( serverOpts ) {
	serverOpts = serverOpts || {};
	let resourcePath = serverOpts.resourcePath || ".";
	const npm_path = serverOpts.npmPath || ".";

	return function( req, res ) {
		/*
			this is the request remote address if required....
		const ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
			 req.connection.remoteAddress ||
			 req.socket.remoteAddress ||
			 req.connection.socket.remoteAddress;
		*/
		//const resource_path = serverOpts.resourcePath || ".";

		//console.log( "Received request:", req );
		if( req.url[req.url.length-1] === "/" ) req.url += "index.html";

		let filePath = resourcePath + unescape(req.url);
		if( req.url.startsWith( "/node_modules/" ) 
		   && ( req.url.startsWith( "/node_modules/@d3x0r" ) 
		      || req.url.startsWith( "/node_modules/jsox" )
		      || req.url.startsWith( "/node_modules/sack.vfs/apps" ) ) )
			filePath=npm_path  + unescape(req.url);
		let extname = path.extname(filePath);		
		let contentEncoding = encMap[extname];
		if( contentEncoding ) {
			extname = path.extname(path.basename(filePath,extname));
		}

		// if the name doesn't end in a slash, should generate a redirect to content... 
		if( disk.isDir( filePath ) ) {filePath += "/index.html"; extname = ".html"; }

		const contentType = extMap[extname] || "text/plain";
		if( disk.exists( filePath ) ) {
			const fc = disk.read(filePath );

			if( fc ) {
				const headers = { 'Content-Type': contentType, 'Access-Control-Allow-Origin' : req.connection.headers.Origin };
				if( contentEncoding ) headers['Content-Encoding']=contentEncoding;
				res.writeHead(200, headers );
				if( req.CGI['🔧'] ) {
					const str = fc.toString();
					const content = str.replaceAll( /import([^\(]?\s+[^\(]?.*from\s+|)["']((?!\/|.\/|..\/)[^'"]*)["']/g, 'import$1"/$2?🔧=1+🔨=' + req.CGI['🔨'] +'"' )
					res.end( content );
				} else {
					res.end( fc );
				}

				if( requests.length !== 0 )
					clearTimeout( reqTimeout );
				reqTimeout = setTimeout( logRequests, 500 );
				requests.push( req.url );
			} else {
				console.log( 'file exists, but reading it returned nothing?', filePath, fc );
				return false;
			}
			return true;
		} else {
			const foundModule = findModule( unescape(req.url), req, res );
			if( foundModule ) {
				if( "object" === typeof  foundModule ) {
				const headers = { 'Content-Type': foundModule.contentType, 'Access-Control-Allow-Origin' : req.connection.headers.Origin };
				if( contentEncoding ) headers['Content-Encoding']=contentEncoding;
				res.writeHead(200, headers );
				res.end( foundModule.content );

				if( requests.length !== 0 )
					clearTimeout( reqTimeout );
				reqTimeout = setTimeout( logRequests, 500 );
				requests.push( req.url );
				}
				return true;
			}
			lastFilePath = filePath;
			return false;
		}
	};
	function findModule( spec, req, res ) {
		const specPath = spec.substr( 1 ).split('/');
		let content = null;

		//console.log("Find module:", spec, leader, here );
		let n = 1;
		let a = "";
		while( n <= specPath.length && disk.isDir( a = packageRoot + "/node_modules" + "/" + specPath.slice(0,n++).join('/') ) )
			;//console.log( "still goood", a );
		const relPath = "/node_modules/" + specPath.slice(0,n-1).join('/') + "/";
		//console.log( "so we use:", a, relPath );
		//const filePart = specPath.slice(n-1);

		//console.log( "Even have node_modules..." );
		const modPath = packageRoot + relPath;
		//console.log( "Path:", modPath );
		const pkgDef = modPath + "package.json";
		if( disk.exists( pkgDef ) ) {
			const module = disk.readJSOX( pkgDef, (pkg)=>{
				res.writeHead( 301, {Location: relPath + pkg.module + "?🔧=1+🔨=" + relPath} );
						const modname = modPath + pkg.module;
							
							//content = disk.read( modname );
					} );
			res.end();
			return true;
		}
		if( content )
			return { contentType: "text/javascript", content };
		return null;
	}
}

function hookJSOX( serverOpts, server ) {
const app = uExpress();
server.addHandler( app.handle );

	server.app = app;
app.get( /.*\.jsox|.*\.json6/, (req,res)=>{

	console.log( "express hook?", req.url , serverOpts.resourcePath + req.url);
	const headers = {
		'Content-Type': "text/javascript",
	}

	let filePath;
		if( req.url.startsWith( "/common/" ) ) {
			filePath = commonRoot + decodeURI(req.url).replace( "/common", "" );
		}else {
			filePath = serverOpts.resourcePath + req.url;
		}

	const config = disk.read( filePath );
	if( config ) {
		res.writeHead( 200, headers );
		const resultContent = "import {JSOX} from '/node_modules/jsox/lib/jsox.mjs';const config = JSOX.parse( `" + config.toString().replace( "\\", "\\\\" ).replace( '"', '\\"' ) + "`);export default config;";
		res.end( resultContent );
		return true;
	}else {
		console.log( "no file.." );
		return false;
	}
} ) 
}

class Server extends Events {
	handlers = [];
	resourcePath = ".";
	npmPath = ".";
	app = null;
	constructor( server, serverOpts, reqHandler ) {
		super();
		this.reqHandler = reqHandler;
		this.serverOpts = serverOpts;
		this.server = server;
	}
		server = null;
		//handleEvent( req, res ) {
		//	return eventHandler( req, res );
		//}
		setResourcePath( path ) {
			resourcePath = path;	
		}
		addHandler( handler ) {
			this.handlers.push( handler );
		}
		removeHandler( handler ) {
			const index = this.handlers.findIndex( h=>h===handler );
			if( index >= 0 )
				handlers.splice( index, 1 );
		}

	handleEvent(req,res) {
		for( let handler of this.handlers ) {
			if( handler( req, res, this.serverOpts ) ) {
				return true;
			}
		}
		if( !this.reqHandler( req,res ) ) {
			if( requests.length !== 0 )
				clearTimeout( reqTimeout );
			reqTimeout = setTimeout( logRequests, 100 );
				
			requests.push( "Failed request: " + req.url + " as " + lastFilePath );
			res.writeHead( 404, {'Access-Control-Allow-Origin' : req.connection.headers.Origin } );
			res.end( "<HTML><HEAD><title>404</title></HEAD><BODY>404<br>"+req.url+"</BODY></HTML>");
		}
	}

}

//exports.open = openServer;
let eventHandler = null;
export function openServer( opts, cbAccept, cbConnect )
{
	let handlers = [];
	const serverOpts = opts || {};
	if( !("port" in serverOpts )) serverOpts.port = Number(process.env.PORT)||(process.argv.length > 2?Number(process.argv[2]):0) || 8080;
	if( !("resourcePath" in serverOpts ) ) serverOpts.resourcePath = "."
	if( certChain ) 
	{
		if( !serverOpts.hosts ) serverOpts.hosts = [];
		serverOpts.hosts.push( {
			host: process.env.SSL_HOST || "localhost",
			cert : serverOpts.cert || certChain,
			key : serverOpts.key || certKey
	
		});
		//serverOpts.cert = serverOpts.cert || certChain;
		//serverOpts.key = serverOpts.key || certKey;
	}
	const server = sack.WebSocket.Server( serverOpts )
	//console.log( "serving on " + serverOpts.port, server );
	//console.log( "with:", disk.dir() );

	const srvr = new Server( server, serverOpts, getRequestHandler( opts ) );

	server.onrequest = srvr.handleEvent.bind( srvr );

	server.on( "lowError",function (error, address, buffer) {
		if( error !== 1 && error != 6 ) 
			if( error === 7 ) {
				console.log( "Requested host not found:", address.remoteAddress, "requested:", buffer );
				return;
			}
			else console.log( "Low Error with:", error, address, buffer  );
			//if( buffer )
		//	buffer = new TextDecoder().decode( buffer );
		server.disableSSL(); // resume with non SSL
	} );

	server.onaccept = function ( ws ) {
		//console.log( "send accept?", cbAccept );
		if( cbAccept ) return cbAccept.call(this,ws);
		if( srvr.on( "accept", ws ) ) {
			this.accept();
			return;
		}
		
		if( process.env.DEFAULT_REJECT_WEBSOCKET == "1" )
			this.reject();
		else
			this.accept();
	};

	server.onconnect = function (ws) {
		try {
			if( cbConnect ) return cbConnect.call(this,ws);
		}catch( err){
			console.log( "onconnect callback failed (C++ isn't so good at catching exceptions...):", err );
			return;
		}
		ws.nodelay = true;
		if( !srvr.on( "connect", ws ) ) {
			ws.onmessage = function( msg ) {
				// echo message.
				// ws.send( msg );
				// parser.write( msg );
				// no message callback defined
			};
			ws.onclose = function() {
				console.log( "Remote closed" );
			};
		}
	};

	hookJSOX( serverOpts, srvr );
	return srvr;
}

