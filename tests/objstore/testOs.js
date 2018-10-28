
var sack = require( "../.." );
sack.Volume().unlink( "container.vfs" );

var vfs = sack.Volume( "cmount", "container.vfs" );
//console.log( "sack:", sack );

var store = sack.objectStorage( "cmount@storage.os" );



var rootKey;

var root = { key : null };

vfs.readJSOX( "root", (root)=>{
	rootKey = root.key;
} );
if( !rootKey ) {
	var root;
	root.key = rootKey = store.put( root );
	
	vfs.write( "root", sack.JSOX.stringify( root ) );
}
console.log( "Root Key:" , rootKey );


//vfs.unlink( "storage.os" );



var id = store.put( { hello:"world", root:root} );
console.log( "GOT:", id );
var id = store.put( { hello:"world", root:root}, true );
console.log( "GOT:", id );
