

## TCP Socket Object (Network.TCP)

This is a simple TCP interface.  More complex interfaces might support callbacks for completion of very large packets; sending
large packets which allow the network to not duplicate the data.  The network does generally queue the data, releasing application
buffers on write.  Reads are meant to be read into a user supplied buffer, but this provides a default 4k buffer; TCP read events for
large packets will be a series of 4K buffers.

Close() is implemented gracefully, so any sent data will still be sent, but no further data will be received.  When any pending writes
are completed, the socket is fully closed.

Addresses are strings.  "IP[:port]" format.  IPv6 IP should be enclosed in brackets;  `[::1]:80`.


``` js
var sack = require( 'sack.vfs' );
var tcp  = sack.Network.TCP( "localhost:5555", (msg,rinfo)=>{ console.log( "got message:", msg ) } );
var tcp2 = sack.Network.TCP( {port:5556, address:"localhost", toAddress:"localhost:5555" }, (msg,rinfo)=>{ console.log( "got message:", msg ) } );
var tcp3 = sack.Network.TCP( {port:5557, address:"localhost", toAddress:"localhost", toPort:5555, message:(msg,rinfo)=>{ console.log( "got message:", msg ) } );

tcp2.send( "Hello World" );

```

#### sack.Network.TCP() Invokation

| Construction examples |  |
|-----|----|
| ( { option object } )  | Option object to initialize socket with.  |

#### TCP Object Methods

| Construction examples |  |
|-----|----|
| ports | getter | Get a list of listening ports and associated process identifiers.  |


#### TCP socket creation options

| TCP Socket options | Type | Description  |
|----|----|---|
| port  | &lt;number&gt; |specify the port to listen on |
| address | &lt;string&gt; |specify the address to listen on (defaults to [::]). Optional port notation can be used (specified with a colon followed by a number (or name if under linux?))<BR>If the port is specified here, it overrides the `port` option.  This triggers the tcp object to behave as a server for the connect callback.|
| family | &lt;string&gt; |either 'IPv4' or 'IPv6' which controls the default address; otherwise address string will determine the family |
| toPort | &lt;number&gt; vspecify the port to send to if not specified in send call |
| toAddress | &lt;string&gt; |specify the address to connect to; using toPort (if any) as a default port.  This triggers the TCP object to be a client to a server.|
| readStrings | &lt;bool&gt;| if `true` messages passed to message callback will be given as text format, otherwise will be a TypedArray |
| reuseAddress | &lt;bool&gt; |if `true` set reuse address socket option |
| reusePort | &lt;bool&gt; |if `true` set reuse port socket option (linux option, not applicable for windows) |
| message | &lt;function&gt; | receives a buffer as a parameter.  The buffer is either a Uint8Array or a string, depending on readStrings option.  |
| connect | &lt;function&gt; | For a server, this receives a new connection.  For a client, this will receive `undefined` or a number that is the error of the connection. |
| close | &lt;function&gt; | called when a connection is closed.  No parameters are given. |
| ssl | &lt;bool&gt; | Set to enable SSL on client sockets, without further certificate information. (for client) |
| host | &lt;string&gt; | Host name(s) that match the default cerfiticate; names separated by '~' |
| cert | &lt;string&gt; | cerficiate for server socket to use to accepted TLS connections. (for server) |
| key | &lt;string&gt; | private key associated with cerfificate for TLS connections. (for server) |
| passphrase | &lt;string&gt; | password for private key. |
| ca | &lt;string&gt; | Certificate authority chain for client to use to authenticate server key (optional, unimplemented?) |
| hosts | [ {/*host option*/}] | Array of host definitions; includes members `host`, `cert`, `key`, `passphrase`.  |
| allowSSLfallback | true/false | controls whether the the socket will automatically demote from TLS/SSL to raw.  If set to false, sockets with SSL that fail negotiation are closed. |
| ready | callback () | is called when a socket is ready; this is alternative method to specify the callback to .on( "ready", ... ) |
| message | callback (buffer) | is called when a socket receives a message; this is alternative method to specify the callback to .on( "message", ... ) |
| close | callback () | is called when a socket is closed;  |


If both `address` and `toAddress` are specified, then it is a client connection, which uses address as the address to `bind()` to.  (Untested)

Errors connect returns depend on the system; windows system will return windows websock errors, while linux/mac will return POSIX(?) errors.

#### TCP socket methods

| TCP Socket Methods | Arguments | Description  |
|-----|-----|-----|
| send | (message [,address]) | Send a message, message can be an ArrayBuffer or string,   if second parameter is passed it should be an sack.Network.Address object. |
| close | () | Close the socket. |
| on | (eventName, callback) | Set message or close callbacks on the socket. |
| addCert | (hosts, cert, privateKey, passPhrase ) | Add a cerficate option to a socket.  Similar to specifying this in options for socket creation; but can add certificates after socket creation. |
| ssl | setter true/false | enable/disable ssl; reading this returns secure status(yes/no) of client. |
| readStrings | setter true/false | controls whether the next read is text or a byte buffer |
| allowSSLfallback | setter true/false | controls whether the the socket will automatically demote from TLS/SSL to raw.  If set to false, sockets with SSL that fail negotiation are closed. |

#### TCP Events

| TCP Events |  |  |
|----|----|----|
| message | (msg) | called when a message is received.  msg parameter is either a string if socket was opened with `readStrings` or a TypedArray.  Second parameter is a Network.Socket object representing the source address of the message |
| connect | () | called when a connection happens. This allows setting additional event handlers on the socket, but the socket is not fully connected.  |
| ready | () | Socket is now ready to send data.  Connect is called when 
| close | () | Socket has been closed. | 
| error | (n [,...]) | error callback, used for conditions in the TLS layer to report errors.  Depending on the error, may be additional parameters.  |


#### TCP Error Codes

| Code | Description |
|----|----|
| 7 | host name mismatch.  Additional parameter is host name requested |
| 6 | TLS handshake failed.  Additional parameter is initial buffer that failed negotiation.  Passing this buffer to disableSSL() allows non-TLS fallback to parse message |
| 1-5 | Various low level TLS errors... (they don't occur very often?) |

Needs a good example?

See Also [TCP Server Client Example](https://github.com/d3x0r/sack.vfs/blob/master/tests/tcp/) for example usage.
