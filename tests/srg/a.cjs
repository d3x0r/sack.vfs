

const SRG=require( "sack.vfs" ).SaltyRNG;
console.log( "SRG?", SRG );
//import {SaltyRNG as SRG2} from "./salty_random_generator2.mjs";


const test1 = SRG( ()=>{}, {mode:1} );
//const test1a = SRG( ()=>{}, {mode:2} );

//const test2 = SRG2( ()=>{}, {mode:1} );
//const test2a = SRG2( ()=>{}, {mode:2} );

if(0)
for( let n = 0; n < 1000; n++ ) {
	console.log( "old k12");
	const b1 = test1.getBuffer(256);
	console.log( "new k12 only");
	const b2 = test2.getBuffer(256);
	console.log( "new k12 only(2)");
	const b2a = test2a.getBuffer(256);
	let c = 0;
	for( c = 0; c < 256;c++ ){
		if( b1[c] != b2[c] ) {
			console.log( "1 and 2 fail:", n, c, b1[c], b2[c] );
			break;
		}
		if( b1[c] != b1a[c] ) {
			console.log( "1 and 1a fail:", n, c, b1[c], b1a[c] );
			break;
		}
		if( b2[c] != b2a[c] ) {
			console.log( "2 and 2a fail:", n, c, b2[c], b2a[c] );
			break;
		}
	}
	
	if( c < 256 ) break;
}

//console.log("Made:", test1, test2 ) ;
console.log( "Test1:", SRG.id( "test" ) );
console.log( "Test1:", SRG.id( "test2" ) );
console.log( "Test1:", SRG.id( "test3" ) );
//console.log( "Test1:", SRG2.id( "test" ) );
