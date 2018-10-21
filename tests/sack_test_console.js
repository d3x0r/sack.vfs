
var sack = require( ".." );

sack.Sqlite.optionEditor();

var f = sack.PSI.Frame( "test", -1, -1, 600, 600 );

var b = f.Control( "Button", "Test", 10, 10, 100, 20 );

b.on( "click", function() {
	process.exit();
} );

b.text = "Quit...";

var x = sack.Image.Color( {r:50,g:50,b:255,a:255} );

var con = f.Control( "PSI Console", 0, 40, 500, 500 );

con.oninput( function( string ) {
	console.log( "Event string:", string );
	con.write( string + "\n" );
} );

con.write( "Enter any text to have it echoed..." );

f.show();

