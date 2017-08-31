
var vfs = require( '..' );
var vol = vfs.Volume();
var vol2 = vfs.Volume( 'data.vfs' );

var data = vol.read( "stream.json" );
vol2.write( "streamdata", data );

vol.readJSON( "stream.json", (val)=>{
	console.log( "Got Object:", val );
 } );

vol2.readJSON( "streamdata", (val)=>{
	console.log( "Got Object:", val );
 } );

