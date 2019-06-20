
function test () {
var vfs = require( ".." )( (vfs)=>{
//console.log( "keys: ", Object.keys( vfs ) );
var more = require( "./testaux.js" );

var vol0 = vfs.Volume();
vol0.mkdir( "test/make/path" );

var vol = null;
try {
    vol = vfs.Volume( "mount-name", "./data.vfs" );
} catch( err ) { console.log( err ) } ;
if( vol ) {
	var volDb = vol.Sqlite( "option-internal.db" );
	var rows = volDb.do( "select * from sqlite_master" );
	console.log( "read db in vfs?" );


    console.log( "read file1" );
    file = vol.File( "default file" );
    //file.on( "close", ()=>{console.log( "closed." );} );
    console.log( "read file2" );
    file2 = vol.File( "Another file" );
    //file.on( "close", ()=>{console.log( "closed." );} );

    console.log( "directory:", vol.dir() );

//console.log( "vol prototype", Object.getPrototypeOf( vol ) );

// this shows the methods of the original file.
//console.log( "file prototype", Object.keys(Object.getPrototypeOf( file )) );
// this shows the event emitter methods...
//console.log( "file prototype", Object.keys(file) );

var db = vfs.Sqlite( "$sack@mount-name$chatment.db" );
//var db = vfs.Sqlite( "chatment.db" );
db.makeTable( "create table if not exists tmp (id int);" );
db.do( "insert into tmp (id) values (1),(2),(3),(4)" );

first = db.do( "select * from tmp" );
console.log( first );

var test = db.do( "select sha1( 'test' ) a,encrypt( 'encode this' ) b" );
console.log( test );
var val = test[0].b;
var test = db.do( `select decrypt( '${val}' )d` );
console.log( test );
setTimeout( ()=>{ console.log( "keep database..." ); vfs.Sqlite.so( "get option", "defaultVal" ); db.do( "select 1" ); }, 100 );

}


//db = null;
vol = null;
file = null;
//file2 = null;

// need to run with --expose-gc
//global.gc();


//setTimeout( ()=>{ file2 = null; global.gc(); console.log( "ticked" );}, 500 );
//setTimeout( ()=>{ global.gc();  console.log( "ticked" );}, 1000 );
//setTimeout( ()=>{ global.gc();  console.log( "ticked" );}, 1500 );
});

 }

 
// setTimeout( test, 1000 );
//setTimeout( ()=>{console.log( "was that long enough?")}, 20000 );
 test();
 
