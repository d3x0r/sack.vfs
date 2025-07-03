import {sack} from "sack.vfs"

let interfaces = sack.WIFI.interfaces;

let autoConnect = false;


sack.Systray.set(  !interfaces.find( i=>i.status !== "connected" )?"../gui/pfull.ico":"../gui/pdown.ico", ()=>{
	// only saves the first function
	interfaces.forEach( (i,idx)=> (i.status=== "disconnected")&&sack.WIFI.connect( idx, "MobleyPlace", "MobleyPlace" ) );
	
} );

sack.Systray.on( "Connect All", ( )=>{
	console.log( "Connect all(except connected)" );
	for( let i = 0; i < interfaces.length; i++ ) {
		if( interfaces[i].status !== "connected" )
			sack.WIFI.connect( i, "MobleyPlace", "MobleyPlace" );
	}
} );

sack.Systray.on( "Close All Connections", ( )=>{
	console.log( "Disconnect all wifi" );
	for( let i = 0; i < interfaces.length; i++ ) sack.WIFI.disconnect( i );
} );



const imap = new Map();

if( interfaces.length > 0 )  {
	console.log( "names:", sack.WIFI.interfaces.map( i=>i.name ).join(', ') );
} else {
	console.log( "There are currently no wifi devices." );
}

sack.WIFI.onEvent( (event)=>{
	if( [ 1 /* autoconf enable */
	    , 2 /* autoconf disable */
	    , 3 /* background scan enable */
	    , 4 /* background scan disable */
	    , 26/*scan list refresh*/
	    , 7 /*scan complete - values always 1 1?*/
	    , 15 /* profile change */
	  //  , 23 /* profile unblocked */
	    , 5 /* BSS Type Change */
		, 23 /* profile unblock - gets a Profile and SSID */
	    , 8 /*scanfail*/].includes( event.code ) ) {
		//console.log( "Skipping event:", event );
		return;
	}
	if( event.code === 27 ) {
		if( event.mode === 1 ) {
			console.log( "Turn Off" );			
			return
		}
		if( event.mode === 2 ) {
			console.log( "Turn On" );
			return
		}
		if( event.mode === 3 ) {
			console.log( "Turning Off" );			
			return
		}
		if( event.mode === 4 ) {
			console.log( "Turning On" );
			return
		}
		// operational state change (power off/on)
	} else if( event.code === 13 ) {
		// interface arrival (enable in control panel)
		interfaces = sack.WIFI.interfaces
		console.log( "during arrival... Interfaces?", interfaces );
		return;
	} else if( event.code === 14 ) {
		// interface removal (disable in control panel)
		interfaces = sack.WIFI.interfaces
		console.log( "during remove ... Interfaces?", interfaces );
		return;
	} /*else if( event.code === 7 ) {
		// this isn't really useful....
		console.log( "Scan complete state:", event.index, event.v1, event.v2 );
		return;
	} */ else if( event.code === 23 ) {
		console.log( "Profile Unblock:", event.name );
		return;

	} else if( event.code === 9 ) {
		// connection start
		interfaces[event.interface].state = 7;
		interfaces[event.interface].status = "connecting";
		return;
	}
	else if( event.code === 10 ) {
		interfaces[event.interface].state = 1;
		interfaces[event.interface].status = "connected";
		if( !interfaces.find( i=>i.status !== "connected" ) )
			sack.Systray.set( "../gui/pfull.ico" );
		return;
		
	} else if( event.code === 20 ) {
		interfaces[event.interface].state = 3;
		interfaces[event.interface].status = "disconnecting";
		console.log( "no disconnecting?" );
		sack.Systray.set( "../gui/pdown.ico" );
		return;
	} else if( event.code === 21 ) {
		interfaces[event.interface].state = 4;
		interfaces[event.interface].status = "disconnected";
		// power off wifi ges back an error
		sack.Systray.set( "../gui/pdown.ico" );
		//console.log( "This is disconnected, we can now connect...(connecting.)" );
		if( autoConnect ) {
			const x = sack.WIFI.connect( event.interface, "MobleyPlace", "MobleyPlace" );
			if( x )
				console.log( "Connect error at disconnect event:", x );
		}
		return;
	}

	console.log( "Wifi Event:", event, interfaces[event.interface] );
//	if( event === 
} );

let lastStatus = interfaces.map( i=>"" ) || [];
function tick() {
	const itf = sack.WIFI.interfaces;
	for( let i = 0; i < itf.length; i++ ) {
		const int = itf[i];
		//console.log( "laststatus:", int.status, lastStatus );
		if( int.status != lastStatus[i] ) {
			//console.log( "int:", int );
			console.log( (new Date()).toLocaleString(), int.name,":", int.status );
		}
		if( int.status != "connected" && int.status != "disconnected" ) {
			setTimeout( tick, 10 );
			return lastStatus[i] = int.status;
		} else {
			if( int.status === "disconnected" ) {
				if( lastStatus[i] !== "disconnected" ) {
					console.log( "This is first polled connect, this could connect" );
					//const x = sack.WIFI.connect( event.interface, "MobleyPlace", "MobleyPlace" );
					
				}
			}
		}
		lastStatus[i] = int.status;
	}
	//console.log( "Tick." );
	setTimeout( tick, 1000 );
}

tick();
