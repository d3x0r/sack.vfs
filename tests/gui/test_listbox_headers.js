
var sack = require( "../.." );

var f = sack.PSI.Frame( "test", -1, -1, 1200, 600 );

f.Control( "TextControl", "Simple Listbox", 10, 40, 500, 20 );

var b = f.Control( "Button", "Test", 10, 10, 100, 20 );

b.on( "click", function() {
	process.exit();
} );

b.text = "Quit...";

var x = sack.Image.Color( {r:50,g:50,b:255,a:255} );

var list = f.Control( "Listbox", 0, 80, 500, 500 );

list.onSelect( function( ) {
	console.log( "something selected", this );
} );

list.onDoubleClick( function( ) {
	console.log( "something double clicked", this );
} );

list.setTabs( [0, 250, 350 ] );
list.setHeader( "Item1\titem2" );
list.addItem( "Item One", 0 ).value = 1;
list.addItem( "Item Two", 1 ).value = 2;
list.addItem( "Item Three", { xyzz: 1 } ).value = {xyzz:1};
list.addItem( "Item Four" );
list.addItem( "Item Four-a", 1 );
list.addItem( "Item Four-b", 1 );
list.addItem( "Item Four-c", 1 );


var treeList = f.Control( "listbox", 550, 40, 500, 500 );
treeList.tree = true;

treeList.setTabs( [0, 250, 350], 0 );
treeList.setTabs( [0, 120, 250], 1 );

treeList.setHeader( "Item1\titem2" );
treeList.setHeader( "Item3\titem4", 1 );

treeList.onSelect( function( ) {
	console.log( "something selected", this );
} );

treeList.onDoubleClick( function( ) {
	console.log( "something double clicked", this );
} );

var item;
(item = treeList.addItem( "Item One\tMore Info" ) ).value = 1;
item.addItem( "Item Sub One?\tsub-column" );
item.addItem( "Item Two\tsub-field" ).value = 2;
( item = item.addItem( "Item Three\tsub-sub" )) .value = {xyzz:1};
item.addItem( "Item Four" );

(item = treeList.addItem( "Item One-a\tMore Info" ) ).value = 1;
(item = treeList.addItem( "Item One-b\tMore Info" ) ).value = 1;
(item = treeList.addItem( "Item One-c\tMore Info" ) ).value = 1;
(item = treeList.addItem( "Item One-d\tMore Info" ) ).value = 1;


f.show();

function tick() {
	setTimeout( tick, 50000 );
}
tick();

