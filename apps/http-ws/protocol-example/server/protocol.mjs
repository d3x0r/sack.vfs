
import {Protocol as Protocol_ } from "sack.vfs/protocol"

class Protocol extends Protocol_ {

	constructor() {
		const opts = { resourcePath:"ui", port:Number(process.env.PORT) || 4321 } ;
		super( opts );
		console.log( "Probably serving OK?", opts.port ); 
		//this.on( "
	}

}

export const protocol = new Protocol();
