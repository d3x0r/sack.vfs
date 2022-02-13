

var sack = require( ".." );
const path = require( "path" );

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
	var filePath = "." + unescape(req.url);
	var extname = path.extname(filePath);
	var contentType = 'text/html';
	console.log( ":", extname, filePath )
	switch (extname) {
		  case '.js':
		  case '.mjs':
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
	if( cb ) return cb(ws)
//	console.log( "Connection received with : ", ws.protocols, " path:", resource );
        if( process.argv[2] == "1" )
		this.reject();
        else
		this.accept();
} );

server.onconnect( function (ws) {
	//console.log( "Connect:", ws );
	ws.onmessage( function( msg ) {
        	//console.log( "Received data:", msg );
                ws.send( msg );
		//ws.close();
        } );
	ws.onclose( function() {
        	//console.log( "Remote closed" );
        } );
} );

}

console.log( "Export open?!" );
exports.open = openServer;

if( !module.parent )
	openServer();



/**
*
*/
class ExecPHP {
	/**
	*
	*/
	constructor() {
		this.phpPath = '/usr/bin/php';
		this.phpFolder = '';
	}	
	/**
	*
	*/
	parseFile(fileName,callback) {
		var realFileName = this.phpFolder + fileName;
		
		console.log('parsing file: ' + realFileName);

		var exec = require('child_process').exec;
		var cmd = this.phpPath + ' ' + realFileName;
		
		exec(cmd, function(error, stdout, stderr) {
			callback(stdout);
		});
	}
}

const phpLauncher = new ExecPHP();
