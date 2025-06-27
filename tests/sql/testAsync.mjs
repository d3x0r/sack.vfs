
import {sack} from "sack.vfs"
import dbUtil from "../../apps/dbUtil/db.mjs"

const db = sack.DB( "Test.db" );

db.makeTable( "create table test(id int PRIMARY KEY AUTO_INCREMENT, data char(32)" );
const table = dbUtil.MySQL.getSchema( "table" );
console.log( "Schema:", table );


function select1() {
	return db.run( "select 1+1" );
}

function loop2(n) {
	db.run( "select * from test where data=?", "no match" ).then( data=>{
		console.log( "Got:", data ) 
		loop3();
	});
}

function loop3(n) {
	db.run( "select * from test33 where data=?", "no match" ).then( data=>console.log( "Got:", data ) );
}

function loop(n) {
	if( n > 1000 ) {
		loop2(0);
		//db.close();
		return;
	}
	console.log( "tick" );
	select1().then( ()=>loop(n+1));
}

loop(0);
