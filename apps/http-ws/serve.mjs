import {openServer} from "./server.mjs"

openServer( );
console.log( "serving on?", Number(process.env.PORT) || (process.argv.length > 2?Number(process.argv[2]) : 8080) );
