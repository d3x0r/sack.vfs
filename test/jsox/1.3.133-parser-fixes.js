'use strict';

const SACK=require("../.." );
const JSOX = SACK.JSOX;

describe('Added in 1.3.133 - parser and stringifier fixes', function () {

	describe( 'class names', function () {
		// The class lookup compared only as many bytes as the name being looked up has,
		// and never checked the stored name's length, so a longer name matched any
		// prefix of itself: with 'AB' defined, a reference to 'A' found 'AB'.
		it( 'does not match a longer class name on its prefix', function () {
			// 'A{p}' defines a second class; it does not set AB's first field,
			// which is what used to make this { x:'p' }
			expect( JSOX.parse( 'AB{x,y}A{p}A{1}' ) ).to.deep.equal( { p:1 } );
			// and with nothing after the two definitions there is no value at all,
			// where the prefix match used to produce one
			expect( () => JSOX.parse( 'AB{x,y}A{p}' ) ).to.throw( Error );
		} );
		it( 'does not match a shorter class name either', function () {
			expect( JSOX.parse( 'A{p}AB{x,y}AB{1,2}' ) ).to.deep.equal( { x:1, y:2 } );
		} );
		it( 'still finds a class by its own name', function () {
			expect( JSOX.parse( 'pt{x,y}pt{1,2}' ) ).to.deep.equal( { x:1, y:2 } );
			expect( JSOX.parse( 'a{p}b{q}a{1}' ) ).to.deep.equal( { p:1 } );
			expect( JSOX.parse( 'a{p}b{q}b{2}' ) ).to.deep.equal( { q:2 } );
		} );
		it( 'keeps names apart when many are defined', function () {
			// the names go through a hash table now, so collisions have to resolve
			const defs = [], refs = {};
			for( let n = 0; n < 500; n++ ) {
				defs.push( 'c' + n + '{f' + n + '}' );
				refs[ 'f' + n ] = n;
			}
			// every definition resolves to its own field name
			for( let n = 0; n < 500; n += 97 ) {
				const r = JSOX.parse( defs.join( '' ) + 'c' + n + '{' + n + '}' );
				expect( r ).to.deep.equal( { [ 'f' + n ]:n } );
			}
		} );
		it( 'still recovers a tagged object that is not a class definition', function () {
			// `Tag{a:1}` opens looking like a definition and the first ':' reveals it is
			// a tagged object; the half-made class is dropped, index included
			expect( JSOX.parse( 'Tag{a:1}' ) ).to.deep.equal( { a:1 } );
			// two of them in one document keeps the first, as it did before
			expect( JSOX.parse( 'T1{a:1}T2{b:2}' ) ).to.deep.equal( { a:1 } );
		} );
	} );

	describe( 'typed array views', function () {
		// the converters encoded view.buffer, so a view of part of a buffer came back
		// as the whole array it was cut from
		it( 'serializes only the bytes a view covers', function () {
			const v = new Uint8Array( [ 10, 20, 30, 40 ] ).subarray( 1, 3 );
			const r = JSOX.parse( JSOX.stringify( v ) );
			expect( r ).to.be.instanceof( Uint8Array );
			expect( Array.from( r ) ).to.deep.equal( [ 20, 30 ] );
		} );
		it( 'honours byteOffset for wider element types', function () {
			const u16 = new Uint16Array( [ 1, 2, 3, 4 ] ).subarray( 1, 3 );
			expect( Array.from( JSOX.parse( JSOX.stringify( u16 ) ) ) ).to.deep.equal( [ 2, 3 ] );
			const f64 = new Float64Array( [ 1.5, 2.5, 3.5 ] ).subarray( 1 );
			expect( Array.from( JSOX.parse( JSOX.stringify( f64 ) ) ) ).to.deep.equal( [ 2.5, 3.5 ] );
			const i32 = new Int32Array( new ArrayBuffer( 24 ), 8, 2 );
			i32[0] = -7; i32[1] = 9;
			expect( Array.from( JSOX.parse( JSOX.stringify( i32 ) ) ) ).to.deep.equal( [ -7, 9 ] );
		} );
		it( 'still round-trips a whole array and a bare buffer', function () {
			expect( Array.from( JSOX.parse( JSOX.stringify( new Uint8Array( [ 1, 2, 3 ] ) ) ) ) )
				.to.deep.equal( [ 1, 2, 3 ] );
			const ab = JSOX.parse( JSOX.stringify( new Uint8Array( [ 4, 5 ] ).buffer ) );
			expect( Array.from( new Uint8Array( ab ) ) ).to.deep.equal( [ 4, 5 ] );
		} );
	} );

	describe( 'many class definitions', function () {
		// The name lookup is a hash table now rather than a scan per definition, which
		// took 32,000 definitions from 838ms to 310ms.  There is no timing assertion
		// here: AddLink() is still O(n) in the list it appends to (see
		// src/typelib/typecode.c), so this document is still quadratic overall and any
		// ratio a test could check would encode that curve rather than guard it.
		it( 'gives every definition its own fields', function () {
			const parts = [];
			for( let i = 0; i < 4000; i++ ) parts.push( 'A' + i + '{f' + i + '}' );
			const doc = parts.join( '' );
			expect( JSOX.parse( doc + 'A0{1}' ) ).to.deep.equal( { f0:1 } );
			expect( JSOX.parse( doc + 'A2500{1}' ) ).to.deep.equal( { f2500:1 } );
			expect( JSOX.parse( doc + 'A3999{1}' ) ).to.deep.equal( { f3999:1 } );
		} );
	} );

	describe( 'a failed parse keeps nothing', function () {
		// The error text was created on demand on the parse state and never destroyed,
		// so each failed parse leaked a VARTEXT: 5354 bytes a failure, measured over
		// 80,000 of them, which took the process from 68MB to 477MB.  It is 1098 bytes
		// now -- not nothing, and what is left needs a heap profiler to place, so this
		// guards the rate rather than claiming none.
		it( 'does not leak the error text over repeated failures', function () {
			if( !global.gc ) return this.skip(); // needs --expose-gc
			this.timeout( 60000 );
			const sample = () => { global.gc(); global.gc(); return process.memoryUsage().rss; };
			for( let i = 0; i < 5000; i++ ) { try { JSOX.parse( '[' ); } catch( err ) { /* expected */ } }
			const before = sample();
			const N = 20000;
			for( let i = 0; i < N; i++ ) { try { JSOX.parse( '[' ); } catch( err ) { /* expected */ } }
			const each = ( sample() - before ) / N;
			expect( each ).to.be.below( 2500 );   // was 5354, is 1098
		} );
		it( 'still reports each failure', function () {
			expect( () => JSOX.parse( '[' ) ).to.throw( Error );
			expect( () => JSOX.parse( '{a:1' ) ).to.throw( Error );
			// and a good parse after a bad one still works
			expect( JSOX.parse( '[1,2]' ) ).to.deep.equal( [ 1, 2 ] );
		} );
	} );
} );
