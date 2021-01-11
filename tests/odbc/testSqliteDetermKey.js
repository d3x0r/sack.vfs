
var sack=require('..' );
sack.Volume().unlink( "testDeterm.db" );
var existed = sack.Volume().exists( "testDeterm.db" );


var db = sack.Sqlite( 'testDeterm.db' );
db.procedure( "dFunc", (c)=>`F(${c})` );
db.procedure( "hash", (c)=>`hash(${c})` );
var key = 1234;
db.function( "getID", ()=>{ return "" + (key++); } );

if( !existed ) {

if( !db.makeTable( "create table tableA ( pk char default (getID(34)) PRIMARY KEY, dataA )" ) ) 
  console.log( "Table A Failed?", db.error );

if( !db.makeTable( "create table tableB ( fk, dataB, FOREIGN KEY (fk) REFERENCES tableA(pk) ON DELETE CASCADE ON UPDATE CASCADE )" ) )
  console.log( "Table B Failed?", db.error );

if( !db.makeTable( "create table tableC ( fk, dataC, dFuncC char default (dFunc(dataC)) , FOREIGN KEY (fk) REFERENCES tableA(pk) ON DELETE CASCADE )" ) )
  console.log( "Table C Failed?", db.error );

if( !db.makeTable( "create table tableC2 ( fk, dataC2, dFuncC2 char default (dFunc(12)) , FOREIGN KEY (fk) REFERENCES tableA(pk) ON DELETE CASCADE )" ) )
  console.log( "Table C Failed?", db.error );

if( !db.makeTable( "create table tableC3 ( fk, dataC3, dFuncC3 char default (dFunc()) , FOREIGN KEY (fk) REFERENCES tableA(pk) ON DELETE CASCADE )" ) )
  console.log( "Table C Failed?", db.error );

console.log( db.do( "select * from sqlite_master" ) )

try {
db.do( "alter table tableB add column dFuncB char default (dFunc(dataB))" );
db.do( "create INDEX dFuncIndex on tableB ( dFuncB )" );
} catch(err) {
	console.log( "Error:", err );
}


db.do( "insert into tableA( dataA) values( 'apple')" );
db.do( "insert into tableA( dataA) values( 'banana')" );
console.log( db.do( "select * from tableA" ) );

db.do( "insert into tableB( dataB) values( 'cat')" );
db.do( "insert into tableB( dataB) values( 'dog')" );
console.log( db.do( "select * from tableB" ) );


db.function( "myrandom", ()=>Math.random() );

db.do( "create table tableD ( a char )" );
db.do( "insert into tableD( a) values (1),(2),(3),(4)" );
//-- alter table tableD add column b char default (random()) )  --Error: Cannot add a column with non-constant default
try {
db.do( "alter table tableD add column b char default (myrandom()) " );
} catch( err ) {
console.log( "Failed to add column.", err );
}
console.log( db.do( "select * from tableD" ) );

}

//db.do( "drop table tableF" );
//db.do( "create table tableF( a char, b char default(hash(a)) ); -- Error: default value of column [b] is not constant" );
db.makeTable( "create table tableF( a char, b char default(hash(random())) ); -- no error, but wouldn't be 'constant' either..." );


db.do( "insert into tableF ( a) values (1),(2),(3),(4);" );
console.log( db.do( "select * from tableF;" ) )

db = null;
