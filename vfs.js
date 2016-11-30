
function test () {
//console.log( 'require', "./build/Release/vfs_module.js" )
var vfs = require( "./vfs_module.js" )
//var vfs = require( "./build/Release/vfs_module.node" )


console.log( "keys: ", Object.keys( vfs ) );
console.log( "vfs obj: ", vfs );

var vol = null;
vol = vfs.Volume( "mount-name", "./data.vfs" );//, "some key text", "Some other Key" )
file = vol.File( "default file" );
//file.on( "close", ()=>{console.log( "closed." );} );
file2 = vol.File( "Another file" );
//file.on( "close", ()=>{console.log( "closed." );} );

console.log( "directory:", vol.dir() );
console.log( "vol prototype", Object.getPrototypeOf( vol ) );

// this shows the methods of the original file.
console.log( "file prototype", Object.keys(Object.getPrototypeOf( file )) );
// this shows the event emitter methods...
console.log( "file prototype", Object.keys(file) );

var db = vfs.Sqlite( "$sack@mount-name$chatment.db" );
db.makeTable( "create table if not exists tmp (id int);" );
db.do( "insert into tmp (id) values (1),(2),(3),(4)" );

first = db.do( "select * from tmp" );
console.log( first );

var test = db.do( "select sha1( 'test' ) a,encrypt( 'encode this' ) b" );
console.log( test );
var val = test[0].b;
var test = db.do( `select decrypt( '${val}' )d` );
console.log( test );

db = null;
vol = null;
file = null;
//file2 = null;

// need to run with --expose-gc
//global.gc();


//setTimeout( ()=>{ file2 = null; global.gc(); console.log( "ticked" );}, 500 );
//setTimeout( ()=>{ global.gc();  console.log( "ticked" );}, 1000 );
//setTimeout( ()=>{ global.gc();  console.log( "ticked" );}, 1500 );

 }
 
// setTimeout( test, 1000 );
 test();
