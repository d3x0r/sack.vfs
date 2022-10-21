
import * as PRNG from "./prng_short.mjs"

const s = PRNG.getSeed4( "Seed" );
console.log( "Seeds:", s )

const bits = [];

function resetBits() {
	for( let i = 0;  i < 64; i++ ) {
		bits[i] = 0;
	}
}

function test( Rng ) {
	const rng = Rng( s );	
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
test( PRNG.SFC32_ );
resetBits();
test( PRNG.MUL32_ );
resetBits();
test( PRNG.XOR32_ );
resetBits();
test( PRNG.JSF32_ );
