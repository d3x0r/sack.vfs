
const ws = this;
console.log( "Extend this websocket:", this );

const SaltyRNGModule = await Import( "/node_modules/@d3x0r/srg/salty_random_generator.js" );
const SaltyRNG = SaltyRNGModule.SaltyRNG;
//ws.SaltyRNG = SaltyRNG;


const clientKey = localStorage.getItem( "clientId" );
if( !clientKey ) {
    	ws.send( `{op:newClient}` );
}


const l = {
	pending:[]
}

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

ws.getService = function( domain, service ) {
    //ws.send(
    ws.send( `{op:"service",domain:${JSON.stringify(domain)},service:${JSON.stringify(service)}}`);
}


const sesKey = localStorage.getItem( "seskey" );
if( sesKey ) {
	ws.send( `{op:"Login",seskey:${JSON.stringify(sesKey)}
        		,clientId:${JSON.stringify(localStorage.getItem("clientId"))}
                        ,deviceId:${JSON.stringify(localStorage.getItem("deviceId"))} }` );

}

ws.request = function( domain, service ) {
	const pend = {op:"request", id: SaltyRNG.Id(), p:null,domain:domain, service:service, res:null,rej:null };
	ws.send( `{op:"request",id:'${pend.id}',domain:${JSON.stringify(domain)},service:${JSON.stringify(service)}}`);
	pend.p = new Promise( (res,rej)=>{
		pend.res = res; pend.rej=rej;
	}).then( (msg)=>{
		console.log(" Service should have addr...", msg );
		const idx = l.pending.findIndex( p=>p === pend );
		if( idx >= 0 ) l.pending.splice(idx,1 );
		else console.log( "Failed to find pending request." );
		return msg;
	} )
	l.pending.push( pend );
	return pend.p;
}

ws.processMessage = function( ws, msg ) {
	console.log( "socket gets a turn?", msg );
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
		return true; // client doesn't care.
        }
	else if( msg.op === "guest" ) {
		if( msg.success ) {
			;//Alert(" Login Success" );
		} else
			;//Alert( "Login Failed..." );		
                
        }
        else if( msg.op === "set" ) {
            	localStorage.setItem( msg.value, msg.key );
					return true; // client doesn't care.
        }
	else if( msg.op === "pickSash" ) {
		// this is actually a client event.
	}
	else if( msg.op === "request" ) {
		for( let pend of l.pending ) {
			if( pend.id === msg.id ) {
				pend.res( msg.svc );
			}
		}
	}

}
