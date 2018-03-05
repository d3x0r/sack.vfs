
var sack = require( ".." );

var intershell = sack.InterShell( { 
	save( file ) {
		console.log( "save a global configuration parameter" );
		file.write( "Save Globally Some 3 string" );
	},
	load( config ) {
		config.addMethod( "some %i string", function ( n ) {
			console.log( "Recovered a configuration parameter...", n );
		} );
	},
} );

var customControl = sack.PSI.Registration( { 
	name: "image control",
	width: 256,
	height : 256,
	border : sack.PSI.control.border.bump,
	create() {
		return true;  // can return false/0 to disallow control creation.
	},
	destroy() {
	},
	draw( image ) {
	        //image.drawImage( background, 0+x_del, 100+y_del, 100 * scale, 100 * scale, 0, 0, -1, -1 );
		console.log( "Update control surface..." );
		return true;
	},
	mouse( event ) {
		console.log( "Mouse Event:", x_del, y_del, event.x, event.y, event.b );
		if( event.b & sack.PSI.button.scroll_up ) { 
			scale *= 1.1;
			this.redraw();
		} else if( event.b & sack.PSI.button.scroll_down ) { 
			scale *= 0.9;
			this.redraw();
		} else if( event.b & sack.PSI.button.left ) {
		if( !( _b & sack.PSI.button.left ) ) {
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


var customShellControl = sack.InterShell.Control( { name : "JS Control"
	, control: customControl 
} );


intershell.start();

//intershell.Page( "Another" );
//intershell.page.rows = 192;
//intershell.page.cols = 108;
//intershell.page.Button( 20, 20, 50, 50, "Done", ()=>process.exit(0) );
//intershell.page.Done
