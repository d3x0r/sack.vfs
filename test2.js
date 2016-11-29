
var vfs = require( "./vfs_module.js" )
console.log( "Got", vfs );
var vol = vfs.Volume( "mount-name", "./data.vfs" );
var vol2 = vfs.Volume( "mount-name2", "./data2.vfs", "some key text", "Some other Key" );

console.log( "..." );
var sql = vfs.Sqlite( "$sack@mount-name$app.db" );
console.log( "..." );
var sql3 = vfs.Sqlite( "$sack@mount-name2$app.db" );

var sql2 = vfs.Sqlite( "local.db" );

var val = sql2.op( "oldValue" );
console.log( "option val = ", val );

var val = sql.op( "oldValue" + new Date() );
console.log( "option val = ", val );


var val = sql3.op( "oldValue" + new Date() );
console.log( "option val = ", val );

sql3.setOption( "newValue" + new Date() );
