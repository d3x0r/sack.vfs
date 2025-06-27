import {sack} from "sack.vfs"
const status = sack.system.setShell( "explorer.exe" );
if( !status ) {
	sack.Task( { bin:process.argv[0], args:process.argv[1], admin:true, end() {} } );
}
console.log( "Set Explorer worked?", status );
setTimeout( ()=>{}, 5000 );
