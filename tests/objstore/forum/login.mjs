
import {loginServer} from "../userDb/serviceLogin.mjs"

loginServer.open( {
		server:"localhost:8089"
	} );

loginServer.on( "open", loginOpen );
loginServer.on( "close", loginClose );

async function loginOpen( ws ) {
    const reg = await loginServer.register( "d3x0r.org", "Sack.VFS test forum" );
    reg.on( "login", newLogin );
}

function loginClose() {

}


async function newLogin( opts ) {
    // opts.clientId = user ID
    // opts.name = display name
    // (other options)

    let user = await UserDb.get( opts.clientId );
    if( !user ) {
        user = await UserDb.create( opts.clientId, opts.name );
    }
    if( user )
	    return true;
    if( user.banned ) return false;
    return false;
}
