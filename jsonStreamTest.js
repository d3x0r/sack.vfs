
var vfs = require( "." );
var parser = vfs.JSON6.begin( (obj)=>{
	console.log( "Got value:",typeof obj, ":", obj );
} );


parser.write( 'true false null undefined NaN Infinity' );

parser.write( "1" );
parser.write( "0x123" );
parser.write( '"1"' );

parser.write( '{ a:1234 }' );
console.log( "4 objects..." );
parser.write( '{ a:1234 }{ b:34 }{c:1}{d:123}' );
console.log( "got 4 objects?" );

try {
	parser.write( 'truefalse' );
} catch(err) {
	console.log( "success error" );
}

parser.write( '1_234 0x55_33_22_11 0x1234' );
