


# SSH Module

The SSH2 module provides functions for establishing SSH2 connections, executing commands,
and transferring files over SSH2 protocol. It uses the libssh2 library for SSH2 operations.

The main class in this module is SSH2Session, which encapsulate the
SSH2 connection and session objects respectively. These classes provide methods for
establishing connections, executing commands, and transferring files.

The SSH2 module also includes utility functions for error handling, logging, and memory
management related to SSH2 operations.


[Examples](tests/ssh/)

# usage


(should just link to examples)


``` js 

import {sack} from "sack.vfs"

// optionally may want to load a certificate to authenticate
//const enc = new TextDecoder("utf-8"); // convert arraybuffer to string
//const priv = enc.decode( sack.Volume.mapFile( "rsa" ) );


// this is an SSH object that provides interface to SSH connection to a ssh server
const ssh = sack.SSH();

// the first promise is a connect promise, but it is resolved by a promise to the auth 
// Connect is passed an option object with address to connect to, the port (if specified)
// is overridden by a port in the address string.  (If they are both specified, then the one
// address is wins).
const connectPromise = ssh.connect( { address:"localhost:2222", port:22
             , user:"username", password: "password"
             //, privKey: null // optional private key String to use.
             //, pubKey:null  // (optional) pubKey can be gotten from the private key
             , trace: false // enable debug from libssh2 (if compiled in)
             } );

const loginPromise = connectPromise.then( (fingerprint) =>{
    //Allows inspection of fingerprint to server for their presented host information
    // nothing is really required here  The result of this is resolved into connect promise.
    // the authentication method also passed, and now channels can be started. (handshake complete)
    return ssh;
})
```

### sack.SSH() Object methods.

- connect( options) - connects SSH to a remote and does handshake.  Options may skip login, and the fingerprint can be examined before login completes.
- login() - finishes a connection if skip login option was enabled.
- Channel() - returns a new channel to the remote.  Returns a promise that resolves with the new channel; may reject on error.
- forward(localAddrress, localPort, remoteAddress, remotePort) - forwards a connection to the remote from a locally hosted listener.  `sshd` you are connected to will probably open a remote socket to the specied remote address.  Addresses are strings and Ports are 16 bit unsigned numbers.
- reverse(addr,port) - has the `sshd` this connects to open a port on the remote side to accept listeners. Returns a promise that resolves with a listener; or rejects on error.  Connections to that listener are fowarded as channel 

## Channel creation

After authentication, channels may be created.  Channels can have options.  The `type` field specifies
the sort of service being requested... like a shell or other service?  These really just have a few 
reserved variabled depending on the sshd server I suppose.

If no options are passed to the channel, these are the default values used internally.


Channel objects have a few methods

- binary = true/false - set whether tha read() callback is passed an arraybuffer or string.
read( cb) - set a callback to be called when data is received.  It receives parameters (stream, string/buffer, depending on binary setting above )
- send( [stream,] string/arraybuffer ) - send data to the channel, an optional stream number can be passed, it defaults to 0 or what is typically stdio (stderr is received as 1, I don't know that it's legal to send on stderr; other applications may used stream IDs for multiple purposes).
- pty() - establishses a pty on the connection (mostly an interface the other side needs).  Returns a promise that resolves to Undefined.  May reject with error.
- setenv(key,value) - sets a remote environment data, doesn't seem to work for me actually.  returns a promise that completes when it finished or rejects with an error.
- shell() - opens the default remote shell.  Returns a Promise that resolves when the shell has started, may reject with an error;
- exec( cmd) - sends a command to exec().  Returns a Promise that resolves when the command has executed (finish is probably by close); may reject with an error.
- close() - closes the channel.  No return value.



``` js
    // this can be stacked without waiting for any of the above promises
    const channel = await ssh.Channel( { type:"session", message: "pre-message", windowSize: 4096, packetSize: 4096 } ).then(             (channel)=>{
						console.log( "First channel result?", channel );
                        // should make sure to return the channel - although you could use the
                        // channel in this context to initialize it... or even work with it.
                        return channel;
					} );
    const channel
```




## Starting a generic shell

After having a channel, a 'read' for data from the remote channel can be provided.
The buffer result is either a string or an array buffer, depending on the option

channel.pty() opens a user rectangular terminal sort of control.
channel.read(cb) sets up a callback to receive data from the remote.
channel.shell() starts a default shell in the pty.
channel.send(...)  sends data to the remote, an optional stream identifier may be passed

The operations do not have to wait for the previous result, the commands end up a queue 
to libssh2 to handle the command sequentially anyway; otherwise you could nest setenv in pty
and shell in setenv (if it was critical to resolve).

``` js
    console.log( "Generic channel result:", channel );
    channel.read( (buf)=>{
        console.log( "Channel data:", buf );
    } );
    channel.pty();
    channel.setenv( "FOO", "bar" );
    channel.shell().then( ()=>{
        channel.send( "ls\n" );
    } );
```

## Using a connection(Really a channel) to forward a listener


"In OpenSSH, local port forwarding is configured using the -L option:
    ssh -L 80:intra.example.com:80 gw.example.com"

"In OpenSSH, remote SSH port forwardings are specified using the -R option. For example:
    ssh -R 8080:localhost:80 public.example.com"


This is equivalent to -R.

```
const some_value = ssh.reverse( "::0", 8022 ).then( (listener)=>{
    // there is no generic port fowward to a local address also...
    // that a connection with this listener would then do something like 
    return (some_value)    
} );
```

- `reverse()` returns a promise that resolves with a listener object. It may reject with an error. It's equivalent to the socket it is listening for on the remote address and port specified. 

the Listener object has 'close' as a operation.
It should also have an accept() to register a callback that receives new socket connections
And that should be passed to the handler as a channel, and use channel 'send' and 'read'.



## Using a forward listener to host WebSocket/HTTP

 sack.WebSocket.ssh2ws( listener ) - creates a forward listener which accepts WebSocket/HTTP.  It does magic internally; but makes the interface equivalent for JS.  So all of the following ws events are 
 pretty typical.

In the event callback, this code also starts a websocket object, which listens on a port for a specific http request; if the http request is something else, it generates an event to the `onrequest(cb)` callback specified.  (The ws object is a WebSocketServer object [documented here]](README_WSS.md)).

The request handler only serves that it can't find anything; but the request can be examined and operations performed.

``` js
    ssh.reverse( "::0", 8022 ).then( (listener)=>{
        console.log( "Reverse connection listener:", listener );
        const ws = sack.WebSocket.ssh2ws( listener );
        console.log( "New websocket server:", ws );
        ws.onrequest = (request,response)=>{
            console.log( "Request happened:", request, response );
            response.writeHead( 404 );
            response.end( "<HTML><HEAD><title>404</title></HEAD><BODY>404</BODY></HTML>");
        };
        ws.onaccept = (ws)=>console.log( "Accept ws?", ws );
        ws.onconnect = (ws)=>{
                console.log( "Connected ws?", ws );
            ws.onmessage = (msg)=>{
                console.log( "Received message:", msg );						
            }
            ws.onclose = (code, reason ) =>console.log( "websocket close:", code, reason );
        }
    } );
```