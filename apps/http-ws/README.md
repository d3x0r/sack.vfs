
# Http-ws application

This is a shell static web resource server, and websocket endpoint.

## Usage


``` js
import {openServer} from "/node_modules/sack.vfs/apps/server.mjs";
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs";  

const server = openServer( { /* options */ }, accept, connect );

function accept(ws ) {
	/* `this` is the server socket handle.
    */
	const proto = ws.headers['Sec-WebSocket-Protocol'] || (ws.headers['Sec-Websocket-Protocol'] /* horray for heroku*/);
	if( 'Sec-Websocket-Protocol' in ws.headers )
		setTimeout( ()=>keepAlive( ws ), 15000 ); // keep alive for heroku

	// check protocol(s) to see if this should be accepted.
	this.accept();
	// this.reject(); // do not accept; reject gracefully with an HTTP response.
}

function connect(ws) {
	ws.onopen = opened;
	ws.onmessage = handleMessage;

	function opened() {
		// this is a good place for the server to announce the current state.
		const msg = { op: "init" /*, extra:"parameters" */ };
		ws.send( JSOX.stringify( msg ) );
	}

	function handleMessage( msg ) {

		// if msg is a string, it was text.
		// if msg is an ArrayBuffer, it was a binary message.

		const parsed = JSOX.parse( msg );
		switch( parsed.op ) {
		default:
			console.log( "Unhandled message:", parsed );
			break;
		}
	}
}


```

### Server Options

|Name|Type|Default|Description|
|---|---|---|---|
| resourcePath | string | "." | Where static reousrce base path is |
| npmPath | string | "." | Where node_modules for this project can be found |
| port | number | process.env.PORT or 8080 | port to open server on |
| - see also - |  |  | [core websocket server options](../../README_WSS.md#server-options) |


## CJS loading

Alternatively using require syntax, replace the imports at the top of the script.

``` js

const openServer = require( "sack.vfs/apps/server.js" );
const JSOX = require( "jsox" );

```

## Protocol

A simple protocol, as put forth by Mozilla's sample application https://hacks.mozilla.org/2012/03/browserquest, is described below.

The first thing one needs is a message identifier. Something short like 'op' for the operation the message is 
carying.  Could be 'do' or whatever, but 'op' is a nice standard that rarely conflicts with optional parameters.

In addition to an operation, there are often additional parmeters associated with a request.  These additional parameters
can be built into the message object, below is a sample.

``` js

const msg = { op: "doSomething",
	delay: 5000,
   text: "Hello World!" };

```

So for a command/event 'doSomething' it is probably expecting 2 arguments `delay` and `text` such that maybe that turns into
a command to echo a string on the server.

``` js

switch( msg.op ) {
case "doSomething":
	setTimeout( ()=>console.log( msg.text ), msg.delay );
	break;
/* and your typical default handler */
}

```

This might be encoded for transport using JSON, but a more complex object might use JSOX which can tranport `Date()` and, 
binary buffers, along with a variety of other types that self revive as appropriate core JS objects; additional
user encoding/decoding can be registered to revive application specific classes directly.


## Client

This is an example client; the browser side code can of course take any form you desire, this is just a 
quick summary that would work.

### Client Protocol

The protocol field can be any string; it can also be an array of strings. There are edge cases between what browsers
encode for the argument and what the server receives. A string works on all browsers; while null, and arrays containing
empty and null members might not work as expected(?).

See also [JSOX](https://github.com/d3x0r/jsox#readme), which is an extended JSON and accepts any JSON, or JSON(5,6).
Stringify encodes additional native tyeps like Date(); which should exist as a primitive.

``` js

import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"

const local = {
 	ws: null;
}

// this starts a local connection
function connect() {
	// this works generally to replace the http source with a ws source preserving the port.
	// "Tasks" 
  	const ws = new WebSocket(location.protocol.replace( "http", "ws" )+"//"+location.host+"/", "Protocol");
	local.ws = ws;
	ws.onopen = function() {
    // Web Socket is connected. You can send data by send() method.
    //ws.send("message to send"); 
    //ws.send( JSOX.stringify( { op: "subscribe", group:"lobby" } ) );
	};
	ws.onmessage = function (evt) { 
  		const received_msg = evt.data; 
		processMessage( JSOX.parse( received_msg ) );
	};
	ws.onclose = function(evt) { 
		// evt has 'code' and 'reason'
		setTimeout( ()=>connect(), 5000 );  // reconnect after 5 seconds.
		// websocket is closed. 
	};

	function processMessage( msg ) {
		switch( msg.op ) {
		default:
			console.log( "Unhandled operation:", msg.op, "with details:", msg );
		}
	}


}
connect();

```

## Micro Express

Includes a simple router which can be applied to a server mentioned above.

Supports only most basic express-like interface.  Route matching can be done by String or RexExp.

``` js
import {uExpress} from "sack.vfs/apps/http-ws/uexpress.mjs"

const app = uExpress();
app.get( "/Token", (req,res)=>{
	console.log( "Mapped reqeust to handler." );
		res.writeHead( 200 );
		res.end( "<HTML><HEAD><title>200</title></HEAD><BODY>Routed Successfully.</BODY></HTML>");

	//res.send();
} );

```

## Websocket quick service

Importing protocol.mjs

This defaults to using location.origin as where to connect back to. 
Protocol supports 'on' events, and then the protocol can just implement send and receive events.

See also apps/http-ws/protocol-example

```
import {Protocol} from "sack.vfs/client-protocol";

class MyProtocol extends Protocol {
	constructor(  ) {
		super( "MyProtocol" );
	}
}

```

Server...

```
import {Protocol} from "sack.vfs/protocol";

class MyProtocol extends Protocol {
	constructor( opts ) {
		super( opts );
		// optional message handler handle messages...
		this.on( "message", (ws,msg)=>{
			
		}
	}
}


//const protocol = new MyProtocol( {port:5252 } );

// otherwise users of 
protocol can use protocol.on( "msgtype" ) where "msgtype" will be the command of 'op' contained in a mesage.

```

