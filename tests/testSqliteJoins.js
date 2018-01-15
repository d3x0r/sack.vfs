
var sack=require( ".." );
sack.Volume().unlink( "test.db" );
var db = sack.Sqlite( "test.db" );

db.makeTable( "create table fruit ( fruit_id,name)" );
db.makeTable( "create table color ( color_id,name)" );
db.makeTable( "create table fruit_color ( fruit_id,color_id )" );

db.do( "insert into fruit (fruit_id,name) values (1,'apple'),(2,'orange'),(3,'banana')" );
db.do( "insert into color (color_id,name) values (1,'red'),(2,'orange'),(3,'yellow')" );
db.do( "insert into fruit_color (fruit_id,color_id) values (1,1),(2,2),(3,3)" );

console.log( db.do( "select fruit.*,color.* from fruit join fruit_color on fruit_color.fruit_id=fruit.fruit_id join color on fruit_color.color_id=color.color_id" ) );
console.log( db.do( "select fruit.name as fruit,color.name as color from fruit join fruit_color on fruit_color.fruit_id=fruit.fruit_id join color on fruit_color.color_id=color.color_id" ) );
