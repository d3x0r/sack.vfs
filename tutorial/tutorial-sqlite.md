
# Sqlite

This executes a single command at a time; subsequent commands when multiple commands in the same command will be ignored. 

## Open a database

```
const db = sack.Sqlite( "mydbname.db" );
```

This would declare a variable which is now an open sqlite database.

## Do a command

```
const result = db.do( "select 1+1" );
```

result is an array of rows resulting from the select.


## Create a table

The internal create table support parses the provided create table statement, and compares it to the current definition of the
table in the database, and will add additional columns that do not already exist.

`makeTable()` will return true/false if is able to create the table.

``` js
db.makeTable( "create table something( a int, b int ) " );
```




## Parameterize Commands

Parameters to queries are best passed as bound parameters.  There is a method to escape strings for SQL, but this has
occasions it will fail.

### Init...

For this we need to know what the table structure looks like, so create a table...

``` js
	db.makeTable( "create table users ( name char, password char, deleted integer ) " ) )
```

### Setup...

Create some data in the table, and a value to lookup later...

``` js
	db.do( "insert into users (name,password,deleted) values (?,?,? )", 'userName','0123456',1 );
	var passHash = "0123456";
```

### Anonymous, ordered parameters

The first type of parameterization, which works in both Sqlite and ODBC database connections, using `?` to specify
the location of a parameter in the query, and then passing extra parameters after the query.


```
	const results = db.do( "select * from users where name=? and password=?", "userName", passHash );
```

### Anonymous, numbered parameters

Parameters can be defined by ordinal in Sqlite.  Parameters are `?` followed by a one based number indicating
the parameter that should be substituted into that location.

```
    const results = db.do( "select * from users where name=?2 and password=?1", passHash, "userName" );
```

### Named parameters

Parameters can be named in Sqlite.  Named parameters begin with a `$` follwed by more characters indiciating the 
name of the parameter.

```
    const results = db.do( "select * from users where name=$user and password=$pass", { $user:"userName", $pass: passHash } );

```

### Named and anonymous parameters

Combining both named and anonymous parameters.  The first parameter should be the object with all of the named
parameter's values.  The name of the value to Sqlite also includes the `$` specified in the query for the place holder; 
so values specified must also include the '$'.

```    
    const results = db.do( "select * from users where name=$user and password=$pass and deleted=?", { $user:"userName", $pass: passHash }, true );

```


### Processing results

Usually a query will result with an array of objects which represent the row data selected...


```
	if( results )
		for( let result of results ) {
			// process selected result.
		} 
```

### Naive parameter concatenization

Multiple strings, without any parameter specifications will be concatenated with appopriate escapes.

```
    sqlite.do( "select * from users where name=","userName","and password=", passHash );
```

## Results of a `do()` command

### SELECT

Select statements return an array of rows if the query is valid.  If the result had no data, the array is empty.  If 
the query fails, it throws an exception.


### All Others

the return is `undefined`.



## Handling Errors

Db.do will throw an exception if there is an error, the error message will in the exception. 

`makeTable()` results with true/false whether the table was created or matches the definition, or was not created. The 
error message of the database object (`db` as referenced in these tutorials) has an `error` getter which returns the last
error on the connection.  If an exception is thrown, the exception contains this error instead of the error field.


# Advanced Features


## UUID Type Keys

Using random ID (UUID, or similar) for primary keys can be useful.  SACK has a [Salty Random Number](tutorial_srg.md) generator
which can be used to generate cryptographically secure random numbers. 

A couple utility are exposed directly on sack `id()` and `Id()`.

```

const hashId = sack.id();
const shortHashId = sack.Id();
console.log( hashId, shortHashId );

// example output
// FrGfVvZZROu4YI$dq6oshLUflsq1V7Byn2lsgAVChYM= SgOiKedJ7APh8Wfw

```


`Id()` is 12 base64 digits, and is 96 bits long (slightly shorter than a UUID). 

`id()` is 44 base64 digits, and is 256 bits long (double the size of a UUID).   A UUID proper is 36 characters.


## User Functions

New functions may be registerd when using SQlite databae.  ODBC Connections do not support user defined functions.

### Deterministic Computational Functions

`db.function( name, callback)` defines a new function of the specified name which may be used later in a SQL query.
The function in SQL may be passed parameters, these parameters will be forwarded to the user defined function.
Value type is determined by the type of the result the function returns (such as: number, string, null,... ).

SQLite will cache the result of functions for specified inputs, so the function is only called when a new value
needs to be computed.


``` js

	//db.function( "func", (...params)=>{ return JSON.stringify( params) } );


	db.function( "func", sqlFunction );

	function sqlFunction( ...params ) {
		return JSON.stringify( params)
	}

```


Example usage:
  will result with the arguments passed as a JSON string.

```
	console.log( db.do( "select func(field,val,123,4.55) from table1" ) );
```


### Non-Deterministic fucntions

`db.procedure( name, callback)` defines a function the same as the aobve `function()` but the result is non-deterministic
and the function will always be called with new input data.

```
db.procedure( "dFunc", (c)=>`F(${c})` );
db.procedure( "hash", (c)=>`hash(${c})` );
var key = 1234;
db.function( "getID", ()=>{ return "" + (key++); } );
```

### User function Usage

User functions can be used as the default value for a table, or used in Insert select, update, delete statments.  (Experiemntally was trying to auto assign a hash ID much like auto_increment).

```
db.makeTable( "create table tableA ( \
		pk char default (getID(34)) PRIMARY KEY, \
		dataA )" ) ) 

db.makeTable( "create table tableF( \
     a char, \
     b char \
     default(hash(random())) ); -- no error, but wouldn't be 'constant' either..." );
```

### Custom aggregate functions


New methods to aggregate values like sum(x) may be defined.

```

db.aggregate( "agg"
      , (...params)=>{console.log( "step:", params ) }
      , ()=>{ console.log( "final"  ); return "abc" } );

```


Example Usage:	

```
	console.log( db.do( "select agg(val) from table1" ) );
```



## Overlapped name behavior

Sometimes, during the course of database design you want to standarize on field name like 'id' and 'name' and so you get the same name all over the place.  Getting the result of these names
when a query returns 'name' from multiple tables, the result record is different.

Example:

```
// create some tables
db.makeTable( "create table fruit ( fruit_id,name)" );
db.makeTable( "create table color ( color_id,name)" );
db.makeTable( "create table fruit_color ( fruit_id,color_id )" );

// make some data
db.do( "insert into fruit (fruit_id,name) values (1,'apple'),(2,'orange'),(3,'banana')" );
db.do( "insert into color (color_id,name) values (1,'red'),(2,'orange'),(3,'yellow')" );
db.do( "insert into fruit_color (fruit_id,color_id) values (1,1),(2,2),(3,3)" );
```




This select returns an object that on field `name`, it contains an array of values for each table.  It also contains the named tables and the associated.

It also has references from the tables... `fruit_color` and `fruit` become field names, which contain the column names and the related values.


```
console.table( db.do( "select 1,1,* from fruit \
		join fruit_color on fruit_color.fruit_id=fruit.fruit_id \
		join color USING(color_id)" ) );

/*
  {
    '1': 1,
    fruit: { fruit_id: 1, name: 'apple' },
    fruit_color: { fruit_id: 1, color_id: 1 },
    color: { name: 'red' },
    fruit_id: [ 1, 1, fruit: 1, fruit_color: 1 ],
    name: [ 'apple', 'red', fruit: 'apple', color: 'red' ]
  },
  {
    '1': 1,
    fruit: { fruit_id: 2, name: 'orange' },
    fruit_color: { fruit_id: 2, color_id: 2 },
    color: { name: 'orange' },
    fruit_id: [ 2, 2, fruit: 2, fruit_color: 2 ],
    name: [ 'orange', 'orange', fruit: 'orange', color: 'orange' ]
  },
  {
    '1': 1,
    fruit: { fruit_id: 3, name: 'banana' },
    fruit_color: { fruit_id: 3, color_id: 3 },
    color: { name: 'yellow' },
    fruit_id: [ 3, 3, fruit: 3, fruit_color: 3 ],
    name: [ 'banana', 'yellow', fruit: 'banana', color: 'yellow' ]
  }
]
*/
```

Another Example

```
console.table( db.do( "select fruit.name,color.name from fruit join fruit_color on fruit_color.fruit_id=fruit.fruit_id join color on fruit_color.color_id=color.color_id" ) );

/*
┌─────────┬────────────────────┬────────────────────┬──────────────────────────────────────────────────────────┐
│ (index) │       fruit        │       color        │                           name                           │
├─────────┼────────────────────┼────────────────────┼──────────────────────────────────────────────────────────┤
│    0    │ { name: 'apple' }  │  { name: 'red' }   │     [ 'apple', 'red', fruit: 'apple', color: 'red' ]     │
│    1    │ { name: 'orange' } │ { name: 'orange' } │ [ 'orange', 'orange', fruit: 'orange', color: 'orange' ] │
│    2    │ { name: 'banana' } │ { name: 'yellow' } │ [ 'banana', 'yellow', fruit: 'banana', color: 'yellow' ] │
└─────────┴────────────────────┴────────────────────┴──────────────────────────────────────────────────────────┘
*/

```

### Why doesn't this work?

The Sqlite parser fails on this with an overlap of colume and talbe name...  Though syntactically you can't specify just a `table` in any clause, everything
is by `column`, or by `table.column` which itself isn't `column.column` ever either... so there should never be a time that 'c' or 'c' would fill the same position.

Maybe it's a failure to lookahead that the failure is before it gets to '.' in the parsing state?

```
try {
   console.table( db.do( `select fruit.name as c,fruit.*,c.* from fruit join fruit_color USING(fruit_id) join color as c USING(color_id)`) );
   console.log( db.do( `select fruit.name as c,fruit.*,c.* from fruit join fruit_color USING(fruit_id) join color as c USING(color_id)`) );


} catch(err) {
   // expected error... "Column name overlaps table alias : c"
	console.log( "Expected to error, error is:", err );
}
```

