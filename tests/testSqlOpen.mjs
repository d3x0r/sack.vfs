
import {sack} from "sack.vfs"

const db = sack.Sqlite( "MySQL2" );
console.log( "Opened DB:", db, db.provider );
const db2 = sack.Sqlite( "psql" );
console.log( "Opened DB:", db2, db2.provider );
