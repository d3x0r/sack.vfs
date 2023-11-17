import {sack} from "sack.vfs"
const status = sack.system.disableTaskManager( true );
console.log( "Disable worked?", status );
