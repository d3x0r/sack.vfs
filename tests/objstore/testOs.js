
const orgRoot = "org.example.domain"
const serviceRoot = "data";
const dbRoot = "users";

const appIdentifier = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";



var sack = require( "../.." );
//sack.Volume().unlink( "container.vfs" );

var vfs = sack.Volume( "cmount", "container.vfs" );
//console.log( "sack:", sack );

var store = sack.ObjectStorage( "cmount@storage.os" );
console.log( "Store:", store );


store.get(  { //`${orgRoot}.${serviceRoot}`, 
		objectHash : `${orgRoot}.${serviceRoot}.${dbRoot}`,

		sealant : null,
		readKey : null,

		then(node){
			// this is the managmeent container of node.  
			console.log( "Recovered root somehow:", node );
			root = node;
		},
		catch() {
			if( vfs.exists( "keyinfo" ) )
				vfs.readJSOX( "keyInfo", (ki)=>keyInfo = ki );
			else	
				vfs.write( "keyInfo", sack.JSOX.stringify( keyInfo ) );		        
			console.log( "catch, put record so we can get its object identifier.");

			store.put( userIndex, {
				objectHash:`${orgRoot}.${serviceRoot}.${dbRoot}`,
				sealant: myhashWrite,
				//readKey: myhashRead,
				stored(id){ 
					
					rootState.keyFile = this; 
					root.key = id; 
					this.put();
				},
				failed() { 
					 console.log( "Failed to store root into object storage." );
				},
				
			} );
		}
	}
);


function getSomeUsers( f, l, n, gotUsers ){
	userIndex.get( { where: { firstName: f, lastName : l },
			 orderBy: null,
			 limit : n,
			then(users) {
			},
			catch( err) {
			}
	               } );
}

function getSomeUsers2( f, l, n, gotUsers ){
	userIndex.map( { where: { firstName: f, lastName : l },
			 orderBy: null,
			 limit : n,
	               } );

}

function storeUser( user ){
	userIndex.put( user );
}



/////var userInfo = store.CreateFactory( [ "firstName", "lastName", "birthday" ] );

var rootKey;

var root = { key : null };
var rootState = { keyFile : null };



function initRoot( cb ) {

	var storeRoot;

	var keyInfo = {
		 password : appIdentifier,
	}
	var myhashWrite = sack.SaltyRNG.id(keyInfo.password+"write", 4);
	var myhashRead  = sack.SaltyRNG.id(keyInfo.password+"read", 4);
	

	store.get( { //`${orgRoot}.${serviceRoot}`, 
		objectHash : `${orgRoot}.${serviceRoot}`,
		sealant : myhashWrite,
		//readKey : null,
		then(node){
			// this is the managmeent container of node.  
			root = node;
		},
		catch() {
			if( vfs.exists( "keyinfo" ) )
				vfs.readJSOX( "keyInfo", (ki)=>keyInfo = ki );
			else	
				vfs.write( "keyInfo", sack.JSOX.stringify( keyInfo ) );		        
			
			store.put( root, {
				objectHash:`${orgRoot}.${serviceRoot}`,
				sealant: myhashWrite,
				//readKey: myhashRead,
				stored(id){ 
					rootState.keyFile = this; 
					root.key = id; 
					this.put();
				},
				failed() { 
					 console.log( "Failed to store root into object storage." );
				},
				
			} );
		}	
	} );


	root.unsigned = [];
	root.signed = [];

	vfs.write( "root", sack.JSOX.stringify( root ) );
        
	addMoreNodes();
}

if( !vfs.exists( "root" ) ) {
	console.log( "virtual disk does not yet have 'root'" );
	initRoot();
} else
	vfs.readJSOX( "root", (newRoot)=>{
		rootKey = newRoot.key;
		if( !rootKey ) {
			initRoot();
		} else {
			store.map( rootKey, {
				then(obj){
					root = obj; 
					console.log( "new root:", sack.JSOX.stringify(root,null,3) );
					addMoreNodes();
				} 
			});
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
		stored(id){
			console.log( "(from store)GOT storage ID:", id );
		},
		failed() {
			
		},
	} );

	console.log( "THIS SHOULD REWRITE ROOT:", sack.JSOX.stringify( root,null,3) );
	store.put( root );

}



//store.get( 
var userdb = {
	users : null //userIndex
};

