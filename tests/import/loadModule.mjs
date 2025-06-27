import {forceNextModule} from "sack.vfs/import";
forceNextModule(true);
//import {Class} from "./module.js"

const c = (await import( "./module.js" ) ).Class;
console.log( "Success?", c );
forceNextModule(false);
