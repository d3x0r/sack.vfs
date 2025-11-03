

import {Protocol} from "/common/http-ws/client-protocol.js"


export class ExplorerProtocol extends Protocol {
	constructor() {
		super( "FileExplorer", location.origin.replace("http","ws") + "/" );
	}
	dir() {
		this.send( {op:"dir"} );
	}

	setDir( setDir ) {
		this.send( {op:"setDir", setDir } );
	}

	upDir( d ) {
		this.send( {op:"upDir" } );
	}

}