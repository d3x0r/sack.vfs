
var sack = require( "../.." );

var f = sack.PSI.Frame( "test", -1, -1, 600, 200 );

var label = f.Control( "TextControl", "Simple Listbox", 10, 40, 500, 20 );
var input = f.Control( "EditControl", "<default Text>", 10, 80, 500, 20 );

var b = f.Control( "Button", "Test", 10, 10, 100, 20 );

b.on( "click", function() {
	label.text = input.text;
} );


f.show();

// some sort of f.wait() event is needed... maybe it's a promise that only resolves on close?
function tick() { setTimeout( tick, 50000 );} tick();
