var vfs = require( ".." );
//var db = vfs.Sqlite( "file://localhost/gun.db?nolock=1" );   
//open:file://localhost/gun.db?nolock=1
//Sqlite3 Err: (14) os_win.c:42795: (2) winOpen(M:\gun.db) - The system cannot find the file specified.


var db = vfs.Sqlite( "file:gun.db?nolock=1&mode=rwc" );

db.do ("PRAGMA journal_mode=PERSIST" );
db.do ("PRAGMA synchronous = 0" );
db.do ("PRAGMA locking_mode = EXCLUSIVE" );

db.makeTable( "create table a (a int PRIMARY KEY, b int )" );
var n = 0;
for( n = 0; n < 10; n++ ) {
console.log( db.do ("select * from a where a=3" ));
db.do( "replace into a (a,b) values (3," + n + ")");
}

//process.exit(0);
var db2 = vfs.Sqlite( "gun.db" );
db2.makeTable( "create table a (a int )" );


var db = vfs.Sqlite( "$sack@native$native.db" );
db.makeTable( "create table a (a int PRIMARY KEY, b int )" );
for( n = 0; n < 10; n++ ) {
console.log( db.do ("select * from a where a=3" ) );
db.do( "replace into a (a,b) values (3," + n + ")");
}

