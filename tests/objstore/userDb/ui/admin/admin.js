
import {Popup,popups} from "/node_modules/@d3x0r/popups/popups.mjs"
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"
import {connection,openSocket} from "../login/webSocketClient.js"

const l = {
	login : null, // login form
	ws :null,
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




export class Admin extends Popup {
	constructor( parent ) {
		super( "User Database Administration", parent );
		popups.fillFromURL( this, "/ui/admin/adminForm.html" );
		this.hide();
		openSocket();
		
		const login = l.login = popups.makeLoginForm( async ()=>{
		
			//console.log( "login form event" );
			//debugger;
			login.hide();
			const ws = l.ws = await connection.request( "d3x0r.org", "login" );
			ws.onmessage = handleMessage;
			ws.onclose = handleClose;
			//this.show();
			this.load();

		} , {useForm:"/ui/login/loginForm.html", wsLoginClient:connection} );

		

		function handleMessage( msg_ ) {
			const msg = JSOX.parse( msg_ );

		}
		function handleClose( code, reason ) {
			l.ws = null;
			loginForm.show
		}
	}


	load() {
		this.show();
		
	}
}



