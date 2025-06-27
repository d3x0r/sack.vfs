
var sack = require( "../.." );

sack.Image.Font.dialog( (font)=>{
	console.log( "Font:", font );
	process.exit(0);		
} );

// some sort of f.wait() event is needed... maybe it's a promise that only resolves on close?
function tick() { setTimeout( tick, 50000 );} tick();
