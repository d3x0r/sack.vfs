

var sack = require( ".." );
const path = require( "path" );

function ExpressX() {
	/* router really */
	function makeRouter() {
		return {
			pre_mappings : new Map(),
			pre_req_maps : [],
			mappings : new Map(),
			req_maps : []
		};
        	
	}

	const defaultMap = makeRouter( );
	
	const pre_mappings = defaultMap.pre_mappings;
	const pre_req_maps = defaultMap.pre_req_maps;
	const mappings = defaultMap.mappings;
	const req_maps = defaultMap.req_maps;
        
	return {
        	all(a,b ) {
                	if( "string" === typeof a )
	                	pre_mappings.set( a, b ); // b(req,res,next /*next()*/ )
                        else
	                        pre_req_maps.push( { expr:a, cb:b } );
                },
        	get( a, b ) {
                	if( "string" === typeof a )
        	        	mappings.set( a, b );
                        else
	                        req_maps.push( { expr:a, cb:b } );
                },
        	post( a, b ) {
                	if( "string" === typeof a ) 
	                	mappings.set( a, b );
                        else
	                        req_maps.push( { expr:a, cb:b } );
                },
                handle( req, res) {
						//console.log( "Look for request:", req, res );
						const parts = req.url.split("?");
						const url = unescape(parts[0]);
						const filepath = path.dirname(url)+path.basename(url)+path.extname(url);
						const name = path.basename(url);
						const type = path.extname(url);
                        
						//console.log( "Think parts is:", filepath, name, type, parts[1] );
						let cb;
						let ranOne = false;
						if( cb = pre_mappings.get( filepath ) ) {
							let runNext = false;
							ranOne = true;
							cb( req, res, ()=>{ runNext = true; } );
							//if( !runNext ) break;
						}
						for( let map of req_maps ) {
							if( map.expr.match( filepath ) ) {
								let runNext = false;
								ranOne = true;
								map.cb( req, res, ()=>(runNext = true) );
								if( !runNext ) break;
							}
						}

						//console.log( "mappings:", mappings );
                	if( cb = mappings.get( filepath ) ) {
								console.log( "got cb?" );
								ranOne = true;
                       	cb( req, res, ()=>{} );
						}
						return ranOne;
				}
        }
}

const ex_app = ExpressX();
ex_app.get( "/Token", (req,res)=>{
	console.log( "Mapped reqeust to handler." );
		res.writeHead( 200 );
		res.end( "<HTML><HEAD><title>200</title></HEAD><BODY>Routed Successfully.</BODY></HTML>");

	//res.send();
} );

function openServer( opts, cb )
{
	const serverOpts = opts || {port:Number(process.argv[2])||8181} ;
	const server = sack.WebSocket.Server( serverOpts )
	const disk = sack.Volume();
	console.log( "serving on " + serverOpts.port );
	console.log( "with:", disk.dir() );


	server.onrequest( function( req, res ) {

		if( ex_app.handle( req, res ) ) {
                	return;
                }

	const ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
		 req.connection.remoteAddress ||
		 req.socket.remoteAddress ||
		 req.connection.socket.remoteAddress;
	//ws.clientAddress = ip;

	//console.log( "Received request:", req );
	if( req.url === "/" ) req.url = "/index.html";
	const filePath = "." + unescape(req.url);
	const extname = path.extname(filePath);
	const contentType = 'text/html';
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

