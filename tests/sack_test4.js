
var sack = require( ".." );

console.log( "got", sack );

var _b = 0;
var x_click = 0;
var y_click = 0;
var _x = 0;
var _y = 0;
var x_del = 0;
var y_del = 0;
var scale = 1.0;

var f = sack.Frame( "test", -1, -1, 600, 600 );
console.log( "created frame?", f, Object.keys( Object.getPrototypeOf( f ) ) );
var background = sack.Image( "the rror.jpg" );

console.log( "created frame?", background, JSON.stringify( background ), background.width, background.height );

var b = f.Control( "Button", "Test", 10, 10, 100, 20 );


b.on( "click", function() {
	process.exit();
} );

b.text = "Quit...";

var x = sack.Image.Color( {r:50,g:50,b:255,a:255} );
console.log( "color:",   Object.getPrototypeOf( sack.Image.colors.white ), sack.Image.colors.white);//, sack.Image.colors.white.r );
console.log( "color:",  Object.keys(Object.getPrototypeOf( x )), x.r, x.toString() );

console.log( "b has:", Object.keys( Object.getPrototypeOf( b ) ), sack.Image.colors.white.toString(), b.text, b.layout, b );



var customControl = sack.Registration( { 
	name: "image control",
	width: 256,
	height : 256,
	border : sack.control.border.bump,
	create() {
		return true;  // can return false/0 to disallow control creation.
	},
	draw( image ) {
	        image.drawImage( background, 0+x_del, 100+y_del, 100 * scale, 100 * scale, 0, 0, -1, -1 );
		return true;
	},
	mouse( event ) {
		//console.log( "Mouse Event:", x_del, y_del, event.x, event.y, event.b );
		if( event.b & sack.button.scroll_up ) { 
			scale *= 1.1;
			this.redraw();
		} else if( event.b & sack.button.scroll_down ) { 
			scale *= 0.9;
			this.redraw();
		} else if( event.b & sack.button.left ) {
		if( !( _b & sack.button.left ) ) {
			// first down;
			x_click = event.x;
			y_click = event.y;
		} else { 
			x_del += ( event.x - _x );
				y_del += ( event.y - _y );
				this.redraw();
			}
			_x = event.x;
			_y = event.y;
			_b = event.b;
		} else {
			_b = event.b;
		}
		return true;
	}
} );
console.log( "created custom control registration?", Object.keys(Object.getPrototypeOf(customControl)) );

f.Control( "image control", 0, 40, 500, 500 );

f.show();

var process = require( 'process' );
process.on('exit', function (){
  console.log('Goodbye!');
  f.close();
});

process.on('SIGINT', function (){
  console.log('Goodbye!');
  f.close();
});
