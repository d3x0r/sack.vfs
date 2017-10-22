
var sack = require( ".." );
console.log( "got sack:", sack );

var r = sack.Renderer( "test", -1, -1, 500, 500 );
console.log( "renderer=", r );
var background = sack.Image( "the rror.jpg" );
console.log( "background=", r );

r.setDraw( ( image )=>{	
	console.log( "Needs to be drawn..." );
        image.drawImage( background );
} );

/*
r.setMouse( ( event )=> {
	console.log( "mouse event : ", event.x, event.y, event.b );
} );

r.setKey( ( key )=> {
	console.log( "key event : ", key.toString(16) );
} );
*/

r.show();

