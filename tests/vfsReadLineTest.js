
if( process.argv.length < 3 ) {
	console.log( "Required parameter:filename missing" );
	process.exit();
}
	

var vfs = require( ".." );
var vol = vfs.Volume();
var file = vol.File( process.argv[2] );
if( !file ) {
	console.log( "File not found?", process.argv[2] );
        process.exit();
}

var line;
while( line = file.readLine() ) {
	console.log( "Read:", line );
}
