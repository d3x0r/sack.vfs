

var vfs = require( "./vfs_module.js" )
var vol = vfs.Volume();
var data = vol.read( "comTest.js" );
console.log( Object.keys( data ) )
console.log( "data:", data );
console.log( "data:", data.toString() );

