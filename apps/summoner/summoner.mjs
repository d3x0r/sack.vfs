
import {sack} from "sack.vfs"
import {Protocol} from "sack.vfs/protocol"

class Summoner extends Protocol {
	constructor() {
		super( {address:"::1", port:Number( process.env.PORT) || 123, protocol:"summoner" } );
		this.on( "summon", this.summon.bind( this ) );
	}
	summon( a, b ) {
		const args = b.cmd.split(' ' );
		const task = sack.Task( { bin: args[0], args: args.slice(1)
			, input(buf) {
				a.send( {op:"log", buf } );
			}
			, end() {
				a.send( {op:"done", exitCode:task.exitCode } );
			} } );
	}
}

const summoner = new Summoner();

summoner.on( "failedRequest", (req,res)=>{
	console.log( "Failed:", req.url );
} );
