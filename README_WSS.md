
### Websocket Server Interface

``` js
// primary method, pass options object
const ws = sack.WebSocket.Server( /*options*/ );

// example of some sample options
const ws = sack.WebSocket.Server( { port: 1234, address: "::0" } );


// the constructor can also take a URL and port as arguments instead of in option object
// but the option object is the common method
const ws = sack.WebSocket.Server( [/*url*/, /*port*/,] /*options*/ );



```


Server Constructor Parameters
  - (optional) URL - a URL formatted string specifying server address and optional port.
  - (optional) Port - a number indicating which port number to listen on; if No URL, and no address option specified, listens on all addresses on this port.
  - Options - see Server Options below.
  
Server events

  | Event Name | Event Description |
  |---|---|
  | accept |  optional callback, if it is configured on a server, it is called before connect, and is passed (new_websocket_connection).  Should call server.accept( protocol ), or server.reject() during this callback.  |
  | connect(depr.) | callback receives new connection from a client.  The new client object has a 'connection' object which provides information about the connection. |
  | request | callback is in a new object that is an httpObject; triggered when a non-upgrade request is received. |
  | lowError | callback is passed a closed socket object with remote and local addresses. |
  | error | callback is passed a closed socket object with remote and local addresses. |


### Server Options

  When a server is created it accepts an option object with the following options specified.

  | Server Options |   |
  |---|---|
  | url | URL to listen at. |
  | address | specific address to bind to; default binds to ::0 |
  | port |  a port number.  8080 is the default. |
  | masking | true/false; browsers do not accept server masked data; RFC non-specific; default is false |
  | perMessageDeflate | true/false; default false (performance).  Accepts permessage-deflate extension from client, and deflates messages if requested by client. |
  | perMessageDeflateAllow | true/false; default false (performance).  Accepts permessage-deflate from client, but does not deflate messages on response |    
  | cert | &lt;string&gt;; uses PEM certificate as server certificate chain to send to client. |
  | key  | &lt;string&gt;; uses PEM private key specified for encryption; used by clients to authenticate cerficates  |
  | passphrase | &lt;string&gt;; uses passphrase for key provided |
  | hosts | &lt;array of hosts&gt;; (Server Host Option below) |
  
  | Server Host Option |  |
  |----|----|
  | host | &lt;string&gt; or &ltarray of strings&gt;; host name this certificate applies to |
  | cert | &lt;string&gt;; uses PEM certificate as server certificate chain to send to client. |
  | key  | &lt;string&gt;; uses PEM private key specified for encryption; used by clients to authenticate cerficates  |
  | passphrase | &lt;string&gt;; uses passphrase for key provided |
  
### Server Methods

  | Server Methods |   |
  |---|---|
  | close() | close server socket. |
  | on(eventName,cb) | setup events ( connect, accept, request, error ), callback |
  | onconnect | set to callback for when a WebSocket connection is initiated; allows inspecting protocols/resource requested to accept or reject. |
  | onaccept | set to callback for when socket is accepted, and is completely open.... setup event handlers on passed socket |
  | onrequest |set to callback for HTTP request (GET/POST/...).   |
  | onerror | set to callback for error event.  callback is passed a closed socket object with remote and local addresses (see connection object below) |
  | accept() | Call this to accept a socket, pass protocols to accept with(?).  Only valid within "accept" event. |
  | reject() | Call this in onconnect to abort accepting the websocket.  Only valid within "accept" event. |
  | disableSSL() | closes the SSL layer on the socket and resumes native TCP connection; is only valid during "lowerror" event type (6).  Uses the socket that triggered the event as the one to disable.  (The Websocket Server Client is not yet created). |


### Connected Instance Methods

  This is a slightly different object than a client, although accepts the same events except for on( "open" ) and onOpen() method.  

  
  
  | Method |  Description |
  |----|----|
  | post(target) | send this socket to another thread; best when applied during accept event handler(on accept) |
  | ping() | Generate a websocket ping.  /* no response/event? */ |
  | onmessage | set the callback to be called when a message is received, it gets a single parameter, the message recieved. The data is either a string or an ArrayBuffer type | 
  | send | send data on the connection.  Message parameter can either be an ArrayBuffer or a String. (to be implemented; typedarraybuffer) |
  | disableSSL | closes the SSL layer on the socket and resumes native TCP connection. |
  | close | closes the connection |
  | on | event handler for specified type `on(eventName, callback)` | 
  | noDelay | setter that takes a boolean and enables/disables TCP_NODELAY. | 

## HTTP Fallback

Providing a `onrequest` callback to the server, will allow the server to handle HTTP(S) request events.
The request is given a `req`uest and a `res`ponse object which are used to serve the HTTP(S) request.
request has 'connection' which has information about the local and remote address/port/MAC addresses if available.

### Websocket Node Worker-Thread Support

This is support to be able to send accepted clients to other 'worker_threads' threads in Node.
Sockets which have been accepted can be moved; there's no reason to move the listener thread...
and client connections should be made on the intended thread, so there's no support to move those either.
This is only for threads accepted by a server, which may fork a thread, and hand off said socket to the 
specified target thread.  The handoff takes a unique string which should identify the target thread; although,
a pool of threads all using the same identifier may each receive one socket.  Once the accept event is fired,
it is cleared, and the thread will have to re-post a listener to accept another socket.

``` js
// rough example, not sure about the onaccept interface

var wss = sack.Websocket.Server( "::0" );
wss.onaccept( (wsc)=>{ // wsc  = websocket client
	
    // This is another method - post the client to the specified destination.
	  // sack.Websocket.Thread.post( "destination", wsc );

    // tell the client to post itself to the destination.
    wsc.post( "destination" );
} );

// --------------------------------
// and then destination

sack.Websocket.Thread.accept( "destination", (socket)=>{
	// receve a posted socket
} );

```



``` js
        Thread - Helper functions to transport accepted clients to other threads.
            post( accepted_socket, unique_destination_string ) - posts a socket to another thread
            accept( unique_destination_string, ( newSocket )=>{} ) - receives posted accepted sockets
```


