
var sack=require( '..');
db = sack.Sqlite( "mySQL" );
console.log( db.do( "show tables" ) );
