
var vfs = require( ".." );
var vol = vfs.Volume( null, "test.vfs", "key1", "key2" );

console.log( "Directory:", vol.dir() );
var xx = vol.read( "xx" );
console.log("got xx?:", xx );