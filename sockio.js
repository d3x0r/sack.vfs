

const sack = require( 'sack.vfs' );

const JSOX = sack.JSOX;

const WebSocketServer = sack.WebSocket.Server;
const WebSocket = sack.WebSocket.Client;

const l = {
	DefaultNamespace : new SockNamespace(),
}

class SockIO { // socket specific tracking.
	pingRequest = null;
	connect = null;
	#events = {};
	constructor(){
	}
	connect( data ) {
		// sid
		// ping intervals
		// upgrades:[]
		this.connect = data;
		this.emit( "connecting", data );
	}
	on( event, f ) {
		if( event in this.#events ) this.#events.push( f );
		else                        this.#events = [f];
	}
	emit( event, data, ackCb ) {
		if( event in this.#events ) {
			this.#events[event].forEach( cb=>cb(data) );		
		}
	}
	join( room ) {
	}
	leave( room ) {
	}

}

class SockPacket {
	type = 0;
	attachments = [];
	nsp = l.DefaultNamespace;
	id = 0;
	data = null;
	constructor() {
	}

		
//     <packet type>[<# of binary attachments>-][<namespace>,][<acknowledgment id>][JSON-stringified payload without binary]		
}

SockPacket.decode = function( msg ) {
	const packet = new SockPacket();
	packet.type = msg[0];
	
}


class SockNamespace {
	nsp = '/';
	constructor( nsp ) {
		if( nsp )
			this.nsp = nsp;
	}
}


class IO {
	#path = "/";   //  default:	/socket.io/
	#socket = null;
	#namespace = new SockNamespace();
	#namespaces = new Map();
	constructor( port, options ) {
		let socket = null;
		if( "object" === typeof port ) {
			socket = port;
			port = 0;
		}
		if( "path" in options ) {
			this.#path = options.path;
		}		
		
		this.#socket = socket || WebSocketServer( port, options.protocol, options );//
		this.#socket.on( "accept", (ws)=>{return this.#acceptSocket(ws) } );
		this.#socket.on( "connect", (ws)=>{return this.#connectSocket(ws)} );
		this.#socket.on( "request", (req,res)=>{return this.rejectHTTP(req.res)} );
	}

	of( nsp ) {
		const n = new SockNamespace();
		this.#namespaces.set( nsp, n );
		return n;
	}

	on( event, f ) {
		this.#namespace.on(event,f);
	}

	emit( event, data, ackCb ) {
		
	}

	

	#acceptSocket( ws ) {
		ws.accept();
	}

	#connectSocket( ws ) {
		const sockIO = new SockIO(ws);
		
		ws.on( "message", decodeSockIO );
		ws.on( "close", endSocket );
	   
		this.emit( "connection", sockIO );

		function rejectHTTP( req, res) {
			res.end():
		}
	   
   
		function endSocket() {
			
		}
		
		function decodeSockIO( msg ) {
			const op = msg[0];
			switch( op ) {
			case '0':  // connect
				const data = JSOX.parse( msg.slice( 1 ) );
				sockIO.connect( data );
				break;
			case '1':
				break;
			case '2': // ping
we				this.send( "3" + msg.slice(1) ); // pong with same payload
				break;
			case '3': // pong
				
				break;
			case '4':
				break;
			case '5':
				break;
			case '6':
				break;
			}
		}

	}


}



IO.Server = function( ) {
}

class SockIOReq {
	CONNECT : 0,
        /*
        {
  "type": 0,
  "nsp": "/admin",
  "data": {
    "token": "123"
  }
  
  0{"token":"123"}
  0/admin,{"token":"123"}
  
}
*/
        DISCONNECT: 1,
        /*
        {
  "type": 1,
  "nsp": "/admin"
}
1/admin,

< 2probe                                        => Engine.IO probe request
> 3probe                                        => Engine.IO probe response
> 5                                             => Engine.IO "upgrade" packet type
> 42["hello"]
> 42["world"]
> 40/admin,                                     => request access to the admin namespace (Socket.IO "CONNECT" packet)
< 40/admin,{"sid":"-G5j-67EZFp-q59rADQM"}       => grant access to the admin namespace
> 42/admin,1["tellme"]                          => Socket.IO "EVENT" packet with acknowledgement
< 461-/admin,1[{"_placeholder":true,"num":0}]   => Socket.IO "BINARY_ACK" packet with a placeholder
< <binary>                                      => the binary attachment (sent in the following frame)
... after a while without message
> 2                                             => Engine.IO "ping" packet type
< 3                                             => Engine.IO "pong" packet type
> 1                                             => Engine.IO "close" packet type
1/admin,
	*/
        EVENT : 2,
        CONNECT_ERROR : 4,
        BINARY_EVENT : 5,
        BINARY_ACK : 6,
        
        /*
        <packet type>[<# of binary attachments>-][<namespace>,][<acknowledgment id>][JSON-stringified payload without binary]
        
        */
        
        
        
        /*
        {
  "type": 3,
  "nsp": "/admin",
  "data": [],
  "id": 456
}
	*/     
        
};


class SockIORes {
	ACCEPT : 0,
        /*
        {
  "type": 0,
  "nsp": "/admin",
  "data": {
    "sid": "CjdVH4TQvovi1VvgAC5Z"
  }
}
	*/
        DISCONNECT: 1,
        /*
        {
  "type": 1,
  "nsp": "/admin"
}
*/
        EVENT : 2,
        {
  "type": 3,
  "nsp": "/admin",
  "data": [],
  "id": 456
}

{
  "type": 2,
  "nsp": "/admin",
  "data": ["project:delete", 123],
  "id": 456
}
	ACK : 3,
        {
  "type": 3,
  "nsp": "/admin",
  "data": [],
  "id": 456
}
        
}

class IO {

	A packet contains the following fields:

a type (integer, see below)
a namespace (string)
optionally, a payload (Object | Array)
optionally, an acknowledgment id (integer)


	emit() {
        	switch( 
        }
}




/*


const socket = io();

const socket = io("https://server-domain.com");
const socket = io("https://server-domain.com/admin");





*/