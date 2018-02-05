
var vfs = require( '..' );
var SRG = vfs.SaltyRNG();

for( var n = 0; n < 10; n++ ) {
	console.log( "n:", SRG.getBits() );
}
var start = Date.now();
for( var n = 0; n < 100000; n++ ) {
	SRG.getBits();
}
var end = Date.now();
console.log( n, "(32 bits) in ", end-start, n/(end-start));

SRG=vfs.SaltyRNG( (salt)=>{salt.push(1);} );

var start = Date.now();
for( var n = 0; n < 100000; n++ ) {
	SRG.getBits();
}
var end = Date.now();
console.log( n, "(32 bit signed) in ", end-start, n/(end-start));

var start = Date.now();
for( var n = 0; n < 100000; n++ ) {
	SRG.getBuffer(1000);
}
var end = Date.now();
console.log( n, "(1000 bits) in ", end-start, n/(end-start));

return

for( var n = 0; n < 10; n++ ) {
	console.log( "n:", SRG.getBits(8) );
}

for( var n = 0; n < 10; n++ ) {
	console.log( "n:", SRG.getBits(8,true) );
}
