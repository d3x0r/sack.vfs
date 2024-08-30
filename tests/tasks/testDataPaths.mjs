import {sack} from "sack.vfs"

console.log( "Program:", sack.Task.programName );
console.log( "Program Path:", sack.Task.programPath );
console.log( "Program Data:", sack.Task.programDataPath );
console.log( "Common(global) Data:", sack.Task.commonDataPath );
console.log( "Module Path:", sack.Task.modulePath );
