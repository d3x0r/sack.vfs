
import {Protocol as Protocol_} from "/node_modules/sack.vfs/apps/http-ws/client-protocol.js"

class Protocol extends Protocol_ {

	static ws = null;

	constructor(){
		super( "xkb-editor" );
		//this.on( "dir", ( message ) 
	}

	load() {
		// results with a "dir" event (?)
		this.send( "{op:load}" );
	}
} 

export const protocol = new Protocol();
export default protocol;
