


import {Popup,popups} from "/node_modules/@d3x0r/popups/popups.mjs"
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"

import {connection,openSocket,Alert} from "../login/webSocketClient.js"

const l = {
	login : null, // login form
	ws :null,
}



export class Profile extends Popup {
	#sock = null
	constructor( parent ) {
		super( "User Profile Manager", parent );
		this.hide();
		// this will ahve to be re-opened...
		openSocket(null,(sock)=>{
			this.#sock = sock;
			this.#sock.on( "disconnect", ()=>{
				console.log( "disconnect for login socket... probably OK... but will need it next time. ");
			} );
		} ); // trigger client begin connection... 
		
		const login = l.login = popups.makeLoginForm( async (guest)=>{
			console.log( "paraeter is guest?:", guest );
			//console.log( "login form event" );
			//debugger;
			login.hide();
			const info = await connection.request( "d3x0r.org", "login" );
			
			console.log( "service information:", info );
			if( info ) {
				openSocket( info.addr, (ws)=>{
					ws.onmessage = handleMessage;
					ws.onclose = handleClose;
					this.load();					
				}, "profile" );
			} else {
				Alert( "Profile service failed to be found" );
				login.show();
			}

		} , { useForm:"/ui/login/loginForm.html"
		    , useSashForm:"/ui/login/pickSashForm.html"
		    , sashScript : "/ui/login/pickSashForm.js"
		    , wsLoginClient:connection} );

		

		function handleMessage( msg_ ) {
			const msg = JSOX.parse( msg_ );
			console.log( "Really this should be some sort of internal handler?" );
		}
		function handleClose( code, reason ) {
			console.log( "profile service disconnected..." );
			l.ws = null; // service connection... 
			login.show();
		}
	}


	load( something ) {
		popups.fillFromURL( this, "./profileForm.html" );
		this.show();
		
	}
}



