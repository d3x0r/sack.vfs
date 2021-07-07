const _debug_location = false;

const ws = this;
console.log( "Extend this websocket:", this );


  //const sack = (await Import( "sack.vfs" )).sack;
const serviceConfig = (await Import("./config.jsox")).default;

const os = await Import( "os" );
const sackModule = await Import( "sack.vfs" );
const sack = sackModule.sack;
const JSOX = sack.JSOX;
const disk = sack.Volume();
// my path is poorly defined here...
const srvc = disk.exists( "service.jsox" ) && sack.JSOX.parse( sack.Volume.readAsString( "service.jsox" ) );
if( srvc ) srvc.badges = srvc && disk.exists( "badges.jsox" ) && sack.JSOX.parse( sack.Volume.readAsString( "badges.jsox" ) );
let mySID = srvc.badges && disk.exists( "mySid.jsox" ) && sack.Volume.readAsString( "mySid.jsox" );

if( !srvc ) {
	console.log( "Service definition not found..." );
}
else if( !srvc.badges ) {
	console.log( "Badge definition not found for oranization..." );
}
//const SaltyRNGModule = await Import( "/javascript/d3x0r/srg/salty_random_generator.js" );
const SaltyRNG = sack.SaltyRNG;

const l = {
	badges : [],

};

const config = {
       	interfaces: []  // who I am...
       	, internal_interfaces: []  // who I am...
	, addresses: []  // who I am...
	, internal_addresses: []  // who I am...
	, run: {
		 hostname: os.hostname()
		, defaults: { useIPv6 : false, include_localhost : false, dedupInterfaces: false }
	},
}

const loc = getLocation();




ws.addService = function( prod,app,interface ) {
		
}


function registered( ws,msg ) {
	if( msg.ok ) {
		// srvc result ok?
		mySID = msg.sid;
		disk.write( "mySid.jsox", msg.sid );

		
	} else {
		console.log( "Failed to register Self" );
	}
}

function acceptUser( ws, msg ) {
	
}

const events = {};

ws.on = function( evt, d ) {
	if( "function" === typeof d ) {
        	if( evt in events ) events[evt].push( d );
                else events[evt] = [d];
        } else {
        	if( evt in events ) for( let cb of events[evt] ) cb( d );
        }
}

ws.processMessage = function( msg ) {
	console.trace( "handle message:", ws, msg );
	if( msg.op === "register" ) {
		registered( ws, msg );
		return true;
	} else if( msg.op === "expect" ) {
        	ws.on( "expect", msg );
	} else if( msg.op === "authorize" ) {
		// this has to be handled outside of this code
		// otherwise need to register event handler
		// on( "authorize", msg.user );		
		//acceptUser( ws, msg );
		console.log( "This seems to be inverted.." );
	} else {
		console.log( "Unhandled message from login server:", msg );
	}
}

if( srvc instanceof Array ) {
	// this might be an option; but then there would have to be multiple badge files; or badges with orgs
	//org.forEach( registerOrg );
} else 
	registerService( srvc, srvc.badges );

function registerService( srvc ) {
console.log( "Blah:", serviceConfig, serviceConfig.publicAddresses );
	ws.send( JSOX.stringify( { op:"register", sid:mySID, svc:srvc, loc:loc, addr:config.addresses, iaddr:config.internal_addresses, public:serviceConfig.publicAddresses } ) );
	const p = {p:null,res:null,rej:null};
	p.p = new Promise((res,rej)=>{p.res=res;p.rej=rej});
	return p.p;
}

ws.onclose= function() {
	// Add a second close handler for this.
}






//--------------------------------------------------


function getLocation() {
	const here = { 
		dir : process.cwd(),
		name : os.hostname(),
		macs : []
	};
	let i = os.networkInterfaces();
	const i2 = [];

	// sort interfaces by mac address
	for( var int in i ) {
        	var placed = false;
        	for( var int2 in i2 ) {
                	//2console.log( "is ", i2[int2][0].mac, " < ", i[int][0].mac );
                	if( i2[int2][0].mac > i[int][0].mac ) {
            			//console.log( "unshifted?" );
                                i2.splice( int2, 0, i[int] );
                                placed = true;
                                break;
                        }
                }
                if( !placed )
                	i2.push( i[int] );
        }
        i = i2;
        //console.log( "Should be sorted here.", i2 );
        config.internal_addresses = [];
        config.addresses = [];
	for( var int of i ) { 
		let added; added = 0; 
		let isLocal = false;
		const skipped = [];
		int.forEach( checkAddr );
		function checkAddr(i) {
			//console.log( "i:", i );
			if( i.family == "IPv6" ) {
				_debug_location && console.log( "check ipv6 interface:", i );
				if( i.address.startsWith( 'fe80' ) )
					;	
				else if( 
				    i.address.startsWith( 'dc00' )
					 ) {
					//console.log( "Applying this interface as address...", i );
					if( !isLocal )  {
						skipped.push( i );
					} else {
						//console.log( "So it's already known local... so... mark firewall target" );
						config.addresses.push( i );
						config.internal_addresses.push( i );
					}
								
					
				}
				else if( i.address === "::1"   // do allow ip6 localhost
					) {
					//if( !config.run.defaults.include_localhost ) return;
					// allow localhost as either a external or internal....
					if( !isLocal ) 
						skipped.push( i );
					else {
						config.addresses.push( i );
						config.internal_addresses.push( i );
					}
				}
				//config.interfaces.push( i );
				else if( true /*config.run.defaults.useIPv6*/ )
					if( !i.cidr.endsWith( "/128" ) )
					if( !isAddrLocal( i.address ) ) {
						_debug_location && console.log( "public ipvt?", i.address );
						config.addresses.push( i );
						config.interfaces.push( i );
						//console.log( "is external v6", config.addresses );
					} else {
						if( !isLocal ) {
							_debug_location && console.log( "is local v6", i.address );
							config.internal_addresses.push( i );
							config.internal_interfaces.push( i );
						}
					}
				added |= 2;
			} else {
				_debug_location && console.log( "v4:", i.address );
				if( i.address === "127.0.0.1" ) {
					//if( !config.run.defaults.include_localhost )
					return;
				}
				else 
					if( !isAddrLocal( i.address ) ) {
						_debug_location && console.log( "is NOT local:", isLocal );
						config.addresses.push( i );
						config.interfaces.push( i );
					} else {
						isLocal = true;
						_debug_location && console.log( "is local:", isLocal );
						config.internal_addresses.push( i );
						config.internal_interfaces.push( i );
					}
				added |= 1;
			}

			//added = true;
			//here += i.mac
		} 

		if( isLocal ) {
			_debug_location && console.log( "Redo some skipped ones", skipped.length );
			skipped.forEach( checkAddr );
		}
		if( added & 3 ) 
			if( !here.macs.find( m=>(m===int[0].mac) ) )
				here.macs.push( int[0].mac )
	}
	// move localhost address last.
	if( config.internal_addresses[0].address == "::1" ) {
		var save = config.internal_addresses[0];
		config.internal_addresses.splice( 0, 1 );
		config.internal_addresses.push( save );
	}
	console.log( "Usable addresses:", config.addresses, "internal:", config.internal_addresses, "here:", here, "JSOX(here):", JSOX.stringify( here ) );
	//here = "/home/chatment/kcore00:00:00:00:00:000c:c4:7a:7f:93:500c:c4:7a:7f:93:500c:c4:7a:7f:93:510c:c4:7a:7f:93:51";
	//console.log( "here is:", here,  idGen.regenerator( here ) );
	return sack.id( JSOX.stringify( here ) );
	

	function isAddrLocal(address) {
		//console.log( "Test address:", address );
		if( address.startsWith( "::ffff:" ) ) { // ::ffff:192.168.173.13
			address = address.substr( 7 );
		}
                if( address.startsWith( "::" ) && address.includes( "." ) ) {
                	address = address.substr( 2 );
                }
		if( address.startsWith( "192.168" )
			|| ["172.16.", "172.17.", "172.18.", "172.19.", "172.20.", "172.21.", "172.22.", "172.23.",
		        "172.24.", "172.25.", "172.26.", "172.27.", "172.28.", "172.29.", "172.30.", "172.31."].find( prefix=>address.startsWith( prefix ) )
			||address.startsWith( "10." ) )
			return true;
		if( address.startsWith( 'fe80' ) )
			return true;
		if( address === "::1" ) 
			return true;
		return false;
	}


}



