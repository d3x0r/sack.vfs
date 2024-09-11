import {sack} from "sack.vfs"

sack.Task( { bin:"cmd.exe", args:[ "/c", "echo done && pause" ], admin:true, noKill:true
		} );
