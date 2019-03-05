
var sack = require( ".." );

var f = sack.PSI.Frame( "test", -1, -1, 600, 600 );
var b = f.Control( "Button", "Test", 10, 10, 100, 20 );
b.on( "click", function() {
	process.exit();
} );

b.text = "Quit...";
f.show();

var P = sack.PSI.Popup;
var p = sack.PSI.Popup();

var b = p.add( P.itemType.string,  "Button", ()=>console.log( "button selected" ) );

var c = [ 
	// this is a horrible example using p and P as variables.
	// can barely even tell that one is lower case and the other
	// is capital
	p.add( P.itemType.string,  "Button 2", ()=>console.log( "button2 selected" ) ),
	p.add( P.itemType.string,  "Button 3", ()=>console.log( "button3 selected" ) ),
	p.add( P.itemType.string,  "Button 4", ()=>console.log( "button4 selected" ) ),
	p.add( P.itemType.separator ),
	p.add( P.itemType.string,  "Button 5", ()=>console.log( "button5 selected" ) ),
	p.add( P.itemType.string,  "Button 6", ()=>console.log( "button6 selected" ) ),
	p.add( P.itemType.string,  "Button 7", ()=>console.log( "button7 selected" ) ),
	p.add( P.itemType.popup,  "Button 8", sack.PSI.Popup() ),
 ]
setTimeout( ()=>{p.track(f)}, 1000 );
//p.track();

