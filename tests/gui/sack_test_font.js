
var sack = require( ".." );

sack.Image.Font.dialog( (font)=>{
	console.log( "Font:", font );
	process.exit(0);		
} );
