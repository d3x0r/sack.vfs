
const ws = this;
console.log( "Extend this websocket:", this );

const SaltyRNGModule = await Import( "/javascript/d3x0r/srg/salty_random_generator.js" );
const SaltyRNG = SaltyRNGModule.SaltyRNG;
ws.SaltyRNG = SaltyRNG;

/*
const clientKey = localStorage.getItem( "clientId" );
if( !clientKey ) {
    	ws.send( `{op:newClient}` );
}
*/

//console.log( "localStorage?", localStorage);

ws.addService = function( prod,app,interface ) {
		
}
ws.doCreate = function( display, user, pass, email ) {
}
ws.doGuest = function( user ) {
}


ws.processMessage = function( ws, msg ) {
	if( msg.op === "login" ) {
		if( msg.success )
			;//Alert(" Login Success" );
		else if( msg.ban ) {
			//Alert( "Bannable Offense" );
			localStorage.removeItem( "clientId" ); // reset this
			ws.close();
		} else if( msg.device ) {
			//temporary failure, this device was unidentified, or someone elses
			const newId = SaltyRNG.Id();
			localStorage.setItem( "deviceId", newId );
			ws.send( JSON.stringify( {op:"device", deviceId:newId } ) );
			return true;
		} else
			;//Alert( "Login Failed..." );		
                
        }
	else if( msg.op === "create" ) {
		if( msg.success ) {
			;//Alert(" Login Success" );
                        localStorage.setItem( "deviceId", msg.deviceId );
		} else if( msg.ban )  {
			//Alert( "Bannable Offense" );
			localStorage.removeItem( "clientId" ); // reset this
			ws.close();
		} else if( msg.device ) {
			//temporary failure, this device was unidentified, or someone elses
			const newId = SaltyRNG.Id();
			localStorage.setItem( "deviceId", newId );
			ws.send( JSON.stringify( {op:"device", deviceId:newId } ) );
			return true;
		} else
			;//Alert( "Login Failed..." );		
                
        }
        else if( msg.op === "set" ) {
            	localStorage.setItem( msg.value, msg.key );
        }

}
