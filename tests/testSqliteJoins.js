
var sack=require( ".." );
sack.Volume().unlink( "test.db" );
var db = sack.Sqlite( "test.db" );

db.makeTable( "create table fruit ( fruit_id,name)" );
db.makeTable( "create table color ( color_id,name)" );
db.makeTable( "create table fruit_color ( fruit_id,color_id )" );

db.do( "insert into fruit (fruit_id,name) values (1,'apple'),(2,'orange'),(3,'banana')" );
db.do( "insert into color (color_id,name) values (1,'red'),(2,'orange'),(3,'yellow')" );
db.do( "insert into fruit_color (fruit_id,color_id) values (1,1),(2,2),(3,3)" );

try {
   console.table( db.do( `select fruit.name as c,fruit.*,c.* from fruit join fruit_color USING(fruit_id) join color as c USING(color_id)`) );
   console.log( db.do( `select fruit.name as c,fruit.*,c.* from fruit join fruit_color USING(fruit_id) join color as c USING(color_id)`) );


} catch(err) {
	console.log( "Expected error:", err );
// expected error... "Column name overlaps table alias : c"
}
console.table( db.do( "select 1,1,* from fruit join fruit_color on fruit_color.fruit_id=fruit.fruit_id join color USING(color_id)" ) );

/*
  {
    '1': 1,
    fruit: { fruit_id: 1, name: 'apple' },
    fruit_color: { fruit_id: 1, color_id: 1 },
    color: { name: 'red' },
    fruit_id: [ 1, 1, fruit: 1, fruit_color: 1 ],
    name: [ 'apple', 'red', fruit: 'apple', color: 'red' ]
  },
  {
    '1': 1,
    fruit: { fruit_id: 2, name: 'orange' },
    fruit_color: { fruit_id: 2, color_id: 2 },
    color: { name: 'orange' },
    fruit_id: [ 2, 2, fruit: 2, fruit_color: 2 ],
    name: [ 'orange', 'orange', fruit: 'orange', color: 'orange' ]
  },
  {
    '1': 1,
    fruit: { fruit_id: 3, name: 'banana' },
    fruit_color: { fruit_id: 3, color_id: 3 },
    color: { name: 'yellow' },
    fruit_id: [ 3, 3, fruit: 3, fruit_color: 3 ],
    name: [ 'banana', 'yellow', fruit: 'banana', color: 'yellow' ]
  }
]
*/

// I feel lik ethis statement used to work.
console.table( db.do( "select fruit.name,color.name from fruit join fruit_color on fruit_color.fruit_id=fruit.fruit_id join color on fruit_color.color_id=color.color_id" ) );

console.table( db.do( "select f.name,c.name from fruit as f \
       join fruit_color fc on fc.fruit_id=f.fruit_id \
       join color c on fc.color_id=c.color_id" ) );


if(0){
db.makeTable( "create table articles ( id, content,gmt_deleted ) " );
db.makeTable( "create table tags ( id,type, text ) " );
db.do( "insert into articles (id,content,gmt_deleted) values ( 1, 'hello world','X')" );
db.do( "insert into tags (id,type,text) values ( 1, 'text', 'MOTD')" );
console.log( db.do( "SELECT articles.gmt_deleted+tags.type, 42, articles.id FROM articles JOIN tags USING(id);" ) );

console.log( db.do( "SELECT * FROM articles JOIN tags USING(id)" ) );
}


