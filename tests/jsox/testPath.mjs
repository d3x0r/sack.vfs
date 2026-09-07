import {JSOX} from "jsox" 

console.log( JSOX.parse( "this.test" ) );

console.log( JSOX.parse( "{this.test:1}" ) );