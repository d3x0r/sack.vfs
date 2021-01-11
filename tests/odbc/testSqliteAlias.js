
var sack=require( "../.." );

var db = sack.Sqlite( "MySQL" );
db.do( "use testDb" );

db.makeTable( "create table egg ( parent_id char,data char,id char,member char)" );
db.makeTable( "create table fog ( parent_id char,data char,id char,member char)" );
db.makeTable( "create table gap ( noOverlap int,column char,names char,parent_id char )" );


db.do( "insert into egg (parent_id,data,id,member) values (1,2,3,'asd'),(3,5,4,'cat'),(4,9,2,'bad')" );
db.do( "insert into fog (parent_id,data,id,member) values (1,2,3,'asd'),(3,5,4,'cat'),(4,9,2,'bad')" );
db.do( "insert into gap (parent_id,noOverlap,`column`,names) values (1,2,3,'asd'),(3,5,4,'cat'),(4,9,2,'bad')" );


console.log( db.do( "select * from egg as foo join gap as bar2 on foo.parent_id=bar2.parent_id" ) );

console.log( db.do( "select foo.*,bar.* from egg as foo join egg as bar on foo.id=bar.parent_id" ) );
console.log( db.do( "select * from egg as foo join egg as bar on foo.id=bar.parent_id join egg as egg on foo.id=egg.parent_id" ) );


//console.log( db.do( "select * from egg as foo join egg as bar on foo.id=bar.parent_id" ) );

console.log( db.do( "select * from egg as foo" ) );
