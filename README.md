# sack.vfs

[![Join the chat at https://gitter.im/sack-vfs/Lobby](https://badges.gitter.im/sack-vfs/Lobby.svg)](https://gitter.im/sack-vfs/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)[![Build Status](https://travis-ci.org/d3x0r/sack.vfs.svg?branch=master)](https://travis-ci.org/d3x0r/sack.vfs)

Node addon for a custom virtual file system interface.  
JSON/JSON6 (stream)parser, 
COM/serial port access, Sqlite interface, an option/configuration database built on Sqlite.
Windows specific registry access for application settings. 
WebSocket network library.  UDP sockets.
Vulkan API to be added eventually... 

## Requirements
#### npm
        nan (Native Abstractions for Node.js)

#### Centos/linux
        yum install gcc-c++ libuuid-devel
        
#### Windows
	none

# Usage

```
var vfs = require( 'sack.vfs' );
var volume = vfs.Volume( "MountName", "fileName.vfs" );
var file = volume.File( "filename" );
var fileString = file.read();
var fileOut = volume.File( "filename.out" );
fileOut.write( fileString );

```

## Objects
This is the object returned from require( 'sack.vfs' );

```
vfs = {
    ComPort(comport) - access to com ports.
    JSON6 - A json parser. (JSON5 Compatible)
        parse(string) - result with a V8 object created from the json string.  
        begin( cb ) - begin parsing JSON6 stream; callback is called as each value is completed.
    JSON - A json parser.
        parse(string) - result with a V8 object created from the json string.  
        begin( cb ) - begin parsing JSON stream; callback is called as each value is completed.
    mkdir(pathname) - utility function to make directories which might not exist before volume does; 
            (Volume() auto creates directories now if specified path to filename does not exist)
            parameters - (pathname)
                path to create- will created missing parent path parts too.
    Sqlite(dbName) - an interface to sqlite which can open databases in the volume.
            parameters - ( databaseName )
                databaseName maybe a simple ODBC name; if the name ends with .db, it is assumed to be 
                a sqlite database.  If ODBC is not available, it may be assumed that the name will 
                just be a sqlite database name. Extra syntax - if the name starts with a $, then the 
                next word after $ and before the next @ or $ is used to select a sqlite vfs name.
                ( https://sqlite.org/vfs.html )  
                if the name is 'sack' then after @ and before the next $ is specified a mounted filesystem name.
        Sqlite has methods available on it to access native program options.
        Sqlite.op( opName, defaultValue ) - read/write default option database option.
        Sqlite.so( opName, newValue ) - write new value to default option database.
    Volume(mountName,fileName,a,b) - a virtual disk partition that holds files.
          mountName - the mount name used to mount to volume as a filesystem; it may be referenced 
                later in the string passed to Sqlite.  It may be `null` if it is anonymous mount.
          if no parameters are passed, a Volume object representing the native filesystem is returned.
    File - some native filsystem utility methods
    SaltyRNG - creates a random number generator
    TLS - namespace for utilities to generate certificates/keys
        genkey( length [,password]) - Generates a keypair
        pubkey( {options} ) - gets public key of a keypair or certificate
        gencert( { options } ) - Generates a self signed certificate.
        genreq( {options} ) - Generates a certification request
        signcert( {options} ) - uses a certificate to sign a certificate request
        validate( {options} ) - validate a certificate against a certificate chain.
        expiration( certificate ) - gets the expiration of a certificate as a Date().     
    WebSocket - Websocket interface
        Client( ... ) - create a websocket client
        Server( ... ) - create a websocket server        
    Network - Raw network utilities
        Address( address [,port] ) - holder for network addresses.
        UDP( ... ) - UDP Socket to send/received datagrams.
    // windows only
    registry - an interface to windows registry options
    	set( path, value ) - set a new value in the registry
        get( path )  - get a value from the registry.
}
```


### Volume Interface 
 (result from vfs.Volume())
```
Volume = {
    File(filename) - open a file (returns a new object)
            (filename) 
                the file to open in the volume.
    dir() - returns an array of filenames in the volume.
    exists(file/path name) - boolean if filename exists (as a file or a directory)
    read(fileName) - read a file from a volume; return an ArrayBuffer with a toString() that returns the buffer as utf8 as a string.
    readJSON(fileName, callback) - read a file from a volume; calls callback with each object decoded from the file interpreted as JSON (unimplemented)
    readJSON6(fileName, callback) - read a file from a volume; calls callback with each object decoded from the file interpreted as JSON6.
    write(fileName,arrayBuffer/string) - writes a string or arraybuffer to a file. 
    Sqlite(database) - an interface to sqlite database in this volume.
    rm(file),delete(file),unlink(file) - delete a file.
    mkdir - utility function to make directories which might not exist before volume does; (volume auto creates directories now)
            parameters - (pathname)
                path to create- will created missing parent path parts too.
}


```
### File Interface opened within a Volume
 (result from vfs.Volume().File())
```
File instance methods (prototype methods)
File = {
    read(size[, position]) - read from the file; return an ArrayBuffer with a toString method to interpret it as utf8.  Optional parameter
                           position may set the position to read from.
    readLine( [position] ) - reads a line from a text file; optional parameter position may set the position to read from.
    write(arrayBuffer/string) - write data to the file; at the current position
    writeLine( line [, position] ) - output text to a file; a newline will be automatically appended to the line.
    seek(pos[,whence]) - set file position to write.  optional whence parameter can be vfs.File.SeekSet(0), vfs.File.SeekCurrent(1), or vfs.File.SeekEnd(2)
    trunc() - set file size to the curent file position.
    pos() - return the current position in the file.
    ... /*most other methods unimplemented*/
}

File Methods
    rm(file),delete(file),unlink(file) - delete in the native filesystem; file may have to be closed before delete can work.

File Constants
    SeekSet - used in seek methods; value SEEK_SET(0)
    SeekCurrent - used in seek methods; value SEEK_CUR(1)
    SeekEnd - used in seek methods; value SEEK_END(2)

```

## Sqlite Interface
  (result from vfs.Sqlite() or vfs.Volume().Sqlite())

Sqlite() will create a in memory database<br>
Sqlite( &lt;String&gt; ) will user the string to specify a filename.  Sqlite URI decoding is enabled.  `":memory:"` will also result in a memory only database.

There are methods on the Sqlite() function call...

| Sqlite function methods  |  |  |
|---|---|----|
| eo | (callback) | Enumerate Options.  Each option node in global database is passed to callback (see Option Nodes below) |
| go | ( &lt;String&gt; [,&lt;String&gt; [,&lt;String&gt;]] ) | Get Option.  First string is section, second optional string is option name, final string is default value of option.  Option name may be concated to section ( "section/option", "default" ) === ("section", "option", "default" ) |
| so | ( &lt;String&gt; [,&lt;String&gt; [,&lt;String&gt;]] ) | Set Option.  First string is section, second optional string is option name, final string is default value of option.  Option name may be concated to section ( "section/option", "new Value" ) === ("section", "option", "new Value" ) 


#### Sqlite Instance Methods 

```
  var sack = require( 'sack.vfs' );
  var sqlite = sack.Sqlite( "test.db" );
  // the following describes methods on the sqlite instance
```

| Sqlite instance methods | Parameters | Description |
|-----|-----|----|
| escape | (&lt;String&gt;) | returns an encoded string for (string) appropriate for the current database. (may vary encoding based on ODBC driver used; otherwise escapes single quotes for Sqlite) |
| end | () | close this database.  Another command on this database will re-open it.  |
| transaction | ()  |  begin a sql transaction, ```do()``` commands issued after this will be in a transaction. |
| commit | ()  |  end a transaction successfully. |
| autoTransact | (&lt;bool&gt;) | enabled/disable auto transaction features.    A command will begin a transaction, a timer will be set such that if no other command between happens, then a commit will be generated. So merely doing ```do()``` commands are optimized into transactions.
| makeTable | (tableString) | evalute a sql create table clause, and create or update a table based on its definitions.          Additional columns will be added, old columns are untouched, even if the type specified changes.    Additional indexes and constraints can be added to existing tables.
| do | ( &lt;String&gt;) | execute a sql command or query.  Results with null on error, or an array on success.  If command generates no output, array length will be 0.  |
| op  | (section [, opName],defaultValue) |  get an option from sqlite option database; return defaultValue  if not set, and set the option int he database to the default value.
| getOption | (section [,opName],defaultValue) | longer name of 'op' |
| so | (section [,opName] ,value) | set an option in sqlite option database |
| setOption | (section [,opName] ,value) | longer name of 'so' |
| fo  | (opName) | find an option node under this one (returns null if node doesn't exist)<BR> fo( "name" ) |
| go  | (opName) | get an option node      <BR>go( "name" ) |
| eo  | (callback) |  enum option nodes from root of options, takes a callback as a paraemter.<br> callback parameters ( optionNode, optionName ) ... the callback parameters get a node and a name.   The node is another option node that migth be enumerated with eo...<BR> `function callback(node,name)  {console.log( "got", name, node.value );` |
     

example sql command?
```
    var results = sqlite.do("select * from table");  // do a sql command, if command doesn't generate data result will be true instead of an array
```

### Option Database
  (result from vfs.[Volume().Sqlite()/Sqlite()].[fo/go/eo]() or any of these own methods )

| Option Node Instance methods |  |   |
|---|---|----|
|fo | (option name) | find an option node under this one (returns null if node doesn't exist)   fo( "name" )  |
| go | (option name) |  get an option node      go( "name" )
| eo | (callback)   | enum option nodes from root of options  <br> eo( cb )<br> function cb( node, name ) { console.log( "got", name, node.value ); }  |
| value | getter/setter<BR>-none- | set/get current value |


### Sqlite Usage

```
var dbName = "filename.db";
var db = vfs.Sqlite( dbName );
rows = db.do( "select * from sqlite_master" );
rows.forEach( row=&gt;{
    console.log( "row : ", row );
}
```

db.do returns a result set (if any).  It will be an array of objects.  Objects will have fields named that are the names from the sqlite query.  If the name is the same as another, the columns are amalgamated into another array....

```
db data...

table fruits  
fruit_id name
1 apple
2 banana
3 orange

table colors
color_id name
1 yellow
2 orange
3 red
4 green


table fruit_color
fruit_id color_id
1 3
1 4
2 1
3 2

SELECT * from fruits join fruit_color join color 

[ { fruit_id : [1,1], name: [apple, red], color_id, [3, 3] } 
 { fruit_id : [1,1], name: [apple, green], color_id, [4, 4] } 
 { fruit_id : [2,2], name: [banana, yellow], color_id, [1, 1] } 
  { fruit_id : [3,3], name: [orange, orange], color_id, [2, 2] } 
 ]
 ```

```
 console.log( db.do( 'SELECT * from fruits join fruit_color join color' );
```

 Would probably return soemthing like the above. Numbers will be type Number, strings are strings, and well otherwise that's it.
I don't know; maybe it's smart enough if it's the same value it doesn't exapnd it to an array.  That was what one MS SQL driver gave me back when I did a join with a ```*```... the joined column's values were put into an array instead of a simple ID... 
That's why the names from the seaprate tables get put into the same value as an array... If the query were more like

```
SELECT fruits.fruit_id fruit_id,fruits.name fruit,color.name color,color.color_id color_id from fruits join fruit_color join color


[ { fruit_id : 1, fruit: apple,  color:red, color_id:3 } 
 { fruit_id : 1, fruit: apple, color:green, color_id:4 } 
 { fruit_id : 2, fruit: banana, color:yellow, color_id:1 } 
  { fruit_id : 3, fruit: orange, color:orange, color_id:2 } 
]

```
 
which makes a lot more sense.  and simple queries on well crafted schemas should be pretty straight forward.

## Options


### getting options ... ```go()```
``` javascript
var vfs = require( 'sack.vfs' )
var db = vfs.Sqlite( 'option.db' );
var root = db.go( "test" );
var tmp;
var tmp2;
var path = ( tmp = root.go( "a" ).go( "b" ).go( "c" ) );

console.log( "option value is : ", tmp.value, tmp, Object.keys(tmp), Object.getPrototypeOf( tmp ) );
```
root is now an option root.  go ~ getOption.... getOption also exists... but the shorthand is ... short.
getOption may be depricated.

tmp is a option 'a/b/c' in the root.  

``` javascript
tmp.value = "Default Other Value";
```
give tmp option a value.


This is diagnostic output in optionTest.js source... it outputs values and tests defaults and when the program restarts it may have new values in some of the keys; tests persistence of the keys (aka can it write the sqlite database)
```
console.log( "Initial.. no defaults...", ( tmp2 = root.go( "a" ).go( "b3" ).go( "c2" ) ).value );
console.log( "option value is : ", tmp.value, tmp, Object.keys(tmp) );

tmp2.value = "Set Value";
console.log( "after setting value..",  tmp2.value );
console.log( "after setting value..", ( tmp2 = root.go( "a" ).go( "b3" ).go( "c2" ) ).value );
//setTimeout( ()=&gt;{console.log( "Wait a second to flush?") }, 1000 );
```

### enumerating options  ```eo()```

db.eo( callback );  Callback is passed option
``` javascript

var level =0;
db.eo( dumptree )
function dumptree(opt, name){
        if( name === "." ) return;
        var leader = "";
        for( var n = 0; n < level;n++ ) leader += "   ";
        console.log( leader + "option:", name );
        level++;
        opt.eo( dumptree );
        level--;
}
```


This dumps the option tables internally option4_map, option4_name, option4_value I think are the tables it uses... there is option4_blob which should store ArrayBuffer types... (provide concat operations/block update internal for retry recovery?)
```
function tick() {
        console.log( "dump tables: \n"
                , db.do( "select * from option4_map" ) 
                );
}
tick();
```


## JSON( [JSON6](https://www.github.com/d3x0r/json6) ) - JSON and JSON6 compatible processor 

Slightly extended json parser to allows simple types to be returned as values, not requiring { or [ to start
the JSON input.  JSON6 parsing is 100% compatible with JSON parsing; that is JSON6 can read JSON content with no issues.

Simple values, objects and arrays can result from parsing.  Simple values are true,false,null, Numbers and Strings.

Added support 'reviver' parameter.


  - JSON
     - parse( string [,reviver] )
     - begin( callback )
         - write( data )
  - JSON6
     - parse( string [,reviver] )
     - begin( callback )
         - write( data )

Reviver callback parameter is not provided for streaming callback.

``` javascript
var vfs = require( "sack.vfs" );

var object = vfs.JSON.parse(string [, reviver]);

var object2 = vfs.JSON6.parse(string [, reviver]);

```

### Streaming JSON Parsing

  - begin(cb)  returns a parser object, pass a callback which receives objects as they are recognized in the stream.
    - parser.write( data )  Write data to the parser to parse into resulting objects.

Simple values, objects and arrays can result from parsing.
because a streaming input is expected, cannot pass just simple objects to be parsed, because an input of "1234" might
be just part of a number, and more of the number might be added in the next write.  If there is a trailing newline or other 
whitespace character after the number, the number will be sent to the callback registered in the begin.

``` javascript
var vfs = require( "sack.vfs" );

var parser = vfs.JSON6.begin( objectCallback );
parser.write( "123 " );
function objectCallback( o ) {
    console.write( "Received parsed object:", typeof o, o );
}

// another example.
var streamExample = vfs.JSON6.begin( (o)=> { console.log( "received value:", o ) };
streamExample.write( "123\n" );   // generate a single event with number '123'
streamExample.write( '"A string"' );    // generate another event with the string
streamExample.write( '{a : 1} { b : 2}{c:3}' );   // generates 3 callback events, 1 for each object

```


## TLS Interface

Exposes OpenSSL library functions to create key pairs, cerficates, and certificate chains.

### Interface

```
  TLS={
     genkey( length [,password]) - Generates a keypair, returns PEM string.  Requires length parameter, allows optional password to be specified.
     pubkey( {options} ) - gets public key of a keypair or certificate
     gencert( { options } ) - Generates a self signed certificate.
     genreq( {options} ) - Generates a certification request
     signcert( {options} ) - uses a certificate to sign a certificate request
     validate( {options} ) - validate a certificate against a certificate chain.
     expiration( certificate ) - gets the expiration of a certificate as a Date().     
  }
```

 | pubkey options | |
|---|---|
| cert | a certificate to get a key from, will not be used if 'key' option is specified.  either this or key MUST be specified, but not both.|
| key | a keypair to get the public key from.  Either this or cert MUST be specified, but not both.|
| password | password for keypair if required.|

 | gencert options | |
|---|---|
| country | specified country of origin; must be a 2 character country specifier. |
| state | specifies state or province of origin; |
| locality | specifies city in state specified.    |
| org | specified organization for certificate       |
| orgUnit | specifies organizational unit.             |
| name | specifies common name, this is typically a domain name with possible wildcard at start. |
| key | private/public key to use                                                                  |
| password | if required for key                                                                     |
| serial | serial number for this certificate (is a number type).                                      |
| expire | number of days before this certificate expires (is a number type).                            |
| pubkey | override storing the public key of this certificate. (used to test certificate chain validity check; do not use) |
| issuer | override the issuer/subject identifier of this certificate. (used to test certificate chain validity check; do not use) |

| genreq options |  |
|---|---|
| country | specified country of origin; must be a 2 character country specifier. |
| state | specifies state or province of origin;   |
| locality | specifies city in state specified.  |
| org | specified organization for certificate |
| orgUnit | specifies organizational unit. |
| name | specifies common name, this is typically a domain name with possible wildcard at start. |
| subject | add fields to Subject Alternative Name...  This is an object containing arrays |
| | <TABLE><TR><TD> DNS </TD><TD> a single domain name or an array of domain name strings to add </TD></TR> <TR><TD> IP </TD><TD> a single IP address string or an array of IO addresses to add; does support IPv6 addresses.</TD></TR></TABLE>  |
| key | private/public key to use  |
| password | if required for key |


```
// some examples of subject options
TLS.genreq( { subject : { IP: "127.0.0.1" } } );
TLS.genreq( { subject : { IP: ["127.0.0.1","192.168.1.1"] } } );
TLS.genreq( { subject : { DNS: "www.example.com" } } );
TLS.genreq( { subject : { DNS: ["www.example.com", "smtp.example.com"], IP:[ "10.0.0.1" ] } } );
```



| signcert options |  |
|---|---|
| signer | certificate to sign with |
| request | certificate request to sign |
| expire | number of days this certificate is valid for |
| key | private key of the signer |
| password | password of the signer's key. |
| serial | serial number for this certificate (is a number type). |
| issuer | used to override issuer identifier. (used to test certificate chain validity check; do not use) |
| subject | used to override subject identifier. (used to test certificate chain validity check; do not use) |
| pubkey | used to override the public key stored in the output certificate. (used to test certificate chain validity check; do not use) |

| validate options |  |
|---|---|
| cert | certificate to validate | 
| chain | concatenated certificate chain to use for validation.  Order does not matter.  Not required if cert is self signed. |

See Also [tlsTest.js](https://github.com/d3x0r/sack.vfs/blob/master/tests/tlsTest.js) for example usage.

## SRG Module

Salty Random Generator 

```
  var sack = require( 'sack.vfs' );
  var SRG = sack.SaltyRG( salt=&gt;salt.push( new Date() ) )// callback is passed an array to add salt to.
  var number = SRG.getBits(); // get a signed 32 bit number -  -2,147,483,648  to 2,147,483,647
```

  - SaltyRG( callback ), SaltyRG( initial_seed ), SaltyRG()<BR>
     callback is called to request more salt.  An array is passed which can have values pushed into it.<BR>
     initial_seed is a seed to use initially, then no callback is called.<BR>
     no parameters results with a generator with no initial seed, and no callback<BR>
     Results with a random_generator object.
     
     
    - seed( toString ) : set the next seed to be used; any value passed is converted to a string.
    - reset() : reset the generator to base state.
    - getBits( [bitCount [, signed]] ) : if no parameters are passed, a 32 bit signed integer is returned.  If bitCount 
       is passed, it must be less than 32 bits.  If signed is not passed or is false, the resulting value is the specified
       number of bits as an unsigned integer, otherwise the top bit is sign extended, and a signed integer results.
    - getBuffer( bits ) : returns an ArrayBuffer that is the specified number of bits long, rounded up to the next complete
       byte count.
    



```
  // some examples
  var vfs = require( "sack.vfs" );
  var SRG = vfs.SaltyRG( salt=&gt;salt.push( new Date() ) )// callback is passed an array to add salt to.
  
  var signed_byte = SRG.getBits( 8, true );
  var unsigned_byte = SRG.getBits( 8 );
  var someValue = SRG.getBits( 13, true );  // get 13 bits of randomness, sign extended.
  var buffer = SRG.getBuffer( 1234 ); // get 155 bytes of random data (last byte only has 2 bits )
```

## WebSocket Module

```
var sack = require( "sack.vfs" );
var client = new sack.WebSocket.Client( &lt;url&gt; [, &lt;protocol(s)&gt;] [,option object] );
client.on( &lt;event&gt;, &lt;callback&gt; );
client.onOpen( callback );
client.onMessage( callback );
client.onError( callback );
client.onClose( callback );

var server = new sack.WebSocket.Server( { &lt;options&gt; } );
server.on( &lt;event&gt;, &lt;callback&gt; );
server.onConnect( callback );
server.onAccept( callback );

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
   | close() | call this to end a connection |
   | send(message) | call this to send data to a connection, argument should either be a String or an ArrayBuffer.
   | onOpen | sets the callback to be called when the connection opens. |
   | onMessage | set the callback to be called when a message is received, it gets a single parameter, the message recieved. The data is either a string or an ArrayBuffer type | 
   | onClose | sets the callback to be called when the connection is closed from the other side first |
   | onError | sets the callback to be called on an error (no supported errors at this time) |
   | on  | sets event callbacks by name.  First parameter is the event name, the second is the callback |


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


Server Options

  When a server is created it accepts an option object with the following options specified.

  | Server Options |   |
  |---|---|
  | address | specific address to bind to; default binds to ::0 |
  | port |  a port number.  8080 is the default. |
  | masking | true/false; browsers do not accept server masked data; RFC non-specific; default is false |
  | perMessageDeflate | true/false; default false (performance).  Accepts permessage-deflate extension from client, and deflates messages if requested by client. |
  | perMessageDeflateAllow | true/false; default false (performance).  Accepts permessage-deflate from client, but does not deflate messages on response |    
  | cert | &lt;string&gt;; uses PEM certificate as server certificate chain to send to client. |
  | key  | &lt;string&gt;; uses PEM private key specified for encryption; used by clients to authenticate cerficates  |
  | passphrase | &lt;string&gt;; uses passphrase for key provided |

Server Client Methods
  this is a slightly different object than a client, although accepts the same events except for on( "open" ) and onOpen() method.
  
  | Method |  Description |
  |----|----|
  | send | send data on the connection.  Message parameter can either be an ArrayBuffer or a String. (to be implemented; typedarraybuffer) |
  | close | closes the connection |

Http Request fields

  | Name  | Description |
  |----|----|
  | url | the URL requested |
  | connection | same as a Websocket Connection object |
  | headers | headers from the http request |

Http Response methods
   These methods are available on the 'res' object received in the Server "request" event.

  | Method | Parameters | Description |
  |----|----|-----|
  | writeHead | (resultCode [,extraHeadersObject]) | setup the return code of the socket.  Second parameter is an object which is used to specify additional headers. |
  | end | ( content [,unused]) | sends specified content.  String, Buffer, uint8Array, ArrayBuffer area all accpeted.  (sack.vfs.File?) |


WebSocket connection Object

  | Connection Field | Usage |
  |----------|----------|
  | localFamily | &lt;String&gt; value is either 'IPv4' or 'IPv6' |
  | localAddress | &lt;string&gt; the IP address of the local side of the connection |
  | localPort | &lt;Number&gt; port number of the local connection |
  | remoteFamily | &lt;String&gt; value is either 'IPv4' or 'IPv6' |
  | remoteAddress | &lt;string&gt; the IP address of the remote side of the connection |
  | remotePort | &lt;Number&gt; port number of the remote connection |
  | headers | &lt;Object&gt; field names are the names of the fields in the request header; values are the values of the fields.<BR> Client side connections may not have headers present.  |

## Network Address Object (Network.Address)
```
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



## UDP Socket Object (Network.UDP)

```
var sack = require( 'sack.vfs' );
var udp  = sack.Network.UDP( "localhost:5555", (msg,rinfo)=>{ console.log( "got message:", msg ) } );
var udp2 = sack.Network.UDP( {port:5556, address:"localhost", toAddress:"localhost:5555" }, (msg,rinfo)=>{ console.log( "got message:", msg ) } );
var udp3 = sack.Network.UDP( {port:5557, address:"localhost", toAddress:"localhost", toPort:5555, message:(msg,rinfo)=>{ console.log( "got message:", msg ) } );

udp2.send( "Hello World" );

```

#### sack.Network.UDP() Invokation

| Construction examples |  |
|-----|----|
| ()  | not valid, requires either a string, or option object. |
| ( "localhost:1234" ) | creates a UDP socket bound to localhost port 1234 |
| ( { address: "localhost",<BR>port:1234} ) | creates a UDP socket bound to localhost port 1234 |
| ( { address: "localhost",<BR>port:1234}, (msg,rinfo)=>{} ) | creates a UDP socket bound to localhost port 1234, and specifies a message handler callback |

  can pass a string address to listen on.  "IP[:port]" format.  IPv6 IP should be enclosed in brackets;  `[::1]:80`.
  if address is not passed, an object with socket options must be specified.
  final parameter can optionally be a callback which is used as the on('message') event callback.

#### Udp socket creation options

| UDP Socket options |   |
|----|----|
| port  | &lt;number&gt; specify the port to listen on |
| address | &lt;string&gt;; specify the address to listen on (defaults to [::]). Optional port notation can be used (specified with a colon followed by a number (or name if under linux?))<BR>If the port is specified here, it overrides the `port` option.|
| family | &lt;string&gt; either 'IPv4' or 'IPv6' which controls the default address; otherwise address string will determine the family |
| toPort | &lt;number&gt; specify the port to send to if not specified in send call |
| toAddress | &lt;string&gt; specify the address to send to if not specified in send call.  Optional port notation can be used (specified with a colon followed by a number (or name if under linux?)) <BR>If the port is specified here, it overrides the `toPort` option.|
| broadcast | &lt;bool&gt; if `true` enable receiving/sending broadcast UDP messages |
| readStrings | &lt;bool&gt; if `true` messages passed to message callback will be given as text format, otherwise will be a TypedArray |
| reuseAddress | &lt;bool&gt; if `true` set reuse address socket option |
| reusePort | &lt;bool&gt; if `true` set reuse port socket option (linux option, not applicable for windows) |

#### Udp socket methods

| UDP Socket Methods | Arguments | Description  |
|-----|-----|-----|
| send | (message [,address]) | Send a message, message can be an ArrayBuffer or string,   if second parameter is passed it should be an sack.Network.Address object. |
| close | () | Close the socket. |
| on | (eventName, callback) | Set message or close callbacks on the socket. |
| setBroadcast | (bool) | enable broadcast messages |

#### Udp Events

| UDP Events |  |  |
|----|----|----|
| message | (msg, remoteAddress) | called when a message is received.  msg parameter is either a string if socket was opened with `readStrings` or a TypedArray.  Second parameter is a Network.Socket object representing the source address of the message |
| close | () | Socket has been closed. | 

See Also [testudp.js](https://github.com/d3x0r/sack.vfs/blob/master/tests/testudp.js) for example usage.


## RegAccess - windows registry system access....

Parses a simple path and assumes last name of path is key to read/write a value to.
HKCU
HKLM
and capital longhand forms of above are supported.

``` javascript
var vfs = require( "./vfs_module.js" )

function update( newVal ) {
        var oldval = vfs.registry.get( "HKCU/Software/Freedom Collective/ChangeMe" );
        console.log( "Old Value is:", oldval );
        if( oldval != newVal )
                 vfs.registry.set( "HKCU/Software/Freedom Collective/ChangeMe", newVal );
}
update( Date.now() );
```

The above script reads the value, reports what it was, if the old value is not the new value, then write the new value.
example was testing a constant like 526.  if typeof value is a number, value is set as a REG_DWORD.  Otherwise it's set as REG_SZ.


registry = {
     set( regPath, value );
     get( regPath );
}

```
var reg = vfs.registry.set( "HKCU/something", value );
var val = vfs.registry.get( "HKCU/something" );
```

### COM Ports
   (result from vfs.ComPort() )
 
```
ComObject = { 
     onRead( callback ) - sets a callback to be called with a uint8Array parameter when data arrives on the port.
     write( uint8Array ) - write buffer specfied to com port; only accepts uint8array.
     close() - close the com port.
}

COM port settings are kept in the default option database under 
  /comports.ini/COM PORTS/&lt;comName&gt; = 57600,N,8,1,carrier,RTS,rTSflow
  /comports.ini/&lt;comName&gt;/port timeout = 100 

     the com port settings string is semi-standard DOS string... the first paramaeters, 
         baud, N/E/O/M/S (parity), 8/7 (data bits), 1/2 (stop bits); 
         then the next settings are toggles based on case...
            [C/c]arrier - enable/disable respect carrier detect to send (lower case is disable)
            [R/r]TS - enable/disable respect request to send (lower case is disable)
            [R/r]TSflow - enable/disable using RTS for flow control. (lower case is disable)

    port timeout is how long to wait for idle between sending com data.  
        (no data received for 100ms, post packet; this gives a slight delay between when the 
         data is received and actually dispatched)
```
---

# GUI Interface objects

Building GUI extensions use 'npm run build-gui' or 'npm run build-gui-debug'  Which will download SACK from Github and build against the full library instead of a partial amalgamation.

```
var sack = require( 'sack.vfs' );
```

| Interface | Description |
|-----|------|
| Image | Handles loading, creating, and simple image manipulation methods. |
| Image.Color | namespace to create color objects to interact with display.  Internally, depending on the driver, format changes from ARGB to ABRG potentially. |
| Render | A way to open a window which can display images |
| Intershell | Interactive shell, higher level application thing that allows dynamic page layout configurations |
| Frame | Entry point for GUI controls |
| Registration | Namespace for defining new control types which can be created on Frames. |

## Image Methods

```
var imageFromFile = sack.Image( "image file.jpg" ); // handles bmp, gif, png, jpeg formats.
var imageFromBuffer = sack.Image( UInt8Array or TypedArray or ArrayBuffer ); // attempts to decompress image data contained in passed buffer.
var imageRaw = sack.Image( 100, 100 ); // creates a new image of specified width, height
var imageSubImage = sack.Image( 100, 100, otherImage, 10, 10 ); // creates new image of width, height starting at x, y in otherImage.

```

Image constructor can be passed either a string which is treated as a filename and the specified image is loaded.  It can be passed a binary buffer
which is treated as the contents of a file and is attempted to be decoded.  Specifying a width and height alone will create a new blank image.
A sub image can be created in another existing image by specifying the image and x, y position to start at.  This last operation can be done 
by using the Image method on an existing image also.


| Image Methods | arguments | description |
|-----|-----|----|
| Image | (x, y, width, height ) | Creates a image within the current image.  Operations on this image are clipped within the region specified |
| reset | () | Clear an image to transparent black (0,0,0,0) |
| fill | ( x, y, width, height, color ) | fill a region of an image with a simple color.  |
| fillOver | ( x, y, width, height, color ) | fill a region of an image with a simple color; if color is semi-transparent applies color over existing image data.  |
| line | ( x1, y1, x2, y2, color ) | Draw a straight line in specified color.|
| lineOver | ( x1, y1, x2, y2, color ) | Draw a straight line in specified color.  If color is semi-transparent, applies color alpha over existing image data.|
| plot | ( x, y, color ) | set a pixel on the output image to specified color. |
| plotOver | (x, y, color ) | set a pixel on the output image to specified color.   If color is semi-transparent, applies color alpha over existing image data.|
| drawImage | ( image [, x, y [, xAt, yAt [,width, height]]] ) or ( image [, x, y, width, height, xAt, yAt , source_width, source_height ) | Sets the specified image into the 'this' image.  Draw at x, y (0,0 if not specified), starting at position (xAt, yAt) in the source IMage (0,0 if not specified), for a width and height specified (otherwise full image width, height.  If all parameters are specified, scales the portion of the source image to the target image.  )
| drawImageOver | (same as drawImage) | Same as drawImage, except applies alpha in source image to overlay the new image over the existin content in image. |
| imageSurface | () | Gets a uint8arry which is the underlaying image buffer.  PIxel format is typically [r,g,b,a].  If a lot of custom pixels are to be updated, this is far more efficient to update than using plot or plotOver methods.| 
| png | read-only accessor | Gets the image data as an ArrayBuffer, converted using PNG compression.  The quality of compression can be controlled using the jpgQuality accessor. |
| jpg | read-only accessor | Gets the image data as an ArrayBuffer, converted using JPEG compression.  The quality of compression can be controlled using the jpgQuality accessor. |
| jpegQuality | integer between 0-100, default is 78 | This controls the quality factor of jpeg compression |
| width | readonly accessor | Gets image width |
| height | readonly accessor | Gets image height |

## Color methods

```
var colorFromInteger = sack.Image.Color( 0xFF995522 ); //creates a color from specifed integer 0xAARRGGBB
var colorFromObject = sack.Image.Color( { a: 255, r:128, g: 64, b: 100 } ); // creates color from specified parts.  Clamped to 0-255.
var colorFromObjectFloat = sack.Image.Color( { a: 0.9, r:0.5, g: 0.25, b: 0.37 } ); // creates color from specified parts.  Clamped to 0-255.  The value 1, unfortunatly is treated as integer 1 for the above 0-255 value.
var colorFromString = sack.Image.Color( "Some string" ); // Unimplemented.
```

There are some constant colors already builtin in Image.colors.[color name]

| Color accessors | description |
|-----|-----|
| r | get/set red channel |
| g | get/set green channel |
| b | get/set blue channel |
| a | get/set alpha channel |

| Color Methods | arguments | description |
|---|---|---|
|  toString| () | gets color in a string format |

##Render Methods

Render constructor takes several optional parameters. 

( title [, x, y [, w, h [, parent Renderer]]] ) 

TItle is a text string set as the window's caption/title string.  X and Y control where the window is opened (-1, -1 uses a system default).
W and H control the width and height of the window (-1, -1 uses a system default).  parent renderer is a renderer that this one is opened over.
This should guarantee stacking order, and is semi-modal, that events will not be sent to the parent renderer while the child is open.

| Renderer Methods | Arguments | description |
|----|----|----|
| getImage | () | returns a sack.Image which is the surface of this renderer |
| setDraw | ( callback ) | callback function is passed a sack.Image which can be drawn on.  Once this function returns, the content is updated to the display. |
| setMouse | (callback) | callback function is passed an event object containing { x, y, b } which is the mouse event x, y position within the surface of this renderer, and the button events.  Button values are defined in  sack.button.[left/right/middle/scroll_down/scroll_up]. |
| setKey | (callback) | callback function is passed an encoded integer value of the key.  (needs more work to improve interfacing) |
| show | () | show the renderer.  This must be called before the window will be displayed. |
| hide | () | hide the renderer.  Does not close the renderer, but stops showing it on the display. |
| reveal | () | reveal the renderer.   Meant to be used after hide() to restore it to the screen.  (might also be show()? ). |
| redraw | () | trigger an event to the renderer which will invoke the draw callback specified. |
| update | () or (x, y, width, height ) | Specify the position of the display which is meant to be updated.  Can be used within the draw callback function for partial updates.  If no arguments are specified, full display is updated.|
| close | () | Close the renderer.  Releases all resources for the renderer.  Ends event loop for display, so application can exit. |
| on | ( event Name, callback ) | Set events by name, "draw", "mouse", "key" setting the specified event to the function specified. |

## Frame Methods

These are higher level interface that creates a frame with a caption using Renderer and Image methods underneath.  First, create a frame...

```
var frame = sack.Frame( "Title", x, y, width, height );
```
Frame constructor requires a title.  If x, y are not specified, (0, 0) is used, and (1024,768) is used for width and height.

| Frame Methods | arguments | description |
|----|----|----|
| Frame | ( title, x, y, width, height ) | create a new frame over the existing frame.  This is a modal operation... that the existing frame will not get events until this frame is closed. |
| Control | ( controlType, x, y, width, height ) | Create a control within the frame, the controlType is a string specifying the type of the control to be created.  Control types are available in an array as `sack.control.types`. |
| show | () | show the Frame.  Frame is hidden allowing creating controls efficiently before showing the completed dialog. |
| hide | () | hide the frame.  Does not close the frame, just removes it from being displayed. |
| reveal | () | restore a frame.  (may be same as show()?) |
| close | () | close a frame. |

| Control Methods | arguments | description |
|----|----|----|
| Control | (controlTypes, x, y, width, height ) | creates a control within this current control. |
| createControl | (controlTypes, x, y, width, height ) | creates a control within this current control. |
| createFrame | creates a new frame over the current control/frame.  Showing this will be modal. |
| show | () | show a control |
| hide | () | make a control hidden. |
| reveal | () | reveal a hidden control. |
| redraw | () | trigger an update to a control. |
| close | () | (missing) |
|  |  |  |
| (other) | ... | Depending on the control created, various addtional methods may be added.  (A button will get a way to set click event, listbox will have methods to add list items, etc) |
| size | accessor | gets an object {width:#, height:#} which has the current width and height of the control.  Passing a similar object will set the width and height of a control. |
| position | accessor | gets an object {x:#, y:#} which has the current x and y position of the control.  Passing a similar object will set the position of a control. |
| layout | accessor |  gets an object {x:#, y:#, width:#, height:#} which has the current x and y position of the control and also size of the control.  Passing a similar object will set the position of a control and also size of the control. |
| text | accessor | sets/gets the caption/text string of a control.  All controls have this, but not all controls show this. |

### Control Registration

New controls can be created by specfiying a string name for the control type, and setting up event callback functions.

```
var newControl = sack.Registration( { name : "custom Control", draw(Image) { /* handle draw */ }, mouse( event ) { /* handle mouse event } } );
```

There are a couple dozen events actually available, but only a handful have been implemented.   These are all specified in the option
object passed to the control Registration constructor.

| Registration Object Fields | description |
|----|----|
| name | string used to create a control of this type |
| width | default width of the control when created through frame editor (not yet exposed) |
| height | default height of the control when created through frame editor (not yet exposed) |
| default_border | the default border attributes of the control.  Border attribute values are in `sack.control.border` |
| create | a function called when the control is created.  Return true/false to allow the control to be created or not. |
| draw | a function called when a control needs to draw.   sack.Image type is passed to the function. |
| mouse | a function called when a mouse event happens.  A mouse event object with {x, y, b} is passed.  coordinates are relative to the surface of the control (within the border of the control) |
| key | a function called when a key event happens.  A encoded key event is passed.  (needs better interfacing) |
| destroy | A function called when the control has been destroyed/closed |
| | |
| Others | as required these will be implemented... focus, touch, property page handlers, moved, sized, load, save....  |

## Intershell Interface

Mostly unimplemented, more of a place holder than functional.


---

## Changelog
- 0.1.99316 Fix rekey.
- 0.1.99315 Add rekey method to volumes.
- 0.1.99314 Fixed duplicating address sent with UDP messages.  
Fix reading files from /sys/class/*.  Fix reading directory.  
Fix websocket protocol option before options object.  
Fix websocket connection sequence.  
Allocate new connection object during accept so protocol lookup can attach data to that object. 
Fix handling SSL certificates on websocket connections.  
Simplify sending/receiving UDP subnet broadcasts by simply enabling broadcast. Other small fixes.
- 0.1.99313 remove some debug logging.
- 0.1.99312 fixes building on mac a little.
- 0.1.99311 Fixes building on msvs (exception flag)
- 0.1.99310 Fix rebinding on linux to subnet broadcast address.
- 0.1.99309 Fix clearing some persistant handles so objects can be garbage collected. 
Fix SSL server issues.  Implement onrequest handling for websocket connections.  Improve websocket close reason handling.
Fix TLS certification validation.  Fix passing additional root certificate for validation.
- 0.1.99308 Improve network event handling; Allow more connections dynamically; fix some locking issues with critical sections.
- 0.1.99307 Implement interface to websocket library. Implement interface to UDP sockets.
- 0.1.99306 Move json reviver parameter handling internal.  Implement volume JSON stream reader interface.  Fix options to create/reopen an existing file.
- 0.1.99305 Fix handling exceptions triggered from callbacks. Fix missing truncate in more instances;  Sync sack filesystem updates; fix unlink return value; add pos() method to File.
- 0.1.99304 fix truncate on simple writes into a volume.
- 0.1.99302, 0.1.99303 improvements for sqlite interface.
- 0.1.99301 add SaltyRandomGenerator interface.
- 0.1.99300 fix random generator overflows.
- 0.1.99299 set default sql auto checkpoint to off.
- 0.1.99298 add preferGlobal to package.json.
- 0.1.99297 replace Persistent strings with Eternal, which work on nodev4.
- 0.1.99296 fix deprecated v8::Delete method to use Maybe version.
- 0.1.99295 fix build error on older versions of node.
- 0.1.99294 fix stream paring empty array elements; and a error stream parsing a break between a field label just at the colon.
