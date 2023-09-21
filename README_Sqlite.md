
# SQL

`sack.Sqlite` is also aliased as `sack.DB` and `sack.ODBC`; the database access is not exclusively Sqlite.
All three are the same function.



## SQL Interface



  (result from `vfs.Sqlite("dbName.db")` or `vfs.Volume().Sqlite("dbName.db")`)

Sqlite command only processes one command at a time.  If a multiple command separated with ';' or '\0' is issued, only the first command will be done.

Calling `sack.Sqlite` with no arguments `sack.Sqlite()` will create an in-memory temporary database.

Sqlite( &lt;String&gt; ) will user the string to specify a filename or ODBC DSN.

Sqlite( &lt;String&gt; [, callback] ) will user the string to specify a filename or ODBC DSN, optional callback is called with the database connection when it is opened (or reopened).  

Sqlite URI decoding is enabled.  `":memory:"` will also result in a memory only database.

There are methods on the Sqlite() function call...

| Sqlite function methods  |  |  |
|---|---|----|
| eo | (callback) | Enumerate Options.  Each option node in global database is passed to callback (see Option Nodes below) |
| go | ( &lt;String&gt; [,&lt;String&gt; [,&lt;String&gt;]] ) | Get Option.  First string is section, second optional string is option name, final string is default value of option.  Option name may be concated to section ( "section/option", "default" ) === ("section", "option", "default" ) |
| so | ( &lt;String&gt; [,&lt;String&gt; [,&lt;String&gt;]] ) | Set Option.  First string is section, second optional string is option name, final string is default value of option.  Option name may be concated to section ( "section/option", "new Value" ) === ("section", "option", "new Value" ).  Low level options may be set with ("comports.ini", "COM PORTS/"+port, value) |
|   |  |  |
| GUI Functions |  | Added when compiled with full SACK library |
| optionEditor | ()  | Open gui option editor for global options |


#### Sqlite Instance Methods 

``` js
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
| provider | accessor | Get the database provider for the connection. (1=SQLite, 2=MySQL, 3=PSSQL, 4=Access, -1=unknown) |
| do | ( &lt;String&gt;) | execute a single sql command or query.  Results with null on error, or an array on success of a select, and true/false/scalar int on result of other commands.    If command generates no output, array length will be 0.  |
| do | ( &lt;Format String&gt; [,bound parameters... ] ) | This varaition, if the first string contains '?', then the first string is taken as the sql statement, and all extra paramters are passed to be bound to 1-N.  |
| do | ( &lt;Format String&gt; , object  [,bound parameters... ] ) | This varaition, if the first string contains ':','@', or '$', works like other format string, but second parameter must be an object with field names that match the names specified in the query.   (see examples below) |
| do | ( &lt;String&gt; [,bound parameters , &lt;More SQL String&gt; [,param,sql ... ] ) | The SQL statement is composed dynamically from parts.  Literal text of the query is interleaved with bound variables.  Each variable is replaced with a '?', and passed as an argument to bind to the statement. |
| run | any do() args | behaves like `do(...)` but returns a promise, and is executed in a background thread; some ODBC drivers will fail because they only support one outstanding statement (MSSQL for example) |
| op  | (section [, opName],defaultValue) |  get an option from sqlite option database; return defaultValue  if not set, and set the option int he database to the default value.
| getOption | (section [,opName],defaultValue) | longer name of 'op' |
| so | (section [,opName] ,value) | set an option in sqlite option database |
| setOption | (section [,opName] ,value) | longer name of 'so' |
| fo  | (opName) | find an option node under this one (returns null if node doesn't exist)<BR> fo( "name" ) |
| go  | (opName) | get an option node      <BR>go( "name" ) |
| eo  | ( callback(node,name)) |  enum option nodes from root of options, takes a callback as a parameter.<br> callback parameters ( optionNode, optionName ) ... the callback parameters get a node and a name.   The node is another option node that migth be enumerated with eo...<BR> `function callback(node,name)  {console.log( "got", name, node.value );` |
| require | setter/getter | set the option for required, which will attempt to re-open a connection if it fails; the optional callback specified when opening the connection will be re-called. |
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
``` js
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

``` js
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

``` js
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

``` js
 console.log( db.do( 'SELECT * from fruits join fruit_color join color' );
```

 Would probably return soemthing like the above. Numbers will be type Number, strings are strings, and well otherwise that's it.
I don't know; maybe it's smart enough if it's the same value it doesn't exapnd it to an array.  That was what one MS SQL driver gave me back when I did a join with a ```*```... the joined column's values were put into an array instead of a simple ID... 
That's why the names from the seaprate tables get put into the same value as an array... If the query were more like

``` js
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
``` js
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
``` js
function tick() {
        console.log( "dump tables: \n"
                , db.do( "select * from option4_map" ) 
                );
}
tick();
```

