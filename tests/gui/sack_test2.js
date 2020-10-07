var sack = require( "../.." );
console.log( "Creates a simple output window and draws an image to it." );
var r = sack.Renderer( "test", -1, -1, 500, 500 );
var background = sack.Image( "images/stop_button.png" );

const color = sack.Image.Color( 0 );
r.setDraw( ( image )=>{	
	//image.fill( 0, 0, 0, 0, sack.Image.Color( 0x7f7f7f7f ) );
	if(1)
	for( i = 0;i < 255; i++ ) {
		color.a = i;
		color.g = 0;
		image.line( 0, i, 127, i, color );
		color.g = 128;
		image.line( 128, i, 256, i, color );
	}
	console.log( "Drew?" );
        image.drawImageOver( background );
	r.update();
	
} );

r.show();

