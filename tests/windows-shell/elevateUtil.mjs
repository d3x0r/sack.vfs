import {sack} from "sack.vfs"
if( !sack.system.isElevated() ) {
	const args =  process.argv.slice(2).map( arg=>(arg=arg.replaceAll( "\\", "\\\\" ), arg.includes(' ' )?'""'+arg+'""':arg) ).join(' ') ;
	const script = `Set UAC = CreateObject("Shell.Application")\nUAC.ShellExecute "${process.argv[0].replaceAll( "\\", "\\\\") }", "${args}", "", "runas", 1`
	sack.Volume().write( process.env.TEMP + "/OEgetPriv.vbs", script );
	sack.Task( { bin:"WScript.exe" , args: [process.env.TEMP + "/OEgetPriv.vbs"], end() { process.exit(0)} } );
}