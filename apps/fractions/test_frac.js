
const Fraction = require( "./fractions.js" );

console.log( "1/3 = ", new Fraction(1,3).toString() );
console.log( "3/2 = ", new Fraction(3,2).toString() );
console.log( "3/2+9/16 = ", new Fraction(3,2).add( new Fraction( 9,16 ) ).toString() );

