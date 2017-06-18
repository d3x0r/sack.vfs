
var vfs = require( "./build/Release/vfs_module.node" )

console.log( vfs.openVolume( "./vault.vfs", "some key text", "Some other Key" ) );

