var sack= require( "../.." );
const JSOX = sack.JSOX;


console.log( JSOX.parse( '{ a:1234, b:{"a":123 }}' ) )
console.log( JSOX.parse( 'a{a,b}{ a:1234, b:{"a":123 },c:a{3,4} }' ) )
console.log( JSOX.parse( '{ a:1234, b:{"a":123 }}' ) )
console.log( JSOX.parse( '{ a:1234, b:{"a":123 }}' ) )
