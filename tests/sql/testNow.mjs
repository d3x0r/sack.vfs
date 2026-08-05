import {sack} from "sack.vfs"
const db = sack.DB( "MySQL" );
console.log( db.do( "select now()" ) );