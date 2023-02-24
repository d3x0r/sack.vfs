import {openServer} from "./server.mjs"

openServer();
console.log( "serving on?", process.env.PORT || 8080 );
