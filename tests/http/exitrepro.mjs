// Does the process exit on its own?  No process.exit() anywhere on purpose -
// whether node returns to the shell is the whole measurement.  The server is a
// separate process (wirepeer.mjs) so it cannot pin this loop.
import { sack } from "sack.vfs";
const PORT = Number(process.env.PORT)||8097;
const STAGE = process.env.STAGE || "idle";

const t0 = Date.now();
process.on( "exit", c => console.log( `EXITED code=${c} after ${Date.now()-t0}ms` ) );

const conn = await sack.HTTP.stream( { hostname:"localhost", port:PORT } );
console.log( "connected" );

if( STAGE === "norequest" ) {
	console.log( "no request issued; end of script" );
} else if( STAGE === "idle" ) {
	console.log( "request:", (await conn.request({path:"/a"})).statusCode );
	console.log( "left open and idle; end of script" );
} else if( STAGE === "closed" ) {
	console.log( "request:", (await conn.request({path:"/a"})).statusCode );
	conn.close();
	console.log( "closed; end of script" );
} else if( STAGE === "pending" ) {
	// fire and DON'T await - does the loop stay alive long enough to settle it?
	conn.request( { path:"/late" } ).then(
		r => console.log( "late request settled:", r.statusCode ),
		e => console.log( "late request rejected:", e.message ) );
	console.log( "request in flight; end of script" );
}
