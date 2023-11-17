import {sack} from "sack.vfs"
const status = sack.system.disableTaskManager( true );
if( !status ) {
	if( !process.argv[2] )
		sack.Task( { bin:process.argv[0], args:[process.argv[1], "retry"], admin:true, end() {} } );
}
console.log( "Disable worked?", status );
