# sack.vfs

[![Join the chat at https://gitter.im/sack-vfs/Lobby](https://badges.gitter.im/sack-vfs/Lobby.svg)](https://gitter.im/sack-vfs/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)[![Build Status](https://travis-ci.org/d3x0r/sack.vfs.svg?branch=master)](https://travis-ci.org/d3x0r/sack.vfs)

Node addon for a custom virtual file system interface.  
JSON/JSON6 (stream)parser, 
JSOX (streaming) parser, 
COM/serial port access, Sqlite interface, an option/configuration database built on Sqlite.
WebSocket/HTTP/HTTPS network library.  UDP sockets.
Windows specific registry access for application settings. 

## Requirements

  ODBC and uuid support. (can be disabled)

#### Various Linux

 *  yum install gcc-c++ libuuid-devel unixodbc-devel
    * (uuid/uuid.h sql.h)
 *  apt-get install uuid-dev unixodbc-dev 
    * (uuid/uuid.h sql.h)
 *  pacman -S unixodbc util-linux
    * (sql.h uuid/uuid.h(probably already available, fs2util) )
 *  (?)emerge unixodbc

#### Mac

  *  (ODBC might be optioned out; just uses sqlite typically)
  *  brew (brew install unixODBC)

#### Windows
	none

# Usage

```
var sack = require( 'sack.vfs' );
var volume = sack.Volume( "MountName", "fileName.vfs" );
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
    JSOX - A jsox (JavaScript Object eXchange) parser. (JSON5/6 input compatible)
        parse(string) - result with a V8 object created from the json string.  
        begin( cb ) - begin parsing JSOX stream; callback is called as each value is completed.
        stringifier() - create a reusable stringifier which can be used for custom types
        stringify(object,replacer,pretty) - stringify an object; same API as JSON.
    JSON6 - A json parser. (JSON5 input compatible)
        parse(string) - result with a V8 object created from the json string.  
        begin( cb ) - begin parsing JSON6 stream; callback is called as each value is completed.
    JSON - A json parser.
        parse(string) - result with a V8 object created from the json string.  
        begin( cb ) - begin parsing JSON stream; callback is called as each value is completed.
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
    ObjectStorage( fileName [, version] ) - open a JS object storage database. 
    Volume([mountName,]fileName[,version]) - a virtual disk partition that holds files.
          mountName - the mount name used to mount to volume as a filesystem; it may be referenced 
                later in the string passed to Sqlite.  It may be `null` if it is anonymous mount.
          if no parameters are passed, a Volume object representing the native filesystem is returned.
    Volume.mkdir(pathname) - utility function to make directories which might not exist before volume does; 
            (Volume() auto creates directories now if specified path to filename does not exist)
            parameters - (pathname)
                path to create- will created missing parent path parts too.
    Volume.readAsString( filename ) - memory map map a file from native filesystem as a utf8 string.
    Volume.mapFile( filename, loaded_callback ) - memory map map a file from native filesystem as an arrayBuffer.
            if loaded callback is specified, a thread is created that touches every page in the file, when it's done
            the callback is called with no parameters.
    File - some native filsystem utility methods(?)
    FileMonitor - receive event notifications when files change
    SaltyRNG(feed salt callback) - creates a random number generator
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
        Thread - Helper functions to transport accepted clients to other threads.
            post( accepted_socket, unique_destination_string ) - posts a socket to another thread
            accept( unique_destination_string, ( newSocket )=>{} ) - receives posted accepted sockets
    HTTP - HTTP Request Method
        get( {options} ) - synchronous http request
    HTTPS - HTTPS Request Method
        get( {options} ) - synchronous https request
    Network - Raw network utilities
        Address( address [,port] ) - holder for network addresses.
        UDP( ... ) - UDP Socket to send/received datagrams.
    Config - Process configuration files; also streaming text parsing
        (methods)[#Config_Methods]
    // windows only
    registry - an interface to windows registry options
    	set( path, value ) - set a new value in the registry
        get( path )  - get a value from the registry.
    Task(options) - an interface for createing and monitoring tasks.
        Task constructor takes an option object.
        end() - cause a task to exit..
        write() - send something to a task.
        send() - send something to a task.
        terminate() - terminate created task.
}	
```


### Volume Interface 

  Arguments to Volume() constuctor `( mount name, filename, extra...)` .  If empty parameters `()`, the native filesystem is opened specifically.  If 
a mount is desired to be opened, it filename must be passed as null; `Volume( "mount", null )`.  If the first parameter is only a string, a VFS volume
by the name is opened and the volume is mounted anonymously; `Volume( "filename" )`. Similarly if the mount name is NULL, and a filename is passed as the second
parameter `Volume( null, "filename" )`, the volume is also mounted anonymously, so it can be refeenced when opening sqlite databases in it.  

`new` may be optionally applied to (most) any root object.  I may have gotten sloppy in some later works.  Everything behaves mostly as a factory by default.

Two addditional strings may be applied to encrypt the volume using those two key parts.

The following methods exist on the resulting object.

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
    readJSOX(fileName, callback) - read a file from a volume; calls callback with each object decoded from the file interpreted as JSOX.
    write(fileName,arrayBuffer/string) - writes a string or arraybuffer to a file. 
    Sqlite(database) - an interface to sqlite database in this volume.
    rm(file),delete(file),unlink(file) - delete a file.
    mv(file,new_filename),rename(file,new_filename) - rename a file. (mostly limited to same physical device)
    mkdir - utility function to make directories which might not exist before volume does; (volume auto creates directories now)
            parameters - (pathname)
                path to create- will created missing parent path parts too.
    rekey(version,a,b) - set key for a volume.
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


### File Monitor module

```

var sack = require( "." );
var monitor = sack.FileMonitor( <pathname>, idleDelay );
monitor.addFilter( "*.jpg", /* imageChanged */ (info)=>{
	// info.path, info.size, info.date, info.directory, info.created, info.deleted
} );

```



## Object Storage

(Added from 0.9.145)
Object storage interface is a hybrid combination of virtual file system and JSOX for object encoding.


So I made that JSOX thing; 
I forked my VFS from a while ago and changed the directory structure so it's a combined radix/remainder of name structure; so as a directory block fills with strings, it finds common ones, and updates a byte-character reference for all the files that start with that... 
I had wanted to do it somewhat more specifically to use 4096 hash which is 2 base64 characters \(except the ==\/\\0 \) at the end... but that ends up being like 16k block minimum; so I revised it to just use 256 entries (based on bytes of the filename).   It has some limitations, like \0 terminates a string; so index 0 will NEVER be used... and actually most control characters aren't in text filenames so hash entries 0-31 and (other than unicode) 127-250/ and definatly 250-255 are unused because those are never bytes in utf8 (using a string 'string' access for file content); but.  It works... generating random hashes `ba63,ab3j,b532` etc eventually overlaps in groups... anyway it's overall O(1) lookup; although that breaks into 10 hash hops (which is 10 pages of memory), plus 1 page for the name fragments.... ) and then each block has 254 'directory entires' (or 53 something 64 bit), which the directory is kept in-order always, so can do a binary search which is 8 string comparisons.... so a total of 10 cache loops, and 8 string comparisons worst case.  (This is in a single C file that compiles on any system with a C library or C++ if you prefer that.)  It can be compiled using emscripten which is about 120k minmum and about 300k total (gzips smaller).  The file system IO for emcc is IndexedDb (but I think I learned that here)....
So then I don't really want a 'file' access to the storage; although it has it, so it could be possible to update segments of blobs, I really intend it to store JS objects... which are stored as a whole and read as a whole (they are encoded to the disk in a container object and JSOX encoded once).  The Object Storage put and get use extended JSOX parser and stringifer that registers a handler for ~os as in `~os"abuasdfu31=="` (object storage identifier).  When it finds one of those, it knows it's a reference to another file in storage.  
If the object is loaded from storage with `get(id)`, and no other objects were loaded, the object would be returned with strings in the places of real objects... the can be manually revived with `obj.fixkey = objStore.get(obj.fixkey)`.  I added a `map(id)` which will also track the reviving objects, read all the files, and result with the completed object exactly as saved.... 
`put(object,bSign )` returns a new ID of the object, unless the object was previously stored or restored.  the optional argument for sigining can generate a nonce and result with a ID that is a PoW for the record and nonce.
records which have a nonce cannot be re-written.  other objects can be updated with their current state.
Oh - (one last note) Added sort to JSOX encoding so objects are always encoded in the same order ( as according to JS `if( stringA > stringB )`


```

const sack = require( "sack.vfs" );
const objectStorage = sack.objectStorage( 'filename.data' );

```

| Object Storage constructor arguments | Description  |
|----|----|
| filename | Filename to use for object storage database.  If the filename includes '@' the part before the '@' is treated as a mount name; ie. 'vfsMount@object.store'.


| Object methods | Return | Arguments | Description |
|----------|------------|-------------|----|
| put | unique ID | ( object \[, sign\] ) | Stores or updates an object into storage.  If the object was not previously stored or loaded, a unique ID for the object is returned. |
| get | promise |  ( id ) | Returns a promise; success is passed the object loaded from storage.  Loads an object using the specified ID. |
| map | promise |  ( id ) | Same as get, but also loads any objects of specified ID. |
| -- | | | |
| delete |  | (id/object) | remove object from storage... (danging references in other stored records?) |
 


## Sqlite Interface

  (result from vfs.Sqlite("dbName.db") or vfs.Volume().Sqlite("dbName.db"))
  Sqlite command only processes one command at a time.  If a multiple command separated with ';' or '\0' is issued, only the first command will be done.  
  This makes most destructive SQL injection impossible other than to make a query generally fail.
  If they are able to otherwise inject into a delete/drop command Still no joy;  
  Generally these are done during initialization with well formed things though...

Sqlite() will create a in memory database<br>
Sqlite( &lt;String&gt; ) will user the string to specify a filename.  Sqlite URI decoding is enabled.  `":memory:"` will also result in a memory only database.

There are methods on the Sqlite() function call...

| Sqlite function methods  |  |  |
|---|---|----|
| eo | (callback) | Enumerate Options.  Each option node in global database is passed to callback (see Option Nodes below) |
| go | ( &lt;String&gt; [,&lt;String&gt; [,&lt;String&gt;]] ) | Get Option.  First string is section, second optional string is option name, final string is default value of option.  Option name may be concated to section ( "section/option", "default" ) === ("section", "option", "default" ) |
| so | ( &lt;String&gt; [,&lt;String&gt; [,&lt;String&gt;]] ) | Set Option.  First string is section, second optional string is option name, final string is default value of option.  Option name may be concated to section ( "section/option", "new Value" ) === ("section", "option", "new Value" ) 
|   |  |  |
| GUI Functions |  | Added when compiled with full SACK library |
| optionEditor | ()  | Open gui option editor for global options |


#### Sqlite Instance Methods 

```
  var sack = require( 'sack.vfs' );
  var sqlite = sack.Sqlite( "test.db" );
  var vfsSqlite = sack.Volume( "sql Volume", "data.vfs" ).Sqlite( "test.db" );
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
| do | ( &lt;String&gt;) | execute a single sql command or query.  Results with null on error, or an array on success of a select, and true/false/scalar int on result of other commands.    If command generates no output, array length will be 0.  |
| do | ( &lt;Format String&gt; [,bound parameters... ] ) | This varaition, if the first string contains '?', then the first string is taken as the sql statement, and all extra paramters are passed to be bound to 1-N.  |
| do | ( &lt;Format String&gt; , object  [,bound parameters... ] ) | This varaition, if the first string contains ':','@', or '$', works like other format string, but second parameter must be an object with field names that match the names specified in the query.   (see examples below) |
| do | ( &lt;String&gt; [,bound parameters , &lt;More SQL String&gt; [,param,sql ... ] ) | The SQL statement is composed dynamically from parts.  Literal text of the query is interleaved with bound variables.  Each variable is replaced with a '?', and passed as an argument to bind to the statement. |
| op  | (section [, opName],defaultValue) |  get an option from sqlite option database; return defaultValue  if not set, and set the option int he database to the default value.
| getOption | (section [,opName],defaultValue) | longer name of 'op' |
| so | (section [,opName] ,value) | set an option in sqlite option database |
| setOption | (section [,opName] ,value) | longer name of 'so' |
| fo  | (opName) | find an option node under this one (returns null if node doesn't exist)<BR> fo( "name" ) |
| go  | (opName) | get an option node      <BR>go( "name" ) |
| eo  | ( callback(node,name)) |  enum option nodes from root of options, takes a callback as a parameter.<br> callback parameters ( optionNode, optionName ) ... the callback parameters get a node and a name.   The node is another option node that migth be enumerated with eo...<BR> `function callback(node,name)  {console.log( "got", name, node.value );` |
| function | (name, callback(...args) | Add a user defined function to the current sql connection.  'name' is the name of the function.  Set as deterministic; callback is called whenever the procedure's value is required by sqlite; values for inputs are cached so it doesn't need to always call the function.  Return value is given as result of function.   **ODBC CONNECTION UNDEFINED RESULT** |
| procedure | (name, callback(...args) | Add a user defined function to the current sql connection.  'name' is the name of the function.  Callback is called whenever the function is used in SQL statement given to sqlite.  Return value is given as result of function.   **ODBC CONNECTION UNDEFINED RESULT** |
| aggregate | ( name, stepCallback(...args), finalCallback() ) | Define an aggregate function called 'name' and when used, each row stepped is passed to the step callback, when the grouping issues a final, invoke the final callback.  Final result is given as the final value.  **ODBC CONNECTION UNDEFINED RESULT** |
| onCorruption | ( callback ) | callback will be called if a database corruption has been found (Sqlite) |
|   |  |  |
| log | accessor | set to false to disable logging on this specific connection (always returns false) |
|   |  |  |
| GUI Functions |  | Added when compiled with full SACK library |
| optionEditor | ()  | Open gui option editor with options in this database (Unimplemented) |


example sql command?
```
    var results = sqlite.do("select * from table");  // do a sql command, if command doesn't generate data result will be true instead of an array

    sqlite.function( "test", (val)=>val*3 );
    console.log( sqlite.do( "select test(9)" ) ); // results with 27


    var passHash = "0123456";
    sqlite.do( "select * from users where name=","userName","and password=", passHash );

    sqlite.do( "select * from users where name=? and password=?", "userName", passHash );
    sqlite.do( "select * from users where name=?2 and password=?1", passHash, "userName" );

    sqlite.do( "select * from users where name=$user and password=$pass", { $user:"userName", $pass: passHash } );
    
    sqlite.do( "select * from users where name=$user and password=$pass and deleted=?", { $user:"userName", $pass: passHash }, true );

```

### Option Database
  (result from vfs.[Volume().Sqlite()/Sqlite()].[fo/go/eo]() or any of these own methods )

| Option Node Instance methods |  |   |
|---|---|----|
| fo | (option name) | find an option node under this one (returns null if node doesn't exist)   fo( "name" )  |
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

db.function( "MyFunction", (...args)=>{ console.log( "Use Args:", args ); return JSON.strinigfy(args); } );
db.aggregate( "MyAggregate", (...args)=>{ console.log( "Use Args:", args ); }, ()=>{ return "Computed Aggregate Value"; );

```

db.do returns a result set (if any).  It will be an array of objects.  Objects will have fields named that are the names from the sqlite query.  
If the name is the same as another, the columns are amalgamated into another array... Data is transported similar to JSON, with 'null', Strings,
Numbers, and ArrayBuffer depending on the column data type.  An Integer number is used if it is an integer source type.

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

SELECT fruits.*,color.* from fruits join fruit_color join fruit_color join color 

[ { fruit_id : 1, name: [apple, red], color_id,  3 } 
 { fruit_id : 1, name: [apple, green], color_id,  4 } 
 { fruit_id : 2, name: [banana, yellow], color_id,  1 } 
  { fruit_id : 3, name: [orange, orange], color_id,  2 } 
 ]

-- newer output style
select fruit.*,color.* from fruit join fruit_color on fruit_color.fruit_id=fruit.fruit_id join color on fruit_color.color_id=color.color_id
[ { fruit: { fruit_id: 1, name: 'apple' },
    color: { color_id: 1, name: 'red' },
    fruit_id: 1,
    name: [ 'apple', 'red', fruit: 'apple', color: 'red' ],
    color_id: 1 },
  { fruit: { fruit_id: 2, name: 'orange' },
    color: { color_id: 2, name: 'orange' },
    fruit_id: 2,
    name: [ 'orange', 'orange', fruit: 'orange', color: 'orange' ],
    color_id: 2 },
  { fruit: { fruit_id: 3, name: 'banana' },
    color: { color_id: 3, name: 'yellow' },
    fruit_id: 3,
    name: [ 'banana', 'yellow', fruit: 'banana', color: 'yellow' ],
    color_id: 3 } ]


select fruit.name as fruit,color.name as color from fruit join fruit_color on fruit_color.fruit_id=fruit.fruit_id join color on fruit_color.color_id=color.color_id
[ { fruit: 'apple', color: 'red' },
  { fruit: 'orange', color: 'orange' },
  { fruit: 'banana', color: 'yellow' } ]  ... 
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


## [JSOX](https://www.github.com/d3x0r/jsox), and JSON( [JSON6](https://www.github.com/d3x0r/json6) ) - JSON and JSON6 compatible processor 

[JSOX](https://www.github.com/d3x0r/jsox) further extends JSON6 with Date support(subtype of numbers), BigInt support and TypedArray encoding.
It also adds the ability to define classes, which reduces data replication in the output, but also reduces
overhead serializing by having to parse shorter data; and gathering just values instead of `<Field> ':' <Value>` is less output usage.
Also, objects that are recovered with a class tag share the same prototype, allowing the objects read to be more readily used in applications.
JSOX parsing is 100% compatible with JSON6, JSON, and ES6(non code/function) parsing; that is JSOX can read their content with no issues.  There
is one small exception, when reading JSON6 in a streaming mode, it was possible to parse `1 23[45]` as `1`,`23`,`[45]` (similarly with {} instead of []), 
but JSOX requires at least one whitepsace between a number and a open brace or bracket, otherwise it tries to interpret it as a class tag.

JSON6 is a slightly extended json parser to allows simple types to be returned as values, not requiring { or [ to start
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
  - JSOX
     - parse( string [,reviver] )
     - begin( callback )
         - write( data )
         - registerFromJSOX(typeName,fromCb) - if an object of the specified name is encountered, the related object/array/string is passed to the callback, and the result is used as the revived object.
     - stringifier() - create a reusable stringifier which can be used for custom types
         - registerToJSOX(typeName,prototype,cb) - if an object that is an instance of prototype, the object is passed to the callback, and the resulting string used for output.
         - stringify(value[,replacer[,space]] ) - stringify using this stringifier.
         - setQuote( quote ) - specifies the perfered quoting to use (instead of default double-quote (") )
         - defineClass( name, object ) - registers a typed object for output; first object defintion is output which contains the failes, and later, typed objects are just their values.  Uses prototype of the object, unless it is the same as Object, or uses the fields of the object to compare that the object is of the same type.
     - stringify(object,replacer,pretty) - stringify an object; same API as JSON.
     - registerToJSOX(typeName,prototype,cb) - if an object that is an instance of prototype, the object is passed to the callback, and the resulting string used for output.
     - registerFromJSOX(typeName,fromCb) - if an object of the specified name is encountered, the related object/array/string is passed to the callback, and the result is used as the revived object.
     - registerToFrom(name,prototype,toCb, fromCb) - register both ToJSOX and FromJSOX handlers.


|JSOX Methods | parameters | Description |
|-----|-----|-----|
|parse| (string [,reviver]) | supports all of the JSOX features listed above, as well as the native [`reviver` argument][json-parse]. |
|stringify | ( value[,replacer[,space]] ) | converts object to JSOX.  [stringify][jsox-stringify] |
|stringifier | () | Gets a utility object that can stringify.  The object can have classes defined on it for stringification |
|escape | ( string ) | substitutes ", \, ', and \` with backslashed sequences. (prevent 'JSON injection') |
|begin| (cb [,reviver] ) | create a JSOX stream processor.  cb is called with (value) for each value decoded from input given with write().  Optional reviver is called with each object before being passed to callback. |
|registerToJSOX  | (name,prototype,toCb) | For each object that matches the prototype, the name is used to prefix the type; and the cb is called to get toJSOX |
|registerFromJSOX| (name,fromCb) | fromCb is called whenever the type 'name' is revived.  The type of object following the name is passd as 'this'. |
|registerToFrom  | (name,prototype,toCb, fromCb) | register both to and from for the same name |


Reviver callback parameter is not provided for streaming callback.

``` javascript
var vfs = require( "sack.vfs" );

var object = vfs.JSON.parse(string [, reviver]);

var object2 = vfs.JSON6.parse(string [, reviver]);

```

### Stringifier reference

|Stringifier method | parameters | Description |
|-------|------|-----|
|stringify | (value[,replacer[,space]] ) | converts object to JSOX attempting to match objects to classes defined in stringifier.  [stringify][json-stringify] |
|setQuote | ( quote ) | the argument passed is used as the default quote for strings and identifiers as required. |
|defineClass | ( name, object ) | Defines a class using name 'name' and the fields in 'object'.  This allows defining for some pre-existing object; it also uses the prototype to test (if not Object), otherwise it matches based on they Object.keys() array. |



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
     
| SaltyRNG Instance Methods | Args | Description |
|-----|-----|-----|
| seed |( toString ) : set the next seed to be used; any value passed is converted to a string.|
| reset|() : reset the generator to base state.|
| getBits|( [bitCount [, signed]] ) : if no parameters are passed, a 32 bit signed integer is returned.  If bitCount    is passed, it must be less than 32 bits.  If signed is not passed or is false, the resulting value is the specified       number of bits as an unsigned integer, otherwise the top bit is sign extended, and a signed integer results. |
| getBuffer|( bits ) : returns an ArrayBuffer that is the specified number of bits long, rounded up to the next complete byte count.|
| setVersion| () | Sets Sign/Verify generator to SHA3.  (needs additional options someday) |
| sign | ( [pad1 [, pad2],] string | get a signing identifier for string |
| setSigningThreads | ( n ) | Set the number of threads to sign with to n |
| verify | ( buffer, key ) | returns true/false if key is a signing key of buffer |
    

| SaltRNG Methods | Args | Description |
|----|-----|-----|
| sign | ( [pad1 [, pad2],] string | get a signing identifier for string |
| setSigningThreads | ( n ) | Set the number of threads to sign with to n |
| verify | ( buffer, key ) | returns true/false if key is a signing key of buffer |
| id | ( [source [, version]] ) | Version indicates which entropy generator to use internally |

The `id()` function takes an optional parameter that is an integer.

| id version | use generator |
|---|---|
| 0 | sha1 |
| 1 | sha2 |
| 2 | sha2_256 |
| 3 | sha3 |
| 4 | K12 |
|default| K12 |

```
  // some examples
  var vfs = require( "sack.vfs" );
  var SRG = vfs.SaltyRG( salt=&gt;salt.push( new Date() ) )// callback is passed an array to add salt to.
  
  var signed_byte = SRG.getBits( 8, true );
  var unsigned_byte = SRG.getBits( 8 );
  var someValue = SRG.getBits( 13, true );  // get 13 bits of randomness, sign extended.
  var buffer = SRG.getBuffer( 1234 ); // get 155 bytes of random data (last byte only has 2 bits )

  // these return base64 encoded bytes.
 var randomId     = SRG.id();  // default generator
 var randomId_v4  = SRG.id(4);  // use version 4 hashing
 var regenId      = SRG.id("Hash This ID");  // default hash generator (NEEDS TESTING!)
 var regenId_v4   = SRG.id("Hash This ID", 4); // regenerate, using hash 4

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

### Websocket Client Interface

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


### Websocket Server Interface

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


Server Options

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
  
Server Methods

  | Server Methods |   |
  |---|---|
  | close() | close server socket. |
  | on(eventName,cb) | setup events ( connect, accept, request, error ), callback |
  | onconnect(cb) | pass callback for when a WebSocket connection is initiated; allows inspecting protocols/resource requested to accept or reject. |
  | onaccept(cb) | pass callback for when socket is accepted, and is completely open.... setup event handlers on passed socket |
  | onrequest(cb) |pass callback for HTTP request (GET/POST/...).   |
  | onerror(cb) | pass callback for error event.  callback is passed a closed socket object with remote and local addresses (see connection object below) |
  | accept() | Call this to accept a socket, pass protocols to accept with(?).  Only valid within "accept" event. |
  | reject() | Call this in onconnect to abort accepting the websocket.  Only valid within "accept" event. |
  | disableSSL() | closes the SSL layer on the socket and resumes native TCP connection; is only valid during "lowerror" event type (6).  Uses the socket that triggered the event as the one to disable.  (The Websocket Server Client is not yet created). |


Server Client Methods
  this is a slightly different object than a client, although accepts the same events except for on( "open" ) and onOpen() method.
  
  | Method |  Description |
  |----|----|
  | post(target) | send this socket to another thread; best when applied during accept event handler(on accept) |
  | ping() | Generate a websocket ping.  /* no response/event? */ |
  | onmessage | set the callback to be called when a message is received, it gets a single parameter, the message recieved. The data is either a string or an ArrayBuffer type | 
  | send | send data on the connection.  Message parameter can either be an ArrayBuffer or a String. (to be implemented; typedarraybuffer) |
  | disableSSL | closes the SSL layer on the socket and resumes native TCP connection. |
  | close | closes the connection |
  | on | event handler for specified type `on(eventName, callback)` | 

### HTTP Request Object Description

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

## HTTP Request Interface ( HTTP/HTTPS )

```
var sack = require( "sack.vfs" );
var response = sack.HTTP.get( { hostname: "example.com", port: 80, method : "get", 
var response2 = sack.HTTPS.get( { ca:&lt;extra root cert(s)&gt;, rejectUnauthorized:true/false, path:"/index.html" } );
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

### Websocket Thread Support

This is support to be able to send accepted clients to other 'worker_threads' threads in Node.
Sockets which have been accepted can be moved; there's no reason to move the listener thread...
and client connections should be made on the intended thread, so there's no support to move those either.
This is only for threads accepted by a server, which may fork a thread, and hand off said socket to the 
specified target thread.  The handoff takes a unique string which should identify the target thread; although,
a pool of threads all using the same identifier may each receive one socket.  Once the accept event is fired,
it is cleared, and the thread will have to re-post a listener to accept another socket.

```
// rough example, not sure about the onaccept interface

var wss = sack.Websocket.Server( "::0" );
wss.onaccept( (wsc)=>{
	/// wsc  = websocket client
	// sack.Websocket.Thread.post( "destination", wsc );
        wsc.post( "destination" );
} );

// --------------------------------
// and then destination

sack.Websocket.Thread.accept( "destination", (socket)=>{
	// receve a posted socket
} );
```



```
        Thread - Helper functions to transport accepted clients to other threads.
            post( accepted_socket, unique_destination_string ) - posts a socket to another thread
            accept( unique_destination_string, ( newSocket )=>{} ) - receives posted accepted sockets
```



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

### Config Methods
   Configuration file processing; for processing formatted statements in text configurations.  Can
also be used to process streaming text input and responding with triggers based on configuration.
Things like a log analyzer can be created using this interface.

```
var sack = require( "sack.vfs" );
var processor = sack.Config();
processor.add( "some line with %i words", (n) => console.log( "found line param:", n ); );

//processor.go( "filename.txt" ); // reads configuration from a file...
processor.write( "Some sort of file streaming data\n" );  // the newline will be important, 
                                                          //otherwise it will be a single long line to match.

processor.write( "some line with 33 words\n" );  

// output:
// found line param: 33


// sample to read a configuration file.
var disk = sack.Volume();
processor.write( disk.read( "some file" ).toString() );
```



Configuration processor can also be used to build log scanners.  Since most of a line in a log is 
the same sort of information token matching and fitlering can trigger event handlers as each line
is discovered.  Recusive contexts can be used within config that will either disable the prior set 
of rules and begin a new set; for example, if a block header is discoverd that the following lines
would also relate to this, then it probably won't be any of the higher level rules for a while, so
a new sub-conconfiguration processor can be opened and rules added.  Whlie in this mode additional 
rules may also extend handlers; for example, processing a GUI configuration that describes a layout, 
a header 'Control....' might be discovered, and as part of the control there are lots of common properties ...
(CSS styles for instance), but then the type of the control might trigger adding an additional set of rules
for things specific to that type of control on the page (button click events), rules for a macro button, properties
for a text label like scroll, highlight, text line justification...



| Methods | arguments | description |
|----|-----|-----|
| add | (format, callback) | Add a matching expression to process match against the stream.  THe callback is passed the variables parts matched in the format statemnt in sequence of match. |
| go | (filename) | Reads a file and processes each line in the file |
| write | ( content) | Write some stream data to be processed; instead of coming from a file, data comes from this |
| on | (event,callback) | defines some event handlers on the stream.  
|   | "unhandled"  |  event is triggered on a line that does not match any format that has been added  |
|   | "done" |  file has finished processing (redundant, go() does not return until file is finished anyway   |
| begin | ( section name ) | begins a new configuration layer.  Rules can be added to this, and are applied until section is end()ed.  This allows defining things like custom configuration handler to handle sections of a configuration; an example might be the configuration defines controls in a layout, and some of the contorls are buttons that have different properties than say text fields. |
| end | () | ends the current sub-section processing.  Pops the context off the current processing stack |
| addFilter | ( data ) | Allows adding a transformation function on the data.  This can create custom ways to separate lines of text for instance (maybe it doesn't end with regular carriage returns).  Can also perform translation on the data and result with different data than it was passed. (Not yet implemented) |
| clearFilters | () | removes filters from the current handler (Not yet implemented) |


These methods are on the (module).Config object itself, not on an instance of the object.
|Static methods| arguments | Description |
| expand | (string) | Adds escape characters to the string to handle default filters.  (escape backslashes and hash marks for instane)
| strip | (string) | Remove extra characters from a string (unescape).  Usually don't have to do this, expand written will be stripped when read. |
| color | (color)  | format a parameter suitable for reading as a color formatted argument |
| encode | (TYpedArray) | format a block of binary data so it can be read in text; it's a variation of base64 encoding |
| decode | (stirng ) | result with a block of data that was previousy encoded |

#### Supported Formats
  This is a table of the format expressions allowed.  '%' character is the prefix of all of these.  if a literal percent is to be matched it should be prefixed with a backslash '\\%'

| Character | meaning |
|----|-----|
| m | multi-word.  Matches a sequency of words.  Ends at either the next word after %m specified or the end of line |
| w | single-word.  Matches a single word. |
| i | integer.  Matches a number.  Argument is passed as an integer |
| f | floating point number.  Matches a decimal/integer type number.  Argument is passed as a number. |
| b | boolean.  Matches (yes/no, true/false, on/off, 1/0, open/close) and is passed as a boolean argument |
| B | blob/binary.  String encountered matched binary encoded string; callback will be passed the binary buffer |
| c | color.  String matched a formatted color.  Color is passed to the callback (may require GUI support to be useful) |
| q | quotient.  Argument is passed as a fraction type (needs more support; fraction library interface not implemented) |
| u | url.  Matches a URL (host.address:port/with/path) 
| d | directory name.  allows gathering words with slashes as a path; must end with a slash (/ or \) |
| n | name of file.  Allows gathering words with slashes as a filename (same as directory?) |
| p | path (filepath).  Interpted path in sequence of words |
| a | a network address... host.name:port; collect a hostname/IP and optional port as an expression |
 

 ---

 ## Task

Task interface for launching and monitoring tasks.  Windows tasks are launched first by name, 
as processes, then as a shell execute (runs things like shortcut .lnk files), and then as 
a `cmd.exe /c ... ` command to run batch files.  Linux processes are attempted first to exec by the
name directly, and then try for each path set in PATH.

Pipes are connected to a task's stdin/stdout/stderr inputs if a output callback is specified.  The pipes
are left untouched otherwise.

| Task Static Methods | description |
| loadLibrary( libname ) | Load external shared library. ex: `sack.Task.loadLibrary( "xxx" );` |

Having created a task instance with `sack.Task( {...} );` the following methods are available
to interact with the process.

 | Task methods | Description |
 |----|----|
 |end() | attempt to cause a task to exit.  It will first dispatch ctrl-c, ctrl-break, post a WM_QUIT message, and if the program does not end soon enough, terminates the process.  (closing pipes to task could also be implemented?)| 
 |terminate() | Terminates the task.  Terminates the process. |
 |write(buf) | Writes data to the task's stdin. |
 |send(buf) | Writes data to the task's stdin. |
 |exitCode | After/during the `end` callback, this may be queried to get the return code of the task |

 Task Option object may contain these options to control how the task is spawned.

| Task options | Type | Description |
|----|----|-----|
| work  | string | specify the directory the task will be started in |
| bin | string | program to run |
| args | string or [ string [, string]...] | an argument or array of arguments to pass to the program |
| firstArgIsArg | bool | If false, first argument in `args` is program name and not an argument (POSIX exec); default is true, and the first argument in `args` is the first argument |
| env | object | key:value pairs to write to the environment before launching the process |
| binary | bool | if true, buffer to input callback will be an ArrayBuffer else it will be a string |
| input | callback(buffer) | A callback function which will receive output from the task(would have to update lower level library to split/identify if the input was stdout or stderr) | 
| end | callback() | This callback will be triggered when the task exits. |
| impersonate | bool | (Needs work;updated compatibility... this is for a service to launch a task as a user which is now impossible(?)) |
| hidden | bool | set windows UI flags such that the next process is created with a hidden window.  Default: false |
| firstArgIsArg | bool | Specified if the first argument in the array or string is the first argument or is the program name.  If it the first element is the program name, set to false.  If it is the first argument set true.  Default: true |
| newGroup | bool | create task as a new task group instead of a child of this group.  Default: false|
| newConsole | bool | create a new console for the new task; instead of ineriting the existing console, default false |
| suspend | bool | create task suspended.  Default: false |


```
var sack = require( "sack.vfs");

// don't redirect/capture input/output
var task1 = sack.Task( {bin:"echo", args:"hello, World"});

// send tasks's output to console.log...
var task2 = sack.Task( {bin:"echo", args:"hello, World", input:console.log });

sack.Task( { bin: "notepad.exe", args:"test.txt" } );
// default tasks exit when node does... or when garbage collected... 
// unless end and or input event handlers are attached...
setTimeout( ()=>{ }, 5000 );


```


---

## Changelog
- 0.9.159
   - Implement websocket thread interface to post accepted sockets to other threads.
- 0.9.158
   - add test for typed array revival; fix typed array revival (JSOX).
- 0.9.157 
   - revert UTC change.
- 0.9.156
   - missed getFullYear->getUTCFullYear update.
- 0.9.155
   - normalize JSOX class defintiion keys for later comparison to objects.
   - removed node 7 travis integration; `#  - "7" (doesn't have Utf8Value with isolate, 12 doesn't have Utf8Value without isolate())`
   - added FileMonitor interface to get event changes when files change on the disk.   
   - fixed some issues with JSOX parsing ( tagged class in tagged class in map )
   - fix time precision in JSOX stringification (add ms), and was encoding Localtime but claiming UTC.
   - fix error in salty random generator partial bit streams.
   - Implement worker_threads support better; remove static global persistent things.
   - Updated parsers so '\xa0' is the same as '\x20'.  Most parsers are for human-readable code; JSOX, TextParse, BurstExx.
- 0.9.154
   - Continued applying deprecation fixes; republished as 154
- 0.9.153
   - Republish 0.9.152 with ignored source
- 0.9.152
   - Update to Node 12 (Requires usage of context, and MaybeLocal types)
   - Fixed id regnerator to match JS version in more cases.
   - Use pthread_mutex for waits.
   - Improve static global usage so no external memory allocation gets used. (/tmp/.shared*)
   - Removed now unused signal handler registrations.
- 0.9.151
   - SKipped due to error in tagging.
- 0.9.150
   - Fix regression handline NUL inline in JSOX parser (re fix partial codepoints received across buffer bounds)
   - Fix releasing the buffer too soon on HTTP fallback from HTTPS.
- 0.9.149
   - Fixed getting events from listening sockets before being fully setup.
- 0.9.148
   - Updated documentation to cover lowerror event and disableSSL.
   - Added ping() method call on sockets accepted by a server and sockets connected to a server.
   - Fixed lost event during SSL handshake.
   - Fixed linux network write lock issue.
   - Fix static initializing SQL.
- 0.9.147 
   - fixed some issues with JSOX.
   - Fixed occasional event loss for SSL connections.
   - Improved ODBC Data marshalling ability.
   - Fix lost task output from task module tasks.
   - Added Text() type.  Which is the internal rich text segment tracking.  (for output to GUI console) 
- 0.9.146
   - SACK Update; add XSWS 
   - Fixed stall issue with http server and lots of requests in series from browser.
   - Fixed http server issue getting data while processing another header.
- 0.9.145 
   - Add 'mv', 'rename' methods to Volume() instance.  
   - Split locks on ssl read/write.  
   - Fix VFS directory truncation issue (early EODMARK injection). 
   - TLS Certificate add Certificate Policy
   - Add JSOX Parser.
   - Add Object Storage Module.
   - Added support for paramter binding in Sqlite.
   - Improve data marshalling for Sqlite to JS and vice versa.
   - Network improvement for send and close edge conditions.
   - Implement parameter binding for sqlite and ODBC (less tested).
   - Fixed spurious HTTP request failures when the connection closed with writes pending.
   - Improves SRG module; allow configuring SRG instance with a specific entropy generator.
   - Fixed task output stall.
- 0.9.144 - Fix websocket receiving packets with multiple frames.
- 0.9.143 - Improve task interface.  Simplify com data buffer; it's now only valid during receive callback. Improve websocket server handling http requests; add a event callback when socket closes, after server HTTP to distinguish between incomplete(TLS error) connections. Sync SACK updates: improve SQL parsing/table-index generation, library load path for current and name as passed, event for http close, some protection against dereferencing null parameters.
- 0.9.142 - Fix node-gyp for windows build.
- 0.9.141 - Add callback event to trigger background thread preload memory mapped files.
- 0.9.140 - Fix bad test on opening file in VFS.
- 0.9.139 - Added onerror callback for websocket server connections.  Add low level windows keyboard interface module.  Added memory mapped file to arraybuffer.  Fix https init failure with certain combinations of options.
- 0.9.138 - Add alpha methods for generating signing identifiers.  Fixes some lost network close notivications.
- 0.9.137 - fix pathchr insensitive path comparison; update tls interface for newer openssl; allow opening volumes by mount name only.
- 0.9.136 - added log option to sql connections.
- 0.9.135 - Fix unmounted volumes to instead mount hidden.
- 0.9.134 - disable delay-load build option for node-gyp build.
- 0.9.133 - Isolate in Utf8Value is actually a very new thing.
- 0,9.132 - Make a pass to cleanup deprecated Utf8Value() usage.
- 0.9.131 - Fixes for Node 10 (missing export TLS_1_2...; backfix for < 10)
- 0.9.130 - Fixes for Node 10.
- 0.9.129 - (cont) merge linux changes.
- 0.9.128 - (cont) lost some changes; reapply on windows.
- 0.9.127 - (cont) Also implement respecting keep-alive on connection.
- 0.9.126 - (cont)
- 0.9.125 - Updated VESL Parsing(WIP); Fixed latency on windows server socket close.
- 0.9.124 - Fix segfault in latest node; resolve ToInteger deprication warnings.  
- 0.9.123 - Fix missing sources in binding.gyp build rules.
- 0.9.122(unpublished) - Add VESL parsing(in progress). Add Task interface.  Fix create table emitter.
- 0.9.121 - Fix missing close after all data queued to be sent was sent.
- 0.9.120 - Fix crash caused by closing socket during http request dispatch.
- 0.9.119 - Network Scheduling error on windows.
- 0.9.118 - Fix lock issue with SSL read/write.  Fix windows wait on short event count.  Fix VES unlink so files disappear even though still open.
- 0.9.117 - lock on wrong side. 
- 0.9.116 - Fix a windows network lock issue. Handle ArrayBuffer output for http response.
- 0.9.115 - Fix stray unlock.
- 0.9.114 - Fix leave critical section in release mode.
- 0.9.113 - Added optional version parameter for VFS. Sync sack sources... A lot of reformats; updates for split network lock.  Added Sqlite.procedure to define deterministic function.
- 0.9.112 - decode of unicode character escape had bad calculation.
- 0.9.111 - promote to more appropriate version.  If anyone else joins; this should go to 1.0.  Improve TLS error reporting and SQL result set ability.  Improve table parsing.
- 0.1.99324 Test and Update sqlite user defined functions (function/aggregate); improved data type retention.
- 0.1.99323 Fix mac ( got travis integration working for mac).
- 0.1.99322 Some fixes building on mac.  Added error accessor to sqlite object.  Added user functions and aggregates to sqlite interface.
Fixed windows registr access. Added interface to configscript parser.  Added analog interface for clock object (GUI build).  Lots of changes 
SACK core code for user functions, json parse fixes, DeleteFromSet parameters, optimize poplink, added nolock dequelink, added callback for websocket
data receive completion, 
- 0.1.99321 Fix some edge json parsing cases.  Fixed some network issues.  Allow NUL characters in json parsing.
- 0.1.99320 Fix json escape of strings containing NUL characters.  Add HTTP(s) request methods.
Improve/modify PSI control registration/creation a little.  Expose font select dialog, color picker dialog, 
Fix property accessors in websocket module.
- 0.1.99319 Fix older gcc crash from bad optimization.
- 0.1.99318 Fix udp readStrings option.
- 0.1.99317 Added optional GUI interfaces. (build with `npm run build-gui` or `npm run build-gui-debug`)
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
