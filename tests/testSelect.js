
var sack=require(".." );
var db = sack.Sqlite( "test.db" );

if( db.do( "select 1 from sqlite_master where tbl_name='b' and type='table'" ).length < 1 ) {
	console.log( "create table" );
        db.makeTable( "create table b ( a char )" );
        db.do( "insert into b (a) values (1),(2),(3)" );
}
else
	console.log( "existing Table" );


if( db.do( "select 1 from sqlite_master where tbl_name='a' and type='table'" ).length < 1 ) {
	console.log( "create table" );
        db.makeTable( "create table a ( a char )" );
        db.do( "insert into a select * from b" );
}
else
	console.log( "existing Table" );
        
console.log( "Result:", db.do( "select * from a" ) );
