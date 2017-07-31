
var vfs = require( "./vfs_module.js" );
var vol = vfs.Volume();

console.log( "Directory:", vol.dir() );
var xx = vol.read( "xx" );
console.log("got xx?:", xx );