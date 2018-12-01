var sack = require( ".." );

const db = sack.Sqlite( 'test.db' );

console.log( db.do( "select * from sqlite_master" ) );
try {
	db.do( `drop table table1` );;
} catch( err) { }

console.log( db.do( "select * from sqlite_master" ) );
if( db.makeTable( "create table table1 ( field ,val )" ) )
	db.do( `insert into table1( field, val ) values ("a","b"),("c","d") ` );;

db.function( "func", (...params)=>{console.log( "passed", params ); return JSON.stringify( params) } );
db.aggregate( "agg"
      , (...params)=>{console.log( "step:", params ) }
      , ()=>{ console.log( "final"  ); return "abc" } );

console.log( db.do( "select func(field,val,123,4.55) from table1" ) );
console.log( db.do( "select agg(val) from table1" ) );
