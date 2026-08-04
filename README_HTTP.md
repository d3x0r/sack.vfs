### HTTP Request Object Description

## HTTP Request Interface ( HTTP/HTTPS )

``` js
var sack = require( "sack.vfs" );
var response = sack.HTTP.get( { hostname: "example.com", port: 80, method : "get", path : "/" } );
var response2 = sack.HTTPS.get( { ca:&lt;extra cert(s)&gt;, path:"/index.html" } );
```

| HTTP(S) get option | Description |
|----|-----|
| hostname | address to request from |
| path | resource path to request; "/app/index.html"  |
| port | optional to override the port requested from |
| method | "GET"/"POST" specifies how to send the request.  If POST is used, then content should be filled in. |
| content | This is the content to send with a POST. |
| rejectUnauthorized | (HTTPS only) whether to accept unvalidated HTTPS certificates; true/false |
| timeout | How long to wait for a response - 3000ms if unspecified. |
| retries | How many times to attempt a request - 3 if unspecified. |
| ca | (HTTPS) Additional certificate authorities to validate connection with |
| headers | object with named values; copied to header object (no HTTP character escapes) |
| onReply | callback to call when response is done.  If this is specified, then the request is done asynchronously; otherwise the request is synchronous and completes before the function returns |

Results with an object with the following fields....

| HTTP Response field | Description |
|----|----|
| content | string content from request |
| statusCode | number indiciating the response code form the server |
| status | text status from server |
| headers | array of header from response (should really be an object, indexes are field names with field values specified) |


## HTTP Streaming Interface ( HTTP/HTTPS )

`get()` above is one request per connection - it opens a socket, sends, waits,
and the socket is done.  `stream()` opens the connection once and lets any
number of requests be issued against it.  Nothing blocks; every request results
with a promise.

``` js
import { sack } from "sack.vfs";   // await at the top level needs a module

const conn = await sack.HTTP.stream( { hostname: "example.com", port: 80 } );

const res = await conn.request( { method: "GET", path: "/index.html" } );
console.log( res.statusCode, res.content );

// several at once; they go out in order and results come back in order
const [ a, b, c ] = await Promise.all( [
        conn.request( { path: "/a" } ),
        conn.request( { path: "/b" } ),
        conn.request( { path: "/c" } ),
] );

conn.close();
```

`sack.HTTPS.stream()` is the same thing over TLS.

| HTTP(S) stream option | Description |
|----|-----|
| hostname | address to connect to.  May carry the port ("host:8080", "[::1]:8080"), which overrides `port` |
| port | port to connect to; 80 for HTTP, 443 for HTTPS if unspecified |
| pipeline | how many requests may be on the wire at once; 1 (the default) sends the next request when the previous result arrives |
| rejectUnauthorized | (HTTPS only) whether to accept unvalidated HTTPS certificates; true/false |
| ca | (HTTPS) Additional certificate authorities to validate connection with |
| preferV4 / preferV6 | prefer an IPv4 or IPv6 address when the name resolves to both |

`stream()` results with a promise that resolves with the connection once it is
connected (and for HTTPS, once the handshake is done).  It rejects if the
connection could not be made.  There is no automatic reconnect; if the
connection is lost, open another one.

### Connection methods

| Method | Parameters | Description |
|----|----|-----|
| request | (options) | send a request; results with a promise for the response |
| close | () | close the connection.  Requests that have not been answered reject. |

An open connection keeps the process alive, the same way an open socket does -
whether or not a request is currently in flight.  "No request right now" is not
"no request coming", so an idle connection is still holding the door open for
the next one.  `close()` is what lets the process exit.

| request option | Description |
|----|-----|
| path | resource path to request; "/app/index.html".  "/" if unspecified |
| method | "GET"/"POST"/"PUT"/... ; "GET" if unspecified |
| content | content to send with the request |
| headers | object with named values; copied to header object (no HTTP character escapes) |
| version | HTTP version to request with; "1.1" if unspecified |
| agent | User-Agent field to send |

`request()` results with a promise that resolves with the same response object
`get()` results with, plus `bytes`:

| HTTP Response field | Description |
|----|----|
| content | string content from the response |
| bytes | ArrayBuffer of the same content, undecoded |
| statusCode | number indiciating the response code from the server |
| status | text status from server |
| headers | array of header from response (indexes are field names with field values specified) |

The status code does not decide whether the promise resolves - a 404 or a 500
resolves like any other response, and it is up to the caller to decide what
counts as a failure.  The promise only rejects when there is no response to be
had; that is, the connection broke, or was closed before the request was
answered.

### About ordering and pipelining

HTTP/1.1 has no way to match a response to a request other than the order they
were sent, so responses are matched in order.  One slow response holds up the
ones queued behind it.

By default only one request is on the wire at a time - the next is written when
the previous response arrives.  Setting `pipeline` higher packs that many
requests ahead, which is faster but head-of-line blocked, and some servers and
proxies handle pipelined requests badly.  Leave it at 1 unless the other end is
known to be happy with more.


Http Request/Server Client fields

  | Name  | Description |
  |----|----|
  | url | the URL requested |
  | connection | same as a Websocket Connection object |
  | headers | headers from the http request |
  | CGI | Parsed CGI Parameters from URL |
  | content | if the message was a POST, content will be non-null |

Server Client Events

  | Event Name | Event Description |
  |---|---|
  |message | callback receives a message argument, its type is either a string or an ArrayBufer |
  |error | unused (probably).  Caches websocket protocol errors. |
  |close | callback is called when the server closes the connection |


Http Response methods
   These methods are available on the 'res' object received in the Server "request" event.

  | Method | Parameters | Description |
  |----|----|-----|
  | writeHead | (resultCode [,extraHeadersObject]) | setup the return code of the socket.  Second parameter is an object which is used to specify additional headers. |
  | end | ( content [,unused]) | sends specified content.  String, Buffer, uint8Array, ArrayBuffer area all accpeted.  (sack.vfs.File?) |

