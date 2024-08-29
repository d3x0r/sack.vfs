var sack= require( "../.." );
const JSOX = sack.JSOX;

const a = JSOX.stringify( { a:"#Thisshouldbequoted" } );
console.log( "A:", a );
