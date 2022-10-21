
import * as PRNG from "./prng_short.mjs"

const bits = [];

function resetBits() {
	for( let i = 0;  i < 64; i++ ) {
		bits[i] = 0;
	}
}

function test( Rng ) {
	const rng = Rng( "Seed" );	
	const start = Date.now();
	for( let i = 0; i < 1000000; i++ ) {
		const val = rng()*0xFFFFFFFF;
//	console.log( "got:", val );
		for( let bit = 0; bit < 32; bit++ ) {
			if( val & (1<<bit) ) bits[bit*2]++;
			else bits[bit*2+1]++;
		}
	}
	const end = Date.now();
	console.log( 'bits:', bits, end-start );
}

resetBits();
test( PRNG.SFC32 );
resetBits();
test( PRNG.MUL32 );
resetBits();
test( PRNG.XOR32 );
resetBits();
test( PRNG.JSF32 );
