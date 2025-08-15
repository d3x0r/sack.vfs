
if( !process.argv[2] ) {
	console.log( "usage: %0 (?) <file>" );
        process.exit(1);
}

import {sack} from "sack.vfs"
const disk = new sack.Volume();

disk.File( process.argv[2] );
