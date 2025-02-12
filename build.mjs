
import child_process from "node:child_process";

import fs from "node:fs";

const   pkg = JSON.parse( fs.readFileSync( "./package.json", {encoding:"utf8"} ) );
		//await import( "./package.json" , {assert:{ type: 'json' }} );

let config = "Release"
let target = (pkg.name === "@d3x0r/sack-gui")?"gui":"vfs";
let GUI = (pkg.name === "@d3x0r/sack-gui")?1:0;
let skipConfigure = false;
let arg = 2;
const moreopts = [];


for( arg = 2; arg < process.argv.length; arg++ ) {

	if( process.argv[arg] === "reldeb") 
		config = "RelWithDebInfo";
	else if( process.argv[arg] === "debug" )
		config = "Debug";
	else if( process.argv[arg] === "release" )
		config = "Release";
	else if( process.argv[arg] === "gui")  {
		target = "gui";
		GUI = 1;
	} else if( process.argv[arg] === "nwjs") 
		moreopts.push( "--CDHOST_NWJS=1" );
	else if( process.argv[arg] === "build" ) {
        	skipConfigure = true;
        }
}


const platform = process.platform;

if( !skipConfigure )

switch( platform ) {
case "win32":
	{
	const proc = child_process.spawn( "npx"
			, ["cmake-js", "-t", "ClangCL", "--CDMAKE_GUI="+GUI
			  , moreopts.join(' ')
			  , "--config", config, "configure"]
			, {stdio:"pipe", shell:true}
		);
		proc.stdout.pipe( process.stdout );
		proc.stderr.pipe( process.stderr );
		proc.on( "exit", (a,b)=>{
			if( a ) process.exit(a);
			else runBuild();
		} );
        	//runBuild();
	}
	break;
case "linux":
	//child_process.execSync( "npm run build-"+target+"-"+config+"config" );
	{
	const proc = child_process.spawn( `npx cmake-js --CDMAKE_GUI=${GUI} ${moreopts.join(' ')} --config ${config} configure`
			, {shell:true, stdio:"pipe"}
		);
	proc.stdout.pipe( process.stdout );
	proc.stderr.pipe( process.stderr );
	proc.on( "exit", (a,b)=>{
			if( a ) process.exit(a);
			else runBuild();
		console.log( "build exitged?" );
	} );

	}
	break;
default:
	console.error( "Platform not handled to build:", platform );
        process.exit(1);
        break;
}

else
	runBuild();

function runBuild() {
	const proc = child_process.spawn( `npx cmake-js build`, ["--config", config]
			, {shell:true, stdio:"pipe"}
			);
	proc.stdout.pipe( process.stdout );
	proc.stderr.pipe( process.stderr );
	proc.on( "exit", (a,b)=>{
		if( a ) process.exit(a);
	} );


}
