
var sack = require( ".." );

var background = sack.Image( "the rror.jpg" );

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
		this.scale = 1.0;
		this.x_del = 0; this.y_del = 0;;
		this._x = 0; this._y = 0; this._b = 0;
		return true;  // can return false/0 to disallow control creation.
	},
	destroy() {
	},
	draw( image ) {
		//console.log ("DRAWE WITH:", this, 0+this.x_del, 10+this.y_del, 10 * this.scale, 10 * this.scale, 0, 0, -1, -1 );
	        image.drawImage( background, 0+this.x_del, 10+this.y_del, 100 * this.scale, 100 * this.scale, 0, 0, -1, -1 );
		//console.log( "Update control surface..." );
		return true;
	},
	mouse( event ) {
		
		//console.log( "Mouse Event:", this.x_del, this.y_del, event.x, event.y, event.b );
		if( event.b & sack.PSI.button.scroll_up ) { 
			scale *= 1.1;
			this.redraw();
		} else if( event.b & sack.PSI.button.scroll_down ) { 
			scale *= 0.9;
			this.redraw();
		} else if( event.b & sack.PSI.button.left ) {
		if( !( this._b & sack.PSI.button.left ) ) {
			// first down;
			this._x = event.x;
			this._y = event.y;
		} else { 
			this.x_del += ( event.x - this._x );
			this.y_del += ( event.y - this._y );
			this.redraw();
		}
			this._x = event.x;
			this._y = event.y;
			this._b = event.b;
		} else {
			this._b = event.b;
		}
		return true;
	}
} );


var customShellControl = sack.InterShell.Control( { name : "JS Control"
	, control: customControl 
} );


var btn = sack.InterShell.Button( "JS/Button 1" );
btn.setClick( ()=>{
	console.log( "button clicked" );
} );

intershell.start();

console.log( "Should be able to dispatch some events?" );

//intershell.Page( "Another" );
//intershell.page.rows = 192;
//intershell.page.cols = 108;
//intershell.page.Button( 20, 20, 50, 50, "Done", ()=>process.exit(0) );
//intershell.page.Done
