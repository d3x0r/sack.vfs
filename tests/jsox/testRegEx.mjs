import {sack} from "sack.vfs"

const JSOX=sack.JSOX;
const r_string = JSOX.stringify( /^G-[0-9]+$/i );
console.log( JSOX.stringify( /^G-[0-9]+$/i ) );

const jreg = JSOX.parse( r_string );
console.log( "Would it be?", jreg.source, " and ", jreg.flags );

const jreg2 = JSOX.parse( JSOX.stringify( /^G-[0-9]+$/i ) );
console.log( "Would it be?", jreg2.source, " and ", jreg2.flags );

const regex = RegExp( r_string );

console.log( "Would it be?", regex.source, " and ", regex.flags );