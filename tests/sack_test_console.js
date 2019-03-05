
var sack = require( ".." );

sack.Sqlite.optionEditor();

var f = sack.PSI.Frame( "test", -1, -1, 600, 600, sack.PSI.control.border.resizable );
var dialogInitialSize = f.size;
var consoleSize = null;

f.on( "size", (w,h,start)=>{
	if( !start ) {
		con.size = {width: w- ( dialogInitialSize.width - consoleSize.width ),height:h-( 60 + (dialogInitialSize.height - consoleSize.height) )};
	}
} );

var b = f.Control( "Button", "Test", 10, 10, 100, 20 );

b.on( "click", function() {
	console.log( "Success This far; process.exit *BOOM*" );
	process.exit();
} );

b.text = "Quit...";

var x = sack.Image.Color( {r:50,g:50,b:255,a:255} );

var con = f.Control( "PSI Console", 0, 40, 500, 500 );
//con.echo = true;

consoleSize = con.size;


con.oninput( function( string ) {
	string = string.slice( 0, string.length-1 );
	console.log( "Event string:", string );
	con.write( string );
} );

con.write( "Enter any text to have it echoed..." );

var customText = new sack.Text();
customText.text = "Hello, World";
customText.noReturn = true;
customText.foreground = 3;
con.write( customText );



var resetText = new sack.Text();
resetText.foreground = true;
resetText.background = true;
resetText.noReturn = true;
con.write( resetText );

con.write( customText );
con.write( resetText );

//con.write( "Just some default text" );
//con.write( "Each write is a new line..." );
//con.write( "Because that's how logging works" );

//con.write( customText );

f.show();

