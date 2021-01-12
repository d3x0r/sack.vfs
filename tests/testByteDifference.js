var sack=require( ".." )

var tbl = [];

for( var n = 0; n < 256; n++ ) {
	var t;
        t = 0;
	for( var b =0; b < 8; b++ )
           if( n & ( 1 << b ) )
             t++;
        tbl.push(t);
}


var RNG = sack.SaltyRNG( (salt)=>{ salt.push(Date.now() ) } );
RNG.setVersion( 3 );
var t = 0;
var n;
var a = RNG.getBits(8);
for(  n = 0; n < 100000000; n++ ) {
	var b = RNG.getBits(8);
	var c = a^b;
	t += tbl[c];	

}
console.log( "total:", t, n, t / n );

console.log( "?", RNG.getBits( 8 ) );

console.log( "?", RNG.getBits( 8 ) );
console.log( "?", RNG.getBits( 8 ) );
console.log( "?", RNG.getBits( 8 ) );
console.log( "?", RNG.getBits( 8 ) );
console.log( "?", RNG.getBits( 8 ) );
console.log( "?", RNG.getBits( 8 ) );
console.log( "?", RNG.getBits( 8 ) );
console.log( "?", RNG.getBits( 8 ) );
console.log( "?", RNG.getBits( 8 ) );
