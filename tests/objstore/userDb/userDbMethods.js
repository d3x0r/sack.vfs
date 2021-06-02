
const ws = this;
console.log( "Extend this websocket:", this );

const SaltyRNGModule = await Import( "/node_modules/@d3x0r/srg/salty_random_generator.js" );
const SaltyRNG = SaltyRNGModule.SaltyRNG;
ws.SaltyRNG = SaltyRNG;

const clientKey = localStorage.getItem( "clientId" );
if( !clientKey ) {
    	ws.send( `{op:newClient}` );
}


console.log( "localStorage?", localStorage);

ws.doLogin = function( user, pass ) {
    //ws.send(
	pass = SaltyRNG.id(pass);
    ws.send( `{op:"login",account:${JSON.stringify(user)},password:${JSON.stringify(pass)}
        		,clientId:${JSON.stringify(localStorage.getItem("clientId"))}
                        ,deviceId:${JSON.stringify(localStorage.getItem("deviceId"))} }` );

}
ws.doCreate = function( display, user, pass, email ) {
    //ws.send(
	pass = SaltyRNG.id(pass);
    ws.send( `{op:"create",account:${JSON.stringify(user)},password:${JSON.stringify(pass)}
            		,user:${JSON.stringify(display)},email:${JSON.stringify(email)}
        		,clientId:${JSON.stringify(localStorage.getItem("clientId"))}
                        ,deviceId:${JSON.stringify(localStorage.getItem("deviceId"))} }` );
}
ws.doGuest = function( user ) {
    //ws.send(
    ws.send( `{op:"guest",user:${JSON.stringify(user)}
        		,clientId:${JSON.stringify(localStorage.getItem("clientId"))}
                        ,deviceId:${JSON.stringify(localStorage.getItem("deviceId"))} }` );
}


const sesKey = localStorage.getItem( "seskey" );
if( sesKey ) {
	ws.send( `{op:"Login",seskey:${JSON.stringify(sesKey)}
        		,clientId:${JSON.stringify(localStorage.getItem("clientId"))}
                        ,deviceId:${JSON.stringify(localStorage.getItem("deviceId"))} }` );

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