
var sack= require( ".." );

var signThis = "Some message Content with some sort of packet { name:\"beans\", qty:1234 }";
sack.SaltyRNG.setSigningThreads( 12 );

console.log("verify:", sack.SaltyRNG.verify(signThis, 'Qk1fYNORquwmRggYaht0_Rl_vZOGP2j3ZpuGVh$Z97c=' ) );
console.log("verify:", sack.SaltyRNG.verify(signThis, 'ApMNmPBfSiNHJrAD$6lrqTcA4KSN8mTxqrSiDrSrJH4='));

var start = Date.now();
var avg = 0;

for( var n =0; n < 10000; n++ ) {
	var id = sack.SaltyRNG.sign( signThis, 3, 3 );
	var keyType = 0;
	if( !( keyType = sack.SaltyRNG.verify( signThis, id, 3, 3 ) ) )
		console.log( id, "non reproducable" );
	else console.log( id, "keyType:", keyType );
	avg += Date.now() - start;
	console.log( "Tick:", Date.now() - start, avg/(n+1) );
	start = Date.now();
}

