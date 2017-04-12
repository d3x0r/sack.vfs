

var vfs = require( "." )

var parse = vfs.JSON.parse;

var o = parse( "123" );
console.log( "o is", o );
var o = parse( "\"123\"" );
console.log( "o is", o );
var o = parse( "null" );
console.log( "o is", o );
var o = parse( "true" );
console.log( "o is", o );
var o = parse( "false" );
console.log( "o is", o );
var o = parse( "undefined" );
console.log( "o is", o );


var o = parse( "{\"a\":123}" );
console.log( "o is", o );
var o = parse( "[123]" );
console.log( "o is", o );
var o = parse( "{\"a\":{\"b\":{\"c\":{\"d\":123}}}}" );
console.log( "o is", o );


var start = Date.now();
var n;
for( n = 0; n < 1000000; n++ ) {
	parse( "{\"a\":{\"b\":{\"c\":{\"d\":123}}}}" );
	parse( '"Simple String Value."' );
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
