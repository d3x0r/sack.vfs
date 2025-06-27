

import {sack} from "sack.vfs";


const db = sack.DB( "maria-jim" );
console.log( "db?", db );

db.makeTable( "Create table test( id int auto_increment PRIMARY KEY, data int )" );

const waits = [];
for( let n = 0; n < 50; n++ )
	waits.push( db.run( "insert into test (data)values("+n+")" ) );

const inserts = await Promise.all( waits );

const rows = await db.run( "select count(*) from test" );
console.log( "rows:", rows );

db.do( "drop table test" );
db.close();
