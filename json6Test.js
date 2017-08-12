

var vfs = require( "." )

var parse = vfs.JSON6.parse;

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
var o = parse( "0b1010101" );
console.log( "0b1010101 is", o, typeof o );
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

	var o = parse( "NaN" );
	console.log( "o is", o, typeof o );
	var o = parse( "-NaN" );
	console.log( "o is", o, typeof o );
	var o = parse( "Infinity" );
	console.log( "o is", o, typeof o );
	var o = parse( "-Infinity" );
	console.log( "o is", o, typeof o );

var o = parse( "{a:123}" );
console.log( "o is", o );

var o = parse( "{a:`abcdef`}" );
console.log( "o is", o );
var o = parse( "{a:\"abcdef\"}" );
console.log( "o is", o );

var o = parse( "{a:'abc\ndef'}" );
console.log( "o is", o );
var o = parse( "{a:'abc\\\ndef'}" );
console.log( "o is", o );
var o = parse( "{a:'abc\\\r\ndef'}" );
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
	//parse( '"Simple String Value."' );
}

var end = Date.now();
console.log( "1m in ", end-start );


var translations = ["{\"a\":{\"b\":{\"c\":{\"d\":123}}}}","{\"a\":{\"b\":{\"c\":{\"d\":123}}}}","{\"a\":{\"b\":{\"c\":{\"d\":123}}}}","{\"a\":{\"b\":{\"c\":{\"d\":123}}}}"];
var ntrans = 0;

start = end;
for( n = 0; n < 1000000; n++ ) {
	JSON.parse( translations[ntrans] );
        ntrans = (ntrans+1)&3;
}
end = Date.now();
console.log( "1m in ", end-start );
}

