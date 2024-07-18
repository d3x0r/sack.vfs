
import child_process from "node:child_process";
import os from "os";

const platform = os.platform;

switch( platform ) {
case "win32":
	const proc = child_process.exec( "npm run build-vfs-config-windows", {}, (error, stdout, stderr)=>{
        		if( error ) {
                        	console.log( error );
                        	process.exit( 1 );
                        } else {
                        	if( proc.exitCode )
	                        	process.exit( proc.exitCode );
                                const proc = child_process.exec( "npm run build-vfs-run1", {}, (err,stdout, stderr)=>{
                                	return proc.exitCode;
                                } );
                        }
        	});        
	break;
case "linux":
	child_process.exec( "npm run build-vfs-config" );
	break;
default:
	console.error( "Platform not handled to build:", platform );
        process.exit(1);
        break;
}


