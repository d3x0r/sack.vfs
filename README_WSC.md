
### Websocket Client Interface


``` js
const ws = sack.WebSocket.Client( /*url*/, /*protocol*/, /*options*/ );
```


Client constructor parameters
  - URL - the URL to connect to
  - protocol - the protocol for this connection.  Accepts either a string or an array of strings to send as protocols.
  - options - an object defining options for the connection.
  
    | Client Options |   |
    |---|---|
    | perMessageDeflate | true/false.  false is the default. (unimplemented yet) |
    | masking | true/false; required by RFC, default is TRUE |
    | perMessageDeflate | true/false; default false (performance).  Reqeusts permessage-deflate extension from server, and deflates messages sent. |
    | perMessageDeflateAllow | true/false; default false (performance).  Accepts permessage-deflate from server, but does not deflate messages. |    
    | ca | &lt;string&gt;; uses PEM certificate as part of certificate chain to verify server. |
    | key | &lt;string&gt;; keypair to use for the connection (otherwise an internal key will be generated) |
    | passPhrase | &lt;string&gt;; password to use with the key specified |
    
  After opening, websocket object has 'connection' which gives some of the information about the connection established.

Client events

  | Event Name | Event Description |
  |---|---|
  |open | callback receives a single argument (websocket) |
  |message | callback receives a message argument, its type is either a string or an ArrayBufer |
  |close | callback is called when the server closes the connection |
  
Client Methods

   | method | purpose |
   | -- | -- |
   | ping() | Generate a websocket ping.  /* no response/event? */ |
   | close() | call this to end a connection |
   | send(message) | call this to send data to a connection, argument should either be a String or an ArrayBuffer.
   | onopen | sets the callback to be called when the connection opens. |
   | onmessage | set the callback to be called when a message is received, it gets a single parameter, the message recieved. The data is either a string or an ArrayBuffer type | 
   | occlose | sets the callback to be called when the connection is closed from the other side first |
   | onerror | sets the callback to be called on an error (no supported errors at this time) |
   | onerrorlow | sets the callback to be called on a low level error event.  (SSL negotation failed), callback gets (error,connection,buffer), and maybe used to check the buffer, disable SSL, and possibly serve a redirect. |
   | on  | sets event callbacks by name.  First parameter is the event name, the second is the callback |
   | noDelay | setter that sets network option TCP_NODELAY to the boolean specified. |



##WebSocket connection Object

  | Connection Field | Usage |
  |----------|----------|
  | localFamily | &lt;String&gt; value is either 'IPv4' or 'IPv6' |
  | localAddress | &lt;string&gt; the IP address of the local side of the connection |
  | localPort | &lt;Number&gt; port number of the local connection |
  | localMAC | &lt;string&gt; Mac address of local socket, 00:00:00:00:00:00 if localhost. |
  | remoteFamily | &lt;String&gt; value is either 'IPv4' or 'IPv6' |
  | remoteAddress | &lt;string&gt; the IP address of the remote side of the connection |
  | remotePort | &lt;Number&gt; port number of the remote connection |
  | remoteMAC | &lt;string&gt; Mac address of remote socket (if available?), 00:00:00:00:00:00 if localhost. |
  | headers | &lt;Object&gt; field names are the names of the fields in the request header; values are the values of the fields.<BR> Client side connections may not have headers present.  |

## Network Address Object (Network.Address)
``` js
var sack = require( 'sack.vfs' );
var address = sack.Network.Address( address string [, port] );

console.log( "test addr:", sack.Network.Address( "google.com", 80 ) );
console.log( "test addr:", sack.Network.Address( "google.com:443", 80 ) );

```

Address string can contain an optional port notation after a colon(':')

| Address Members | Description |
|----|----|
| family | const String; value is either 'IPv6' or 'IPv4' |
| address | const String; address the address object was created with. |
| IP | const String; numeric IP address converted to a string |
| port | const Number; port number this address refers to (0 if not applicable, as in a unix socket) |

