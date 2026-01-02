import {sack} from "sack.vfs"
import {openServer} from "./server.mjs"

const servePort = Number(process.env.PORT) || (process.argv.length > 2?Number(process.argv[2]) : 8080);

{
	const ports = sack.Network.TCP.ports;
	for( let p of ports ) {
		if( p.port === servePort) {
			const task = sack.Task.getProcessList( p.pid );
			console.log( "process ", p.pid, "is already serving on port", servePort, task );
		}
	}
}


openServer( );
console.log( "serving on?", servePort );
