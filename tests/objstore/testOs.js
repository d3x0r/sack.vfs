
const orgRoot = "org.example.domain"
const serviceRoot = "data";



var sack = require( "../.." );
//sack.Volume().unlink( "container.vfs" );

var vfs = sack.Volume( "cmount", "container.vfs" );
//console.log( "sack:", sack );

var store = sack.ObjectStorage( "cmount@storage.os" );


var rootKey;

var root = { key : null, keyFile : null };



function initRoot( cb ) {

	var storeRoot;


	store.get( `${orgRoot}.${serviceRoot}` ).then( function (node){
		// this is the managmeent container of node.  
		root = node;
	} ).catch( ()=>{
		var keyInfo = {
			 password : sack.generate(),
		}
		if( vfs.exists( "keyinfo" ) )
			vfs.readJSOX( "keyInfo", (ki)=>keyInfo = ki );
		else	
			vfs.write( "keyInfo", sack.JSOX.stringify( keyInfo ) );

		var myhashWrite = sack.regenerator(keyInfo.password+"write");
		var myhashRead  = sack.regenerator(keyInfo.password+"read");

		store.put( root, {
			objectHash:`${orgRoot}.${serviceRoot}`,
			sealant: myhash,
			//readKey: myhashRead,
			stored(id){ root.keyFile = this; root.key = id; },
			failed() { },
		} );
	} );


	root.unsigned = [];
	root.signed = [];

	vfs.write( "root", sack.JSOX.stringify( root ) );
	addMoreNodes();
}

if( !vfs.exists( "root" ) ) {
	initRoot();
} else
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
	
	store.put( obj, { 
		signed(id){
			console.log( "GOT storage ID:", id );
		},
		failed() {
			
		},
	} );

	console.log( "THIS SHOULD REWRITE ROOT:", sack.JSOX.stringify( root,null,3) );
	store.put( root );

}