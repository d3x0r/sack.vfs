
var sack = require( "../.." );
//sack.Volume().unlink( "container.vfs" );

var vfs = sack.Volume( "cmount", "container.vfs" );
//console.log( "sack:", sack );

var store = sack.ObjectStorage( "cmount@storage.os" );


var rootKey;

var root = { key : null };

function initRoot() {
	root.key = rootKey = store.put( root );
	root.unsigned = [];
	root.signed = [];

	vfs.write( "root", sack.JSOX.stringify( root ) );
	addMoreNodes();
}

if( !vfs.exists( "root" ) ) {
	initRoot();
}else
vfs.readJSOX( "root", (newRoot)=>{
	rootKey = newRoot.key;
	if( !rootKey ) {
		initRoot();
	} else {
		store.map( rootKey ).then( (obj)=>{
			root = obj; 
			console.log( "new root:", sack.JSOX.stringify(root,null,3) );
			addMoreNodes();
		} );
		//console.log( "did get? " );
	}
	console.log( "Root Key:" , rootKey );
} );


//vfs.unlink( "storage.os" );

function addMoreNodes() {

console.log( "Root is now:", root );
var obj;
root.unsigned.push( obj = { hello:"world", root:root} );
var id = store.put( obj );
console.log( "GOT storage ID:", id );
root.signed.push( obj = { hello:"world", root:root} );
var id = store.put( obj, true );
console.log( "GOT storage ID:", id );

console.log( "THIS SHOULD REWRITE ROOT:", sack.JSOX.stringify( root,null,3) );
store.put( root );

}