'use strict';
const SACK = require( "../.." );
const JSOX = SACK.JSOX;
// 1.2.104 allowed a '+' prefix on numbers.  The prefix is part of the literal, so it
// has to be attached; the spaced forms this originally accepted are rejected as of
// 1.2.126 -- see 1.2.126-signs-and-recovery.js.
describe('JSOX.leading `+` prefix', function () {
	it('accepts `+8`', function () {
		expect( JSOX.parse( '+8' ) ).to.equal( 8 );
	} );
	it('accepts `+Infinity`', function () {
		expect( JSOX.parse( '+Infinity' ) ).to.equal( Infinity );
	} );
	it('throws `123+44`', function () {
		expect( function() { JSOX.parse( '123+44' ); } ).to.throw( Error );
	} );
} );
