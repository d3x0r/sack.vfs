
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



## Handling Errors

Db.do will throw an exception if there is an error, the error message will in the exception.

`makeTable()` results with true/false whether the table was created or matches the definition, or was not created.
