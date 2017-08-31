

var vfs = require( '..' );
var vol = vfs.Volume( "largeStream.vfs" );
var file = vol.File( "data" );

console.log( "Writing file..." );
var start = Date.now();
for(  var i = 0; i < 300000; i++ )
	file.writeLine( `{\"a${i}\":{\"b${i}\":{\"c${i}\":{\"d${i}\":123}}}}` );
console.log( "Wrote in ", Date.now() - start, " .. reading..." );

var start = Date.now();
var n = 0;
vol.readJSON( "data", (object)=>{ n++ } );
console.log( "()=>{} Read in ...", n, Date.now() - start );
var start = Date.now();
n = 0;
vol.readJSON( "data", function(object){ n++ } );
console.log( "function(){} Read in ...", n, Date.now() - start );
