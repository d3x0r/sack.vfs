
import child_process from "node:child_process";

let config = ""
let target = "vfs";

if( process.argv[3] === "gui") 
	target = "gui";

if( process.argv[2] === "reldeb") 
	config = "reldeb-";
else if( process.argv[2] === "debug" )
	config = "debug-";
else if( process.argv[2] === "release" )
	config = "";

const platform = process.platform;

switch( platform ) {
case "win32":
	const proc = child_process.execSync( "npm run build-"+target+"-"+config+"config-windows", {}, (error, stdout, stderr)=>{
        		if( error ) {
                        	console.log( error );
                        	process.exit( 1 );
                        } else {
                        	if( proc.exitCode )
	                        	process.exit( proc.exitCode );
                                //const proc = child_process.exec( "npm run build-"+target+"-run1", {}, (err,stdout, stderr)=>{
                                //	return proc.exitCode;
                                //} );
                        }
        	});        
	break;
case "linux":
	child_process.execSync( "npm run build-"+target+"-"+config+"config" );
	console.log( "configure happened?");
	break;
default:
	console.error( "Platform not handled to build:", platform );
        process.exit(1);
        break;
}


