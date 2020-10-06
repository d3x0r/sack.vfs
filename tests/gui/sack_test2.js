var sack = require( "../.." );
console.log( "Creates a simple output window and draws an image to it." );
var r = sack.Renderer( "test", -1, -1, 500, 500 );
var background = sack.Image( "images/stop_button.png" );

r.setDraw( ( image )=>{	
        image.drawImage( background );
} );

r.show();

