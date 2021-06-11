
import {Popup,popups} from "/node_modules/@d3x0r/popups/popups.mjs"


export class Profile extends Popup {
	constructor( parent ) {
		super( "Profile Managment", parent );
		popups.fillFromURL( this, "/ui/profile/profileForm.html" );
	}
}


