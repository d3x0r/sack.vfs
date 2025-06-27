
var sack = require( "../.." );

sack.Sqlite.optionEditor();

var f = sack.PSI.Frame( "test", -1, -1, 600, 600, sack.PSI.control.border.resizable );
var dialogInitialSize = f.size;
var consoleSize = null;

f.on( "size", (w,h,start)=>{
	if( !start ) {
		con.size = {width: w- ( dialogInitialSize.width - consoleSize.width ),height:h-( 60 + (dialogInitialSize.height - consoleSize.height) )};
	}
} );

var con = f.Control( "PSI Console", 0, 0, 600, 600 );
con.echo = true;

consoleSize = con.size;

con.oninput( function( string ) {
	var num = Number(string);
	con.write( "Got:" + num + "\n");
	if( num > 1 && num < 10 ) {
		con.write( "Thank you. THe secret exit code is 123.\n" );
	} else
		con.write( "input was wrong.  Please enter a number between 1 and 10.\n" );
	if( num === 123 )
		process.exit();
	con.write( " : " );
} );

con.write( "So some sort of demo program...\n" );
con.write( "Enter a number from 1-10\n" );
con.write( " : " );

f.show();

// some sort of f.wait() event is needed... maybe it's a promise that only resolves on close?
function tick() { setTimeout( tick, 50000 );} tick();
