
// what? you want to actually write to files iteritively?

var sack = require( '..' );
var disk = sack.Volume();
var file = disk.File( "output.txt" );

file.write( "test" );
file.write( "\n" );
file.write( "Two Liens?" );

