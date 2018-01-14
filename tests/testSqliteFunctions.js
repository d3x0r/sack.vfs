var sack = require( ".." );

const db = sack.Sqlite( 'test.db' );

console.log( db.do( "select * from sqlite_master" ) );
try {
db.do( `drop table table1` );;
//db.do( `drop table \`table\`` );;
} catch( err) { }

console.log( db.do( "select * from sqlite_master" ) );
if( db.makeTable( "create table table1 ( field ,val )" ) )
	db.do( `insert into table1( field, val ) values ("a","b"),("c","d") ` );;

db.function( "func", (a,b,c,d)=>{console.log( "passed", a,b,c,d ); return JSON.stringify( [a,b,c,d] ) } );
db.aggregate( "agg"
      , (...params)=>{console.log( "step:", ...params ) }
      , ()=>{ console.log( "final"  ); return "abc" } );

console.log( db.do( "select func(field,val,123,4.55) from table1" ) );
console.log( db.do( "select agg(val) from table1" ) );
