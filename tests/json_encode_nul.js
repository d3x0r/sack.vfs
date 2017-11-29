
var vfs = require( ".." );
var JSON = vfs.JSON6;

var input = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
var output = JSON.escape( input );
console.log( "string:", input, "becomes:", output );

