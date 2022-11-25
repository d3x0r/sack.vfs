
var sack = require( ".." );

sack.File.unlink( "test.db" );

var db = sack.Sqlite( "test.db" );

//db.do( ".headers on
db.do( "pragma full_column_names=1;" );
db.do( "pragma short_column_names=0;" );



db.do( "create table addresses( id, name );" );
db.do( "create table users( id, address_id, name );" );
db.do( "create table pets(id, user_id, name );" );

db.do( 'insert into addresses(id,name) values ( 1, "there" );' );
db.do( 'insert into users(id,address_id,name) values ( 1, 1, "bob" );' );
db.do( 'insert into pets(id,user_id,name) values ( 1,1, "odif" );' );

console.log( db.do( "select * from users user join addresses address on address.id=user.address_id join pets pet on pet.user_id=user.id;" ) );
console.log( db.do( "select count(*)count,* from ( select count(*)count,* from addresses join users on users.address_id=addresses.id ) address " ) );//address join pets pet on pet.user_id=address.id_1;" ) );
//console.log( db.do( "select count(*)count,* from ( select count(*)count,* from addresses join users on users.address_id=addresses.id ) address join pets pet on pet.user_id=address.'id:1';" ) );
console.log( db.do( "select count(*)count,* from ( select count(*)count,* from addresses join users on users.address_id=addresses.id ) address join pets pet on pet.user_id=`address.user.id`;" ) );

