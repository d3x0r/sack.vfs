
const sack = require( "sack.vfs" );
const path = require( "path" );

const extMap = { '.js': 'text/javascript'
              ,  '.mjs':'text/javascript'
              , '.js.gz':'text/javascript'
              , '.gz':'text/javascript'
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

function openServer( opts, cb )
{
	var serverOpts = opts || {port:Number(process.argv[2])||8080} ;
	var server = sack.WebSocket.Server( serverOpts )
	var disk = sack.Volume();
	console.log( "serving on " + serverOpts.port );
	console.log( "with:", disk.dir() );


	server.onrequest = function( req, res ) {
		var ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
			 req.connection.remoteAddress ||
			 req.socket.remoteAddress ||
			 req.connection.socket.remoteAddress;

		//console.log( "Received request:", req );
		if( req.url === "/" ) req.url = "/index.html";
		var filePath = "." + unescape(req.url);
		if( req.url.startsWith( "/node_modules/" ) && req.url.startsWith( "/node_modules/@d3x0r" ) )
			filePath="." + unescape(req.url);
		var extname = path.extname(filePath);
		var contentType = 'text/html';
		console.log( ":", extname, filePath )
                contentType = extMap[extname] || "text/plain";
		if( disk.exists( filePath ) ) {
			res.writeHead(200, { 'Content-Type': contentType });
			console.log( "Read:", "." + req.url );
			res.end( disk.read( filePath ) );
		} else {
			console.log( "Failed request: ", req );
			res.writeHead( 404 );
			res.end( "<HTML><HEAD>404</HEAD><BODY>404</BODY></HTML>");
		}
	};

	server.onaccept = function ( ws ) {
		if( cb ) return cb(ws)
	//	console.log( "Connection received with : ", ws.protocols, " path:", resource );
        	if( process.argv[2] == "1" )
			this.reject();
        	else
			this.accept();
	};

	server.onconnect = function (ws) {
		//console.log( "Connect:", ws );
		ws.onmessage = function( msg ) {
                	// echo message.
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
