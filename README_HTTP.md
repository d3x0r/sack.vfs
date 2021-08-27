### HTTP Request Object Description

## HTTP Request Interface ( HTTP/HTTPS )

``` js
var sack = require( "sack.vfs" );
var response = sack.HTTP.get( { hostname: "example.com", port: 80, method : "get", path : "/" } );
var response2 = sack.HTTPS.get( { ca:&lt;extra cert(s)&gt;, path:"/index.html" } );
```

| HTTP(S) get option | Description |
|----|-----|
| host | address to request from |
| path | resource path to request; "/app/index.html"  |
| port | optional to override the port requested from |
| method | "GET" should specify how to send the request (get/post/...) but for now I think GET is only option |
| rejectUnauthorized | (HTTPS only) whether to accept unvalidated HTTPS certificates; true/false |
| ca | (HTTPS) Additional certificate authorities to validate connection with |

Results with an object with the following fields....

| HTTP Response field | Description |
|----|----|
| content | string content from request |
| statusCode | number indiciating the response code form the server |
| status | text status from server |
| headers | array of header from response (should really be an object, indexes are field names with field values specified) |



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

