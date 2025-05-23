
const sack = require( "sack.vfs" );
const path = require( "path" );

//import {sack} from "../../vfs_module.mjs";
//import path from "path";

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
              ,'.mp4':'video/mp4'
              ,'.svg':'image/svg+xml'
              ,'.wav':'audio/wav'
              ,'.crt':'application/x-x509-ca-cert'
              ,'.pem':'application/x-pem-file'
              ,'.wasm': 'application/wasm'
              , '.asm': 'application/wasm' }

const requests = [];
let reqTimeout = 0;
let lastFilePath = null;
function logRequests() {
	const log = requests.join(', ');
	requests.length = 0;
	console.log( "Requests:", log );
}

exports.getRequestHandler = getRequestHandler;
function getRequestHandler( serverOpts ) {
	let resourcePath = serverOpts.resourcePath || ".";
	const npm_path = serverOpts.npmPath || ".";
	const disk = sack.Volume();

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


		const contentType = extMap[extname] || "text/plain";
		//console.log( ":", extname, filePath )
		if( disk.exists( filePath ) ) {
			const headers = { 'Content-Type': contentType, 'Access-Control-Allow-Origin' : req.connection.headers.Origin };
			if( contentEncoding ) headers['Content-Encoding']=contentEncoding;
			res.writeHead(200, headers );
			const fc = disk.read(filePath );

			if( fc ) {
				res.end( fc );

				if( requests.length !== 0 )
					clearTimeout( reqTimeout );
				reqTimeout = setTimeout( logRequests, 100 );
				requests.push( req.url );
			} else {
				console.log( 'file exists, but reading it returned nothing?', filePath );
				return false;
			}
			return true;
		} else {
			lastFilePath = filePath;
			return false;
		}
	};

}

function hookJSOX( serverOpts, server ) {
const app = uExpress();
server.addHandler( app.handle );

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

		const resultContent = "import {JSOX} from '/node_modules/jsox/lib/jsox.mjs';const config = JSOX.parse( `" + config.toString() + "`);export default config;";
		res.end( resultContent );
		return true;
	}else {
		console.log( "no file.." );
		return false;
	}
} ) 
}

exports.open = openServer;
function openServer( opts, cbAccept, cbConnect )
{
	let handlers = [];
	const serverOpts = opts || {port:process.env.PORT || 8080} ;
	const server = sack.WebSocket.Server( serverOpts )

	//console.log( "serving on " + serverOpts.port, server );
	//console.log( "with:", disk.dir() );

	const reqHandler = getRequestHandler( opts );
	server.onrequest = (req,res)=>{
		for( let handler of handlers ) {
			if( handler( req, res, serverOpts ) ) {
				//console.log( "handler accepted request..." );
				return true;
			}
		}

		if( !reqHandler( req,res ) ) {
			if( requests.length !== 0 )
				clearTimeout( reqTimeout );
			reqTimeout = setTimeout( logRequests, 100 );
				
			requests.push( "Failed request: " + req.url + " as " + lastFilePath );
			res.writeHead( 404 );
			res.end( "<HTML><HEAD><title>404</title></HEAD><BODY>404<br>"+req.url+"</BODY></HTML>");
		}
	}

	server.onaccept = function ( ws ) {
			console.log( "send accept?", cbAccept );
		if( cbAccept ) return cbAccept.call(this,ws);
		if( process.env.DEFAULT_REJECT_WEBSOCKET == "1" )
			this.reject();
		else
			this.accept();
	};

	server.onconnect = function (ws) {
		if( cbConnect ) return cbConnect.call(this,ws);
		ws.nodelay = true;
		ws.onmessage = function( msg ) {
			// echo message.
			ws.send( msg );
		};
		ws.onclose = function() {
			console.log( "Remote closed" );
		};
	};

	const serverResult = {
		setResourcePath( path ) {
			resourcePath = path;	
		},
		addHandler( handler ) {
			handlers.push( handler );
		},
		removeHandler( handler ) {
			const index = handlers.findIndex( h=>h===handler );
			if( index >= 0 )
				handlers.splice( index, 1 );
		}
	}
	hookJSOX( serverOpts, serverResult );
	return serverResult;
}

