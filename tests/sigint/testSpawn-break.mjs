
import {sack} from "sack.vfs"

const task = sack.Task( {bin:"node"
		, args:["testSpawnChild.mjs"]
		, newGroup : true
		, useBreak : true
		, noKill : true
		, noWait : false
		, aend() {
			// maybe noKill:true
			console.log( "waited for child to exit" );
		}
                } );

