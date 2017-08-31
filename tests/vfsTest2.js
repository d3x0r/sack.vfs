

var vfs = require( ".." )
var vol = vfs.Volume();
var data = vol.read( "comTest.js" );
console.log( Object.keys( data ) )
console.log( "data:", data );
console.log( "data:", data.toString() );

