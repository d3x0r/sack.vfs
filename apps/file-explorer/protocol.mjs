
import {Protocol} from "../http-ws/server-protocol.mjs"
import {sack} from "sack.vfs"
const JSOX = sack.JSOX;

export class ExplorerProtocol extends Protocol {
	constructor() {
		super( {port:5543, protocol:"FileExplorer", resourcePath:"ui", commonPath:"..", npmPath:"../.."} );
		
	}
	reply( ws, op, data ) {
		ws.send( JSOX.stringify( {op, [op]:data } ) );
	}

}