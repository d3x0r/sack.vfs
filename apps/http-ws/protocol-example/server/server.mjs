
import config from "../config.jsox"

import {sack} from "sack.vfs"
const disk = sack.Volume();
import {openServer} from "sack.vfs/apps/http-ws"
import {protocol} from "./protocol.mjs"


protocol.on( "load", (ws, msg)=>{
	console.log( "got load?", msg );
	for( let path of config.xkb_configurations ) {
		const dir = disk.dir( msg.path || "." );
		ws.send( {op:'dir', dir } );
	}
} );
