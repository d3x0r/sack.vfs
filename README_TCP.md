

## TCP Socket Object (Network.TCP)

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
| ()  | not valid, requires either a string, or option object. |
| ( "localhost:1234" ) | creates a UDP socket bound to localhost port 1234 |
| ( { address: "localhost",<BR>port:1234} ) | creates a UDP socket bound to localhost port 1234 |
| ( { address: "localhost",<BR>port:1234}, (msg,rinfo)=>{} ) | creates a UDP socket bound to localhost port 1234, and specifies a message handler callback |

  can pass a string address to listen on.  "IP[:port]" format.  IPv6 IP should be enclosed in brackets;  `[::1]:80`.
  if address is not passed, an object with socket options must be specified.
  final parameter can optionally be a callback which is used as the on('message') event callback.

#### TCP socket creation options

| TCP Socket options | Type | Description  |
|----|----|---|
| port  | &lt;number&gt; |specify the port to listen on |
| address | &lt;string&gt;; |specify the address to listen on (defaults to [::]). Optional port notation can be used (specified with a colon followed by a number (or name if under linux?))<BR>If the port is specified here, it overrides the `port` option.|
| family | &lt;string&gt; |either 'IPv4' or 'IPv6' which controls the default address; otherwise address string will determine the family |
| toPort | &lt;number&gt; vspecify the port to send to if not specified in send call |
| toAddress | &lt;string&gt; |specify the address to send to if not specified in send call.  Optional port notation can be used (specified with a colon followed by a number (or name if under linux?)) <BR>If the port is specified here, it overrides the `toPort` option.|
| readStrings | &lt;bool&gt;| if `true` messages passed to message callback will be given as text format, otherwise will be a TypedArray |
| reuseAddress | &lt;bool&gt; |if `true` set reuse address socket option |
| reusePort | &lt;bool&gt; |if `true` set reuse port socket option (linux option, not applicable for windows) |
| message | &lt;function&gt; | |

#### TCP socket methods

| UDP Socket Methods | Arguments | Description  |
|-----|-----|-----|
| send | (message [,address]) | Send a message, message can be an ArrayBuffer or string,   if second parameter is passed it should be an sack.Network.Address object. |
| close | () | Close the socket. |
| on | (eventName, callback) | Set message or close callbacks on the socket. |

#### TCP Events

| TCP Events |  |  |
|----|----|----|
| message | (msg, remoteAddress) | called when a message is received.  msg parameter is either a string if socket was opened with `readStrings` or a TypedArray.  Second parameter is a Network.Socket object representing the source address of the message |
| connect | (msg, remoteAddress) | called when a message is received.  msg parameter is either a string if socket was opened with `readStrings` or a TypedArray.  Second parameter is a Network.Socket object representing the source address of the message |
| close | () | Socket has been closed. | 


Needs a good example?

~~See Also [testudp.js](https://github.com/d3x0r/sack.vfs/blob/master/tests/testudp.js) for example usage.~~
