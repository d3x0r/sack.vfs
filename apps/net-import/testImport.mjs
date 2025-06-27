
import {netImport} from "sack.vfs/net-import"

const file = netImport( "http://localhost:4334/test.js" );

const file2 = netImport( "test.js", "http://localhost:4334/" );

console.log( "files?", await file, await file2 );