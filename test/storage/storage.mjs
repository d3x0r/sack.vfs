'use strict';
import {sack} from "sack.vfs";
sack.Sqlite.so( "SACK/filesys/Log open and close", 0 );
sack.Volume().unlink( "storage.os" );
sack.Volume().unlink( "storage2.os" );
const db = {
	storage : sack.ObjectStorage( "storage.os" ),
	storage2 : sack.ObjectStorage( "storage2.os" ),
	root : null,
	root2 : null,
	init() {
		return this.storage.getRoot().then( root=>{
			return this.storage2.getRoot().then( root2=>{
				this.root = root 
				this.root2 = root2 
			} );
		} );
	}
}

db.init().then( ()=>{
	const start = Date.now();
	for( var n = 0; n < 1000; n++ ) {
      ((n)=>{
			db.root.create( "TestFile" + n ).then( (file)=>{
							//console.log( "write:", n );
   	             	return file.write( "Test Content:" + n ).then( ()=>{
						if( n === 999 ) {
							const now = Date.now();
							console.log( "Done:", now - start );
						}	
							} );
      	          } ).catch( (err)=>{
         	       	console.log( 'file name exists already?' );
	           	    } );
		})(n);
	}
	const now = Date.now();
	console.log( "Created", n, "files in", now-start, "ms" );
});

db.init().then( ()=>{
	const start = Date.now();
	
	function createOne( n ) {
		if( n >= 1000 ) return;
		return db.root2.create( "Testfile" + n ).then( (file)=>{
							//console.log( "write:", n );
   	             	return file.write( "Test Content:" + n ).then( ()=>createOne(n+1) );
      	          } ).catch( (err)=>{
         	       	console.log( 'file name exists already?' );
	           	    } );
	}
	return createOne( 0 ).then( ()=>{
		const now = Date.now();
		console.log( "Created", 1000, "files in", now-start, "ms" );
	} );
} )