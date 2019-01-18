

var sack = require( ".." );
sack.Volume().unlink( "test.db" );

var db = sack.Sqlite( "test.db" );
db.makeTable( "Create table test ( a integer, b float, c text, d blob )" );

db.do( "insert into test (a,b,c,d) values ($a,$b,$c,$d)", { $a : 123, $b : 3.14, $c : "Hello", $d : new Uint8Array(8) } );
db.do( "insert into test (a,b,c,d) values ($a,$b,$c,$d)", { $a : null, $b : null, $c : null, $d : null } );

console.log( db.do( "select * from test" ) );
console.log( JSON.stringify( db.do( "select * from test" ), null, "\t" ) );

