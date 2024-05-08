#!node

import {sack} from "sack.vfs" 
import os from "node:os";

const disk = sack.Volume();
const procs = sack.Task.getProcessList( process.argv[2] );
for( let proc of procs ) {
	if( os.platform === "linux" ){
		const threads = disk.dir( `/proc/${proc.id}/task` );
		const threadids = threads.reduce( (acc,thread)=>{if( thread.name[0] !== '.') acc.push( thread.name); return acc} , [] );
		console.log( proc.id.toString().padStart( 8, ' ' ), proc.bin, proc.args.join(' ' ), "\n", "\t", threadids.join(',' ) );
        }
        else
        	console.log( proc.id.toString().padStart( 8, ' ' ), proc.bin, proc.args.join(' ' ) );

}

