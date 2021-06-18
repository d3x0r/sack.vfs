
const ws = this;
console.log( "Extend this websocket:", this );

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

//ws.SaltyRNG = SaltyRNG;

/*
const clientKey = localStorage.getItem( "clientId" );
if( !clientKey ) {
    	ws.send( `{op:newClient}` );
}
*/

//console.log( "localStorage?", localStorage);

ws.addService = function( prod,app,interface ) {
		
}


function resolveBadge( msg ) {
	if( msg.ok ) {
		for( let badgeName of srvc.badges ) {
			if( badgeName === msg.badgeName ) {
				srvc.badges[badgeName].promise.res( msg.badge );
			}
		}
	} else {
		// badge failed creation.
		console.log( "Server failed to create badge" );
	}
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

ws.processMessage = function( ws, msg ) {
	console.trace( "handle message:", ws, msg );
	if( msg.op === "register" ) {
		registered( ws, msg );
	} else if( msg.op === "authorize" ) {
		acceptUser( ws, msg );
	} else if( msg.op === "badge" ) {
		resolveBadge( ws, msg );
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
	ws.send( JSOX.stringify( { op:"register", sid:mySID, svc:srvc } ) );	
	const p = {p:null,res:null,rej:null};
	p.p = new Promise((res,rej)=>{p.res=res;p.rej=rej});
	return p.p;
}

ws.onclose= function() {
	// Add a second close handler for this.
}