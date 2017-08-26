# sack.vfs

[![Join the chat at https://gitter.im/sack-vfs/Lobby](https://badges.gitter.im/sack-vfs/Lobby.svg)](https://gitter.im/sack-vfs/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)[![Build Status](https://travis-ci.org/d3x0r/sack.vfs.svg?branch=master)](https://travis-ci.org/d3x0r/sack.vfs)

Node addon for a custom virtual file system interface.  Also exposes a JSON6 parser, COM/serial port access, Sqlite interface, an option/configuration database built on Sqlite.
Windows specific registry access for application settings. Vulkan API to be added eventually... 

## Requirements
### npm
        none

### Centos  
        yum install gcc-c++ libuuid-devel


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
        1/2 the speed of Node's parser; no real reason to use this. 
        (3x faster than javascript JSON5 implementation)
        // var o = vfs.JSON6.parse(s)
    JSON - A json parser.
        parse(string) - result with a V8 object created from the json string.  
        1/2 the speed of Node's parser; no real reason to use this. 
        (3x faster than javascript JSON5 implementation)
        // var o = vfs.JSON.parse(s)
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
    mkdir - utility function to make directories which might not exist before volume does; (volume auto creates directories now)
            parameters - (pathname)
                path to create- will created missing parent path parts too.
}
```
### File Interface opened within a Volume
 (result from vfs.Volume().File())
```
File = {
    read(size[, position]) - read from the file; return an ArrayBuffer with a toString method to interpret it as utf8.  Optional parameter
                           position may set the position to read from.
    readLine( [position] ) - reads a line from a text file; optional parameter position may set the position to read from.
    write(arrayBuffer/string) - write data to the file; at the current position
    writeLine( line [, position] ) - output text to a file; a newline will be automatically appended to the line.
    seek(pos[,whence]) - set file position to write.  optional whence parameter can be vfs.File.seekSet(0), vfs.File.seekCurrent(1), or vfs.File.seekEnd(2)
    trunc() - set file size to the curent file position.
    pos() - return the current position in the file.
    ... /*most other methods unimplemented*/
}
```
### Sqlite Interface
  (result from vfs.Sqlite() or vfs.Volume().Sqlite())
```
Sqlite = {
    escape(s) - returns an encoded string for (s) appropriate for the current database.  
        (may vary encoding based on ODBC drive used; otherwise escapes single quotes for Sqlite)
    end() - close this database.  Another command on this database will re-open it.
    transaction() - begin a sql transaction, ```do()``` commands issued after this will be in a transaction.
    commit() - end a transaction successfully.
    autoTransact(true/false) - enabled/disable auto transaction features.  
         A command will begin a transaction, a timer will be set such that 
         if no other command between happens, then a commit will be generated.  
         So merely doing ```do()``` commands are optimized into transactions.
    do(command) - execute a sql command
    op(opName,defaultValue) - get an option from sqlite option database; return defaultValue 
         if not set, and set the option int he database to the default value.
    getOption(opName,defaultValue) - longer name of 'op'
    so(opName,value) - set an option in sqlite option database
    setOption(opName, value) - longer name of 'so'
    makeTable(tableString) - evalute a sql create table clause, and create or update a table based on its definitions.
             Additional columns will be added, old columns are untouched, even if the type specified changes.
             Addtional indexes and constraints can be added...
             
    fo(opName) - find an option node under this one (returns null if node doesn't exist)   fo( "name" )
    go(opName) - get an option node      go( "name" )
    eo(callback) - enum option nodes from root of options, takes a callback as a paraemter.
              eo( callback ) ... the callback parameters get a node and a name.  
                    The node is another option node that migth be enumerated with eo...
                    function callback(node,name)  {console.log( "got", name, node.value );
     
    
}
```

example sql command?
```
    var results = sqlite.do(sql);  // do a sql command, if command doesn't generate data result will be true instead of an array

```
### Option Database
  (result from vfs.[Volume().Sqlite()/Sqlite()].[fo/go/eo]() or any of these own methods )
```
OptionNode = {
    fo - find an option node under this one (returns null if node doesn't exist)   fo( "name" )
    go - get an option node      go( "name" )
    eo - enum option nodes from root of options  
                    eo( cb )
                    function cb( node, name ) { console.log( "got", name, node.value ); } 
                    
    value - set/get current value
}
```
### Registry

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
  /comports.ini/COM PORTS/<comName> = 57600,N,8,1,carrier,RTS,rTSflow
  /comports.ini/<comName>/port timeout = 100 

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



## Sqlite Usage


```
var dbName = "filename.db";
var db = vfs.Sqlite( dbName );
rows = db.do( "select * from sqlite_master" );
rows.forEach( row=>{
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
//setTimeout( ()=>{console.log( "Wait a second to flush?") }, 1000 );
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


### JSON( [JSON6](https://www.github.com/d3x0r/json6) ) - JSON and JSON6 compatible processor 

Slightly extended json parser to allows simple types to be returned as values, not requiring { or [ to start
the JSON input.

Simple values, objects and arrays can result from parsing.  Simple values are true,false,null, Numbers and Strings.

Added support 'reviver' parameter.


  - JSON
     - parse( string [,reviver] )
  - JSON6
     - parse( string [,revivier] )
     - begin( callback )
         - write( data )


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
 - pubkey options
    - cert : a certificate to get a key from, will not be used if 'key' option is specified.  either this or key MUST be specified, but not both.
    - key : a keypair to get the public key from.  Either this or cert MUST be specified, but not both.
    - password : password for keypair if required.

 - gencert options
    - country : specified country of origin; must be a 2 character country specifier.
    - state : specifies state or province of origin;
    - locality : specifies city in state specified.
    - org : specified organization for certificate
    - orgUnit : specifies organizational unit.
    - name : specifies common name, this is typically a domain name with possible wildcard at start.
    - key : private/public key to use
    - password : if required for key
    - serial : serial number for this certificate (is a number type).
    - expire : number of days before this certificate expires (is a number type).
    - pubkey : override storing the public key of this certificate. (used to test certificate chain validity check; do not use)
    - issuer : override the issuer/subject identifier of this certificate. (used to test certificate chain validity check; do not use)

  - genreq options
    - country : specified country of origin; must be a 2 character country specifier.
    - state : specifies state or province of origin;
    - locality : specifies city in state specified.
    - org : specified organization for certificate
    - orgUnit : specifies organizational unit.
    - name : specifies common name, this is typically a domain name with possible wildcard at start.
    - subject : add fields to Subject Alternative Name...  This is an object containing arrays
       - DNS : a single domain name or an array of domain name strings to add
       - IP : a single IP address string or an array of IO addresses to add; does support IPv6 addresses.
    - key : private/public key to use
    - password : if required for key

 - signcert options
    - signer : certificate to sign with
    - request : certificate request to sign
    - expire : number of days this certificate is valid for
    - key : private key of the signer
    - password : password of the signer's key.
    - serial : serial number for this certificate (is a number type).
    - issuer : used to override issuer identifier. (used to test certificate chain validity check; do not use)
    - subject : used to override subject identifier. (used to test certificate chain validity check; do not use)
    - pubkey : used to override the public key stored in the output certificate. (used to test certificate chain validity check; do not use)

 - validate options
    - cert : certificate to validate
    - chain : concatenated certificate chain to use for validation.  Order does not matter.  Not required if cert is self signed.

See [tlsTest.js](https://github.com/d3x0r/sack.vfs/blob/master/tlsTest.js) for example usage.

## SRG Module

Salty Random Generator 

```
  var SRG = SaltyRG( salt=>salt.push( new Date() ) )// callback is passed an array to add salt to.
  var number = SRG.getBits(); // get a signed 32 bit number -  -2,147,483,648  to 2,147,483,647
```

  - SaltyRG( callback ), SaltyRG( initial_seed ), SaltyRG()
     callback is called to request more salt.  An array is passed which can have values pushed into it.
     initial_seed is a seed to use initially, then no callback is called.
     no parameters results with a generator with no initial seed, and no callback
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
  var SRG = vfs.SaltyRG( salt=>salt.push( new Date() ) )// callback is passed an array to add salt to.
  
  var signed_byte = SRG.getBits( 8, true );
  var unsigned_byte = SRG.getBits( 8 );
  var someValue = SRG.getBits( 13, true );  // get 13 bits of randomness, sign extended.
  var buffer = SRG.getBuffer( 1234 ); // get 155 bytes of random data (last byte only has 2 bits )
```



## Changelog
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
