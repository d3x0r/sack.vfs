

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
