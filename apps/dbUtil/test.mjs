
import {sack} from "sack.vfs";

import dbUtil from "sack.vfs/dbUtil";
console.log( "dbUtil:", dbUtil );

const db = sack.Sqlite( "mysql" );
const table = dbUtil.MySQL.loadSchema( db, "cards" );
console.log( "Table:", table, table.has("face_number") );

if( !table.has("face_number") ) {
	table.addColumn( "face_number", "bigint(20)" );		
}

