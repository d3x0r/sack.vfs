
var sack = require( ".." );

var f = sack.Frame( "test", -1, -1, 600, 600 );

var b = f.Control( "Button", "Test", 10, 10, 100, 20 );

b.on( "click", function() {
	process.exit();
} );

b.text = "Quit...";

var x = sack.Image.Color( {r:50,g:50,b:255,a:255} );

var list = f.Control( "Listbox", 0, 40, 500, 500 );

list.onselect( function( ) {
	console.log( "something selected" );
} );

list.addItem( "Item One", 1 );
list.addItem( "Item Two", 2 );
list.addItem( "Item Three", { xyzz: 1 } );
list.addItem( "Item Four" );

f.show();

