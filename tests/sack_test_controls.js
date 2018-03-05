
var sack = require( ".." );

var f = sack.PSI.Frame( "test", -1, -1, 600, 600 );

var b = f.Control( "Button", "Test", 10, 10, 100, 20 );

b.on( "click", function() {
	process.exit();
} );

b.text = "Quit...";

f.Control( "EditControl", "editField", 150, 40, 120, 20 );
f.Control( "TextControl", "Text", 10, 40, 120, 20 );
var pages = f.Control( "SheetControl", "Text", 10, 80, 220, 120 );

var page1 = sack.PSI.Frame( "Page 1", -1, -1, 600, 600 );
pages.addPage( page1 );
var page2 = sack.PSI.Frame( "Page 2", -1, -1, 600, 600 );
pages.addPage( page2 );
var page3 = sack.PSI.Frame( "Page 3", -1, -1, 600, 600 );
pages.addPage( page3 );
var page4 = sack.PSI.Frame( "Page 4", -1, -1, 600, 600 );
pages.addPage( page4 );

f.show();

