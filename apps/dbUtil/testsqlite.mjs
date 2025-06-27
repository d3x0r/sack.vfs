
import {sack} from "sack.vfs";

import dbUtil from "sack.vfs/dbUtil";
console.log( "dbUtil:", dbUtil );

const db = sack.Sqlite( "sqlite.db" );

db.makeTable( "Create table table1 ( id int auto_increment, stuff int(11), other char(32) NOT NULL DEFAULT 'asdf', key char(32) NOT NULL )" );
db.makeTable( "Create table table2 ( id int auto_increment, stuff int(11), INDEX stuffkey(stuff) )" );

const table = dbUtil.Sqlite.loadSchema( db, "table1" );
console.log( "Table:", table, table.has("stuff") );

if( !table.has("face_number") ) {
	table.addColumn( "face_number", "int(11)" );		
}

