
# Websockets

## Create a server

The server is created with an option object, it's best to create this as a variable
to be passed in later.

```
const serverOpts = { port:8080 };
```

Create the server...

```
const server = sack.WebSocket.Server( serverOpts )
```

The `onaccept` method is optional, but allows option to either accept or reject the connection; the default
handler will accept all protocols requested by the client, and then the connect callback will be called.

```
// register optional accept callback, default all sockets will be accepted..
server.onaccept( function ( ws ) {
	//the protocol and URL that was requested is available on the ws object.
//	console.log( "Connection received with : ", ws.protocols, " path:", resource );

        if( /* some condition to accept or reject the request * ) 
		this.reject();
        else
		this.accept();
} );
```

Provide an `onconnect` handler, which is called when a client is fully accepted, and the socket is now in an open state.

```
// callback on connect - initialize new socket with mesage and close handler.
server.onconnect( function (ws) {

	// register message handler for client
	ws.onmessage( function( msg ) {
        	// this example just echos whatever the client sends...
                ws.send( msg );
        } );
        
        // register a on-close handler for the client.
	ws.onclose( function() {
        	//console.log( "Remote closed" );
        } );
        
} );


```



## HTTP Request Handling

Registering a callback for the server's request event allows handling of content requests...

The `request` and `result` parameters work similar to the default HTTP API for node.

```
server.onrequest( function( request, result ) {
	// handleRequest( request, result );
} );
```

Get the requestors client IP; there are several standards depending on whether the client is connecting through a proxy or direct that contain the client's address.


```
	var ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
		 req.connection.remoteAddress ||
		 req.socket.remoteAddress ||
		 req.connection.socket.remoteAddress;

```


This is an example handler for parsing URLs, getting the path part and file request and serving the file content form the local file system.

This example uses the disk access methods in sack.vfs, which returns an array buffer of the disk data, this data is the handed back to the network API, which means the buffer never actually enters the JS Virtual Machine.

``` js

function handleRequest( req, res ) 
{
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
        
        // this uses 'disk' setup in `tutorial-disk` to read the file data.
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

```


