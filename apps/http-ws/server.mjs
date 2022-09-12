
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
              ,'.json':'application/json'
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
	const serverOpts = opts || {port:process.env.PORT || 8080} ;
	const server = sack.WebSocket.Server( serverOpts )
	const disk = sack.Volume();
	//console.log( "serving on " + serverOpts.port );
	//console.log( "with:", disk.dir() );


	server.onrequest = function( req, res ) {
		/*
			this is the request remote address if required....
		const ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
			 req.connection.remoteAddress ||
			 req.socket.remoteAddress ||
			 req.connection.socket.remoteAddress;
		*/
		const npm_path = serverOpts.npmPath || ".";
		const resource_path = serverOpts.resourcePath || ".";

		//console.log( "Received request:", req );
		if( req.url === "/" ) req.url = "/index.html";
		let filePath = resource_path + unescape(req.url);
		if( req.url.startsWith( "/node_modules/" ) && req.url.startsWith( "/node_modules/@d3x0r" ) )
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
				
			requests.push( "Failed request: " + req.url );
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
                	//console.log( "Remote closed" );
		};
	};

}

//exports.open = openServer;

