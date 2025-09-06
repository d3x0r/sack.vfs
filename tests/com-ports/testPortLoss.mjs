

import {sack} from "sack.vfs"

const open = [];

sack.ComPort.onRemove = (port)=>{
	console.log( "onRemove port:", port );
	const oldPort = open.find( p=>p.port===port );
}
sack.ComPort.onAdd = (port)=>{
	console.log( "onAdd port:", port );
	
}

const ports = sack.ComPort.ports;
console.log( "ports:", ports );

for( let port of ports ) {
	if( !open.find( p=>p===port.port ) ) {
		console.log( "Should be opening port:", port.port );
	try {
		open.push( {port:port.port, handle:sack.ComPort( port.port, (buf)=>{ lprintf( "Buffer?", buf ) } ) } );
		} catch(err) {
			console.log( "Failed to open port:", err );
		}
	}
}

