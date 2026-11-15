import {Protocol} from "sack.vfs/client-protocol"

export class Invoker extends Protocol {
	constructor( cmd ) {
		super( "http://localhost:4321", "summoner" );
		this.on( "open", ()=>{
			this.send( {op:"summon", cmd: cmd } );
		} );
		this.on( "done", ()=>{
			process.exit(0);
		} );
		this.on( "log", (ws,msg)=>{
			console.log( "Output:", msg.buf );
		} );
	}
}

export function summon(cmd ) {
	new Invoker(cmd)
}
