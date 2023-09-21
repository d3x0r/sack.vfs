
var sack=require( ".." );
//sack.Volume().unlink( "test.db" );
var db = sack.ODBC( "MySQL" );
try {
db.do( "create database `test-alias`" );
} catch( err) {
	console.log( "already exists?", err );
}
db.do( "use `test-alias`" );

db.makeTable( "create table egg ( parent_id int,data int,id int,member char(32))" );
db.makeTable( "create table fog ( parent_id int,data int,id int,member char(32))" );
db.makeTable( "create table gap ( noOverlap int,column int,names char(32),parent_id int)" );


db.do( "insert into egg (parent_id,data,id,member) values (1,2,3,'asd'),(3,5,4,'cat'),(4,9,2,'bad')" );
db.do( "insert into fog (parent_id,data,id,member) values (1,2,3,'asd'),(3,5,4,'cat'),(4,9,2,'bad')" );
db.do( "insert into gap (parent_id,noOverlap,`column`,names) values (1,2,3,'asd'),(3,5,4,'cat'),(4,9,2,'bad')" );


console.log( db.do( "select * from egg as foo join gap as bar2 on foo.parent_id=bar2.parent_id" ) );

console.log( db.do( "select foo.*,bar.* from egg as foo join egg as bar on foo.id=bar.parent_id" ) );
console.log( db.do( "select * from egg as foo join egg as bar on foo.id=bar.parent_id join egg as gge on foo.id=gge.parent_id" ) );


//console.log( db.do( "select * from egg as foo join egg as bar on foo.id=bar.parent_id" ) );

console.log( db.do( "select * from egg as foo" ) );
