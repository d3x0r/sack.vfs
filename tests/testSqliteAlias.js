
var sack=require( ".." );
sack.Volume().unlink( "test.db" );
var db = sack.Sqlite( "test.db" );

db.makeTable( "create table egg ( parent_id,data,id,member)" );
db.makeTable( "create table fog ( parent_id,data,id,member)" );
db.makeTable( "create table gap ( noOverlap,column,names,parent_id )" );


db.do( "insert into egg (parent_id,data,id,member) values (1,2,3,'asd'),(3,5,4,'cat'),(4,9,2,'bad')" );
db.do( "insert into fog (parent_id,data,id,member) values (1,2,3,'asd'),(3,5,4,'cat'),(4,9,2,'bad')" );
db.do( "insert into gap (parent_id,noOverlap,column,names) values (1,2,3,'asd'),(3,5,4,'cat'),(4,9,2,'bad')" );


console.log( db.do( "select * from egg as foo join gap as bar2 on foo.parent_id=bar2.parent_id" ) );

console.log( db.do( "select foo.*,bar.* from egg as foo join egg as bar on foo.id=bar.parent_id" ) );
console.log( db.do( "select * from egg as foo join egg as bar on foo.id=bar.parent_id join egg as egg on foo.id=egg.parent_id" ) );


//console.log( db.do( "select * from egg as foo join egg as bar on foo.id=bar.parent_id" ) );

console.log( db.do( "select * from egg as foo" ) );
