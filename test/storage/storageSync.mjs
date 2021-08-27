'use strict';
import {sack} from "sack.vfs";
sack.Sqlite.so( "SACK/filesys/Log open and close", 0 );
sack.Volume().unlink( "storage.os" );
sack.Volume().unlink( "storage2.os" );

const db = {
	storage : sack.ObjectStorageSync( "storage.os" ),
	storage2 : sack.ObjectStorageSync( "storage2.os" ),
	root : null,
	root2 : null,
	init() {
		if( !this.root ) {
			this.root = this.storage.getRoot();
			this.root2 = this.storage2.getRoot();
//			console.log( "this:", this );
		}
		return { then( cb ) { cb() } }
	}
}

db.init().then( ()=>{
	console.log( "Starting test1" );
	const start = Date.now();
	for( var n = 0; n < 1000; n++ ) {
		const file = db.root.create( "TestFile" + n )
   	file.write( "Test Content:" + n )
					if( n === 999 ) {
						const now = Date.now();
						console.log( "Done:", now - start );
					}	
	}
	const now = Date.now();
	console.log( "Created", n, "files in", now-start, "ms" );
});

db.init().then( ()=>{
	console.log( "Starting test2" );
//	Process.exit(0);
	const start = Date.now();
	function createOne( n ) {
		if( n >= 1000 ) return;
		const file = db.root2.create( "Testfile" + n )
		file.write( "Test Content:" + n )
		createOne(n+1);
	}
	createOne(0);
	const now = Date.now();
	console.log( "Created", 1000, "files in", now-start, "ms" );
} )
