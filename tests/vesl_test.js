
var sack = require( ".." );
var VESL = sack.VESL;
var file = sack.Volume().read( "vesl_hello.vsl" ).toString();
try {
	var p = VESL.parse( file );
	console.log( "Result:", p );
}catch(err) {
	console.log( "Parsing Faulted:\n", err );
}


function stop() { setTimeout( stop, 10000 ) };stop();