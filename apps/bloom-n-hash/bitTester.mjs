import {BitReader} from "./bits.mjs";

const bits = new BitReader( 256 );

console.log( bits );

bits.set( "01010101010101" );
console.log( bits );
const val = bits.getBits( 0,3,false );
const val2 = bits.getBits( 3 );
console.log( "Val1 and 2:", val, val2 );
console.log( "Val1 and 2:", val.toString(2), val2.toString(2) );


bits.set( "01010101010101", true );
console.log( bits );
const val3 = bits.getBits( 0,3,false );
const val4 = bits.getBits( 5,3,false );
console.log( "Val1 and 2:", val3, val4 );
console.log( "Val1 and 2:", val3.toString(2), val4.toString(2) );

bits.set( 3 );
bits.get( 3 );
console.log( bits );
