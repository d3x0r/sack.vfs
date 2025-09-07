

import {sack} from "sack.vfs"

const open = [];
const ticks = [];

sack.ComPort.onRemove = (port)=>{
	console.log( "onRemove port:", port );
	const oldPort = open.find( p=>p.port===port );
	for( let p = 0; p < open.length; p++ ) {
		const openPort = open[p];
		if( openPort.port === port ) {
			console.log( "An open port has gone away" );
			open.splice( p, 1 );
		}
	}
}
sack.ComPort.onAdd = (port)=>{
	console.log( "onAdd port:", port );

	console.log( "Reopening port" );
	open.push( {port:port
		, handle:sack.ComPort( port, (buf)=>{ lprintf( "Buffer?", buf ) } ) } );	
	clearTick( port );
}

const ports = sack.ComPort.ports;

console.log( "disabling port..." );
for( let port of ports ) {
	sack.ComPort.disable( port.port );
//	const tick = {port, timer:setTimeout( ()=>autoEnable(port), 1000 ) };
	ticks.push( {port, timer:setTimeout( ()=>autoEnable(port), 1000 ) } );
}

function autoEnable(port) {
	for( let p = 0; p < ticks.length; p++ ) {
		const tick = ticks[p];
		if( tick.port.port === port.port ) {
			sack.ComPort.enable( port.port );
			ticks.splice( p, 1 );
		}
	}
}

function clearTick( port ) {
	for( let p = 0; p < ticks.length; p++ ) {
		const tick = ticks[p];
		if( tick.port.port === port.port ) {
			ticks.splice( p, 1 );
		}
	}

}

//console.log( "ports:", ports );
if(0)
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

