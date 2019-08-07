var sack = require( ".." );

var r = sack.Renderer( "test", -1, -1, 500, 500 );
var background = sack.Image( "the rror.jpg" );

r.setDraw( ( image )=>{	
        image.drawImage( background );
} );

r.show();

