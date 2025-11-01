

import {sack} from "sack.vfs"

const open = [];
const ticks = [];

sack.ComPort.onRemove = (port)=>{
	//console.log( "onRemove port:", port );
	const oldPort = open.find( p=>p.port===port );
	for( let p = 0; p < open.length; p++ ) {
		const openPort = open[p];
		if( openPort.port === port ) {
			console.log( "An open port has gone away" );
			openPort.handle.close();
			open.splice( p, 1 );
		}
	}
}
sack.ComPort.onAdd = (port)=>{
	//console.log( "onAdd port:", port );

	setTimeout( ()=>{
		console.log( "Reopening port", port );
		open.push( {port:port
		, handle:sack.ComPort( port, (buf)=>{ lprintf( "Buffer?", buf ) } ) } );	
		}, 50 );
	clearTick( port, false );
}

const ports = sack.ComPort.ports;

ports.forEach( port=>{
	console.log( "disabling port...", port.port );
	sack.ComPort.disable( port.port );
	//	const tick = {port, timer:setTimeout( ()=>autoEnable(port), 1000 ) };
	console.log( "Added a port timeout for port too..." );
	ticks.push( {port, timer:setTimeout( ()=>{
		autoEnable(port);
		// the auto enable needs a tick in ticks... 
		clearTick( port.port, true );
	}, 1000 ) } );
})

function autoEnable(port) {
	console.log( "timed out waiting for event, triggering port enable", port.port );
	for( let p = 0; p < ticks.length; p++ ) {
		const tick = ticks[p];
		console.log( "checking:", tick.port.port, port.port );
		if( tick.port.port === port.port ) {
			console.log( "enabling:", port.port );
			sack.ComPort.enable( port.port );
			ticks.splice( p, 1 );
			break;
		}
	}
}

function clearTick( port, isTimer ) {
	for( let p = 0; p < ticks.length; p++ ) {
		const tick = ticks[p];
		if( !isTimer )
			clearTimeout( tick.timer );
		//console.trace( "Finding tick to clear:", port, ticks );
		if( tick.port.port === port ) {
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

