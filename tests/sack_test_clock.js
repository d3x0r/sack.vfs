
var sack = require( ".." );

var f = sack.PSI.Frame( "test", -1, -1, 600, 600 );

var b = f.Control( "Button", "Test", 10, 10, 100, 20 );

b.on( "click", function() {
	process.exit();
} );

b.text = "Quit...";

var x = sack.Image.Color( {r:50,g:50,b:255,a:255} );

var clockDig = f.Control( "Basic Clock Widget", 0, 40, 300, 50 );

var clock = f.Control( "Basic Clock Widget", 0, 90, 500, 500 );
clock.analog();

f.show();

