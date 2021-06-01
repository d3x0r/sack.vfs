
const ws = this;
console.log( "Extend this websocket:", this );

const SaltyRNGModule = await Import( "/node_modules/@d3x0r/srg/salty_random_generator.js" );
const SaltyRNG = SaltyRNGModule.SaltyRNG;
const clientKey = localStorage.getItem( "clientId" );
if( !clientKey ) {
    	ws.send( `{op:newClient}` );
}


console.log( "localStorage?", localStorage);

ws.doLogin = function( user, pass ) {
    //ws.send(
    ws.send( `{op:"login",account:${JSON.stringify(user)},password:${JSON.stringify(pass)}
        		,clientId:${JSON.stringify(localStorage.getItem("clientId"))}
                        ,deviceId:${JSON.stringify(localStorage.getItem("deviceId"))} }` );

}
ws.doCreate = function( display, user, pass, email ) {
    //ws.send(
    ws.send( `{op:"create",account:${JSON.stringify(user)},password:${JSON.stringify(pass)}
            		,user:${JSON.stringify(display)}
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
