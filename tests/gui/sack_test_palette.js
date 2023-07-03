
var sack = require( "../.." );

console.log( "Simply test color choice dialog" );

sack.Image.Color.dialog( (font)=>{
	process.exit(0);	
} );
