

var vfs = require( "." )

var parse = vfs.JSON.parse;

//console.log( "Stringify Test:", vfs.JSON.stringify( { a:123 } ) );

//var x = '[' + JSON.stringify( new Date() ) + ']';
//console.log( "Date Output is:", x, JSON.stringify( new Date() ) );

var o = parse( "123" );
console.log( "123 is", o, typeof o );
var o = parse( "123_456_789" );
console.log( "123_456_789 is", o, typeof o );
var o = parse( "0123" );
console.log( "0123 is", o, typeof o );
var o = parse( "0x123" );
console.log( "0x123 is", o, typeof o );
var o = parse( "\"123\"" );
console.log( "o is", o, typeof o );
var o = parse( "null" );
console.log( "o is", o, typeof o );
var o = parse( "true", typeof o );
console.log( "o is", o, typeof o );
var o = parse( "false" );
console.log( "o is", o, typeof o );

var o = parse( "undefined" );
console.log( "o is", o, typeof o );

var o = parse( "{\"a\":123}" );
console.log( "o is", o );

var o = parse( "{\"a\":\"abcdef\"}" );
console.log( "o is", o );
var o = parse( "{\"a\":\"abcdef\"}" );
console.log( "o is", o );

var o = parse( "{\"a\":\"abc\ndef\"}" );
console.log( "o is", o );
var o = parse( "{\"a\":\"abc\\\ndef\"}" );
console.log( "o is", o );
var o = parse( "{\"a\":\"abc\\\r\ndef\"}" );
console.log( "o is", o );

var o = parse( "{\"a\":123}" );
console.log( "o is", o );
var o = parse( "[123]" );
console.log( "o is", o );
var o = parse( "{\"a\":{\"b\":{\"c\":{\"d\":123}}}}" );
console.log( "o is", JSON.stringify( o ) );


// benchmark - needs some work; ended up somewhat divergent.
if(true)
{

var start = Date.now();
var n;
for( n = 0; n < 1000000; n++ ) {
	parse( "{\"a\":{\"b\":{\"c\":{\"d\":123}}}}" );
	parse( '"Simple String value"' );
	parse( '123456789' );
}

var end = Date.now();
console.log( "1m in ", end-start );


start = end;
for( n = 0; n < 1000000; n++ ) {
	JSON.parse( "{\"a\":{\"b\":{\"c\":{\"d\":123}}}}" );
	JSON.parse( '"Simple String value"' );
	JSON.parse( '123456789' );
}
end = Date.now();
console.log( "1m in ", end-start );
}

