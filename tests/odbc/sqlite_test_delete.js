
var sack = require( "../.." );
var db = sack.Sqlite( "MySQL" );

console.log( "Table?", db.do( "show tables" ) );

db.do( "create database if not exists testDb")
db.do( "use testDb")
console.log( "Table?", db.do( "show tables" ) );

//db.makeTable( "create table users ( user_id char default '1', name char )" );
db.makeTable( "create table users ( user_id char default (random()), name char )" );
if( !db.makeTable( "create table userData( user_id char, data char, FOREIGN KEY(user_id) REFERENCES users(user_id) ON DELETE CASCADE)") )
console.log( "Error creating:", db.error );
db.do( "insert into users (name) values ('a')" );
//db.do( "insert into users (name) values ('a'),('b'),('c')" );
console.log( db.do( "select * from users" ) );
console.log( db.do( "select * from userData" ) );
db.do( "insert into userData (user_id,data) select user_id,name from users" );
db.do( "delete from userData  where userData = 'a'" );
