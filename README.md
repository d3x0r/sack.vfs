# sack.vfs

[![Join the chat at https://gitter.im/sack-vfs/Lobby](https://badges.gitter.im/sack-vfs/Lobby.svg)](https://gitter.im/sack-vfs/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
Node addon for a custom virtual file system interface

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
    JSON - A json parser. 
        parse(string) - result with a V8 object created from the json string.  
        1/2 the speed of Node's parser; no real reason to use this.
        // var o = vfs.JSON.parse(s)
    mkdir(pathname) - utility function to make directories which might not exist before volume does; (volume auto creates directories now)
            parameters - (pathname)
                path to create- will created missing parent path parts too.
    Sqlite(dbName) - an interface to sqlite which can open databases in the volume.
            parameters - ( databaseName )
                databaseName maybe a simple ODBC name; if the name ends with .db, it is assumed to be a sqlite database.  
                If ODBC is not available, it may be assumed that the name will just be a sqlite database name.
                extra syntax - if the name starts with a $, then the next word after $ and before 
                   the next @ or $ is used to select a sqlite vfs name.( https://sqlite.org/vfs.html )  
                   if the name is 'sack' then after @ and before the next $ is specified a mounted filesystem name.
        Sqlite has methods available on it to access native program options.
        Sqlite.op( opName, defaultValue ) - read/write default option database option.
        Sqlite.so( opName, newValue ) - write new value to default option database.
    Volume(mountName,fileName,a,b) - a virtual disk partition that holds files.
          mountName - the mount name used to mount to volume as a filesystem; it may be refernced later in the string passed to Sqlite.  It may be `null` if it is anonymous mount.
          if no parameters are passed, a Volume object representing the native filesystem is returned.
    
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
    write(fileName,arrayBuffer/string) - writes a string or arraybuffer to a file. 
    Sqlite(database) - an interface to sqlite database in this volume.
    mkdir - utility function to make directories which might not exist before volume does; (volume auto creates directories now)
            parameters - (pathname)
                path to create- will created missing parent path parts too.
}
```
### File Interface opened within a Volume
 (result from vfs.Volume().File())
File = {
    read(size) - read from the file; return an ArrayBuffer with a toString method to interpret it as utf8.
    write(arrayBuffer/string) - write data to the file; at the current position
    seek(pos) - set file position to write
    trunc() - set file size to the curent file position.
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
