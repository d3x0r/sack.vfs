const JSOX = require("../.." ).JSOX;

console.log( "Iso", new Date().toISOString() );
console.log( "jsox", JSOX.stringify( new Date() ) );