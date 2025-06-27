
import {Popup,popups} from "/node_modules/@d3x0r/popups/popups.mjs"
import {Protocol}  from "../../http-ws/client-protocol.js"

let config = {};
popups.utils.preAddPopupStyles(document.head);

class Settings extends Popup {
	constructor() {
		super( "Mouse Clicker settings", display );

		popups.makeButton( this, "Save", ()=>{
			protocol.sendSettings();
		} );
		
	}
	update( msg ) {
			Object.assign( config, msg.settings );
	}
}


class SettingsProtocol extends Protocol{
	constructor() {
		super( "mouse-clicker" );
		this.on( "settings", (msg)=>settings.update(msg) );
	}
	sendSettings() {
		this.send( {op:"settings", settings:config } );
	}
}

const settings = new Settings();
const protocol = new SettingsProtocol();