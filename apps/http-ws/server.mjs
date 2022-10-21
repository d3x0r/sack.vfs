
//const sack = require( "sack.vfs" );
//const path = require( "path" );

import {sack} from "../../vfs_module.mjs";
import path from "path";

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
              ,'.wav':'audio/wav'
              ,'.crt':'application/x-x509-ca-cert'
              ,'.pem':'application/x-pem-file'
              ,'.wasm': 'application/wasm'
              , '.asm': 'application/wasm' }

const requests = [];
let reqTimeout = 0;
function logRequests() {
	const log = requests.join(', ');
	requests.length = 0;
	console.log( "Requests:", log );
}

//exports.open = openServer;
export function openServer( opts, cbAccept, cbConnect )
{
	let handlers = [];
	const serverOpts = opts || {port:process.env.PORT || 8080} ;
	const server = sack.WebSocket.Server( serverOpts )
	const disk = sack.Volume();
	let resourcePath = serverOpts.resourcePath || ".";

	//console.log( "serving on " + serverOpts.port, server );
	//console.log( "with:", disk.dir() );


	server.onrequest = function( req, res ) {
		/*
			this is the request remote address if required....
		const ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
			 req.connection.remoteAddress ||
			 req.socket.remoteAddress ||
			 req.connection.socket.remoteAddress;
		*/
		for( let handler of handlers ) {
			if( handler( req, res, serverOpts ) ) {
				//console.log( "handler accepted request..." );
				return;
			}
		}
		const npm_path = serverOpts.npmPath || ".";
		//const resource_path = serverOpts.resourcePath || ".";

		//console.log( "Received request:", req );
		if( req.url === "/" ) req.url = "/index.html";
		let filePath = resourcePath + unescape(req.url);
		if( req.url.startsWith( "/node_modules/" ) 
		   && ( req.url.startsWith( "/node_modules/@d3x0r" ) 
		      || req.url.startsWith( "/node_modules/jsox" ) ) )
			filePath=npm_path  + unescape(req.url);
		let extname = path.extname(filePath);

		let contentEncoding = encMap[extname];
		if( contentEncoding ) {
			extname = path.extname(path.basename(filePath,extname));
		}


		const contentType = extMap[extname] || "text/plain";
		//console.log( ":", extname, filePath )

		if( disk.exists( filePath ) ) {
			const headers = { 'Content-Type': contentType };
			if( contentEncoding ) headers['Content-Encoding']=contentEncoding;
			res.writeHead(200, headers );
			res.end( disk.read( filePath ) );
			if( requests.length !== 0 )
				clearTimeout( reqTimeout );
			reqTimeout = setTimeout( logRequests, 100 );
			requests.push( req.url );
		} else {
			if( requests.length !== 0 )
				clearTimeout( reqTimeout );
			reqTimeout = setTimeout( logRequests, 100 );
				
			requests.push( "Failed request: " + req.url + " as " + filePath );
			res.writeHead( 404 );
			res.end( "<HTML><HEAD>404</HEAD><BODY>404</BODY></HTML>");
		}
	};

	server.onaccept = function ( ws ) {
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

	return {
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
}

//exports.open = openServer;

