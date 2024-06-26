
const sack = require( "sack.vfs" );
const path = require( "path" );

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
              ,'.svg':'image/svg+xml'
              ,'.wav':'audio/wav'
              ,'.crt':'application/x-x509-ca-cert'
              ,'.pem':'application/x-pem-file'
              ,'.wasm': 'application/wasm'
              , '.asm': 'application/wasm' 
						}

function openServer( opts, cb )
{
	var serverOpts = opts || {port:Number(process.argv[2])||8080} ;
	var server = sack.WebSocket.Server( serverOpts )
	var disk = sack.Volume();
	console.log( "serving on " + serverOpts.port );
	console.log( "with:", disk.dir() );


	server.onrequest = function( req, res ) {
		const ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
			 req.connection.remoteAddress ||
			 req.socket.remoteAddress ||
			 req.connection.socket.remoteAddress;

		//console.log( "Received request:", req );
		if( req.url === "/" ) req.url = "/index.html";
		let filePath = "." + unescape(req.url);
		if( req.url.startsWith( "/node_modules/" ) && req.url.startsWith( "/node_modules/@d3x0r" ) )
			filePath="." + unescape(req.url);
		if( disk.isDir( filePath ) ) 
			filePath += "/index.html";
		let extname = path.extname(filePath);
		let contentEncoding = encMap[extname];
		if( contentEncoding ) {
			extname = path.extname(path.basename(filePath,extname));
		}

		if( disk.exists( filePath+".gz" )){
			contentEncoding = "gzip";
			filePath += ".gz";
		}
		else if( !disk.exists( filePath ) ) {
			if( disk.isDir( filePath ) ) {
				filePath += "/index.html";
			}
		}

		var contentType = 'text/html';
		//console.log( ":", extname, filePath )
		contentType = extMap[extname] || "text/plain";
		if( disk.exists( filePath ) ) {
       			const headers = { 'Content-Type': contentType };
       			if( contentEncoding ) headers['Content-Encoding']=contentEncoding;
			res.writeHead(200, headers );
			console.log( "Read:", "." + req.url );
			res.end( disk.read( filePath ) );
		} else {
			console.log( "Failed request: ", req );
			res.writeHead( 404 );
			res.end( "<HTML><HEAD>404</HEAD><BODY>404</BODY></HTML>");
		}
	};

	server.onaccept = function ( ws ) {
		ws.aggregate = true;
		if( cb ) return cb(ws)
	//	console.log( "Connection received with : ", ws.protocols, " path:", resource );
        	if( process.argv[2] == "1" )
				this.reject();
        	else
				this.accept();
	};

	server.onconnect = function (ws) {
		ws.send( "{op:init}" );
		ws.onmessage = function( msg ) {
                	// echo message.
			console.log( "websocket, echo message:", msg );
                        ws.send( msg );
                };
		ws.onclose = function() {
                	//console.log( "Remote closed" );
	        };
	};

}

exports.open = openServer;

if( !module.parent )
	openServer();
