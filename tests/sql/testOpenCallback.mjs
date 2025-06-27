import {sack} from "sack.vfs"

const db = sack.Sqlite( "maria", (db)=>{
	console.log( "db got opened:", db );
	db.required = true;
	const tst = db.do( "Select 1+1" );
	console.log( "open issued:", tst );
} );

