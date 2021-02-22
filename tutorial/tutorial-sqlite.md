
# Sqlite


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

```
db.makeTable( "create table something( a int, b int ) " );
```



