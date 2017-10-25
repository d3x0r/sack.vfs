
var sack = require( ".." );

var customControl = sack.InterShell.Control( { name : "JS Control",
	create() {
		
	},
	destroy() {
		
	},
	draw( image ) {
		
	},
	mouse( event ) {
		
	},
} );

var intershell = sack.InterShell( { save( file ) {
		file.write( "Some 3 string" );
	},
	load( config ) {
		config.addMethod( "some %i string", function ( n ) {
			
		} );
	},
} );

intershell.start();
