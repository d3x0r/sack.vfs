import {openServer} from "./server.mjs"

openServer(  );
openServer( {address:"0.0.0.0" } );
console.log( "serving on?", Number(process.env.PORT) || (process.argv.length > 2?Number(process.argv[2]) : 8080) );
