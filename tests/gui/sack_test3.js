
var sack = require( ".." );

console.log( "got", sack, Object.keys( sack.Image.colors ));

var _b = 0;
var x_click = 0;
var y_click = 0;
var _x = 0;
var _y = 0;
var _x_del = 0;
var _y_del = 0;
var x_del = 0;
var y_del = 0;
var _scale = 1.0;
var scale = 1.0;

var display = sack.Renderer.getDisplay( 0 );
console.log( "display 0:", display );
for( var n = 1; n < 8; n++ ) {
	console.log( "display %d:", n, sack.Renderer.getDisplay( n ) );
}
var r = sack.Renderer( "test", 0, 0, display.width, display.height );
console.log( "created renderer?", r, Object.keys( Object.getPrototypeOf(r)) );
var background = sack.Image( "the rror.jpg" );
r.on( "draw", ( image )=>{	
	//console.log( "It wanted a draw...", 100+y_del, image, Object.keys(Object.getPrototypeOf(image)) ) 
	if( _x_del )
	        image.fill( 0+_x_del, 100+_y_del, 100 * _scale, 100 * _scale, sack.Image.colors.purple );
	
	//console.log( "draw del is: ", x_del, y_del );
        image.drawImage( background, 0+x_del, 100+y_del, 100 * scale, 100 * scale );
        image.drawImage( background, 0+x_del, 100+y_del, 100 * scale, 100 * scale, 0, 0, 50, 50 );
	{
	var n = 0;
	var now = Date.now();
	for( x = 0; x < 160; x++ )
		for( y = 0; y < 180; y++ ) {
		        image.drawImage( background, scale*x*16+0+x_del, scale*y*16+100+y_del, 16 * scale, 16 * scale, 0, 0, 50, 50 );
			n++
		}
	}
	var del = Date.now() - now;
console.log( n, "in", del, " ", 16*n/del, " in 16ms(60fps)" );
	//r.update( 0+x_del, 100+y_del, 100 * scale, 100 * scale );
	_x_del = x_del;
	_y_del = y_del;
	_scale = scale;
} );

r.on( "mouse", ( event )=>{	
	if( event.b & sack.PSI.button.scroll_up ) { 
		scale *= 1.1;
		r.redraw();
	} else if( event.b & sack.PSI.button.scroll_down ) { 
		scale *= 0.9;
		r.redraw();
	} else if( event.b & sack.PSI.button.left ) {
		if( !( _b & sack.PSI.button.left ) ) {
			// first down;
			x_click = event.x;
			y_click = event.y;
		} else { 
			x_del += ( event.x - _x );
			y_del += ( event.y - _y );
			r.redraw();
		}
		//console.log( "maus Del is: ", x_del, y_del );
		_x = event.x;
		_y = event.y;
	}
	_b = event.b;
} );

r.show();

console.log( 'going to call close?!' );

var process = require( 'process' );
process.on('exit', function (){
  console.log('Goodbye!');
  r.close();
});

process.on('SIGINT', function (){
  console.log('Goodbye!');
  r.close();
});
