
import {sack} from "sack.vfs"

//const db = sack.DB( "Test.db" );
const db = sack.DB( "maria-jim" );

import dbUtil from "sack.vfs/dbUtil";
console.log( "dbUtil:", dbUtil );

db.makeTable( "create table test(id int PRIMARY KEY AUTO_INCREMENT, data char(32)" );

const table = dbUtil.MySQL.loadSchema( db, "test" );
console.log( "Schema:", table );


function select1() {
	const r = db.do( "select 1+1" );
	console.log( "got:", r );
}

function loop2() {
	const r = db.do( "select * from test where data=?", "no match" );
	console.log( "loop2 got:", r );
}

function loop(n) {
	if( n > 10 ) {
		loop2();
		return;
	}
	select1();
	return loop(n+1);
}

loop(0);

run_wait(0);

function run_wait() {
	setTimeout( ()=>{}, 5000 );
}