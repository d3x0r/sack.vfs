
import {import as netImport} from "./net-import.mjs"

const file = netImport( "http://localhost:4334/test.js" );

const file2 = netImport( "test.js", "http://localhost:4334/" );

console.log( "files?", await file, await file2 );