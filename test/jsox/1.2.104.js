'use strict';
const SACK = require( "../.." );
const JSOX = SACK.JSOX;
describe('JSOX.forgiving leading `+` ', function () {
	it('accepts `+ 8`', function () {
		expect( JSOX.parse( '+ 8' ) ).to.equal( 8 );
	} );
	it('accepts `+ Infinity`', function () {
		expect( JSOX.parse( '+ Infinity' ) ).to.equal( Infinity );
	} );
	it('throws `123+44`', function () {
		expect( function() { JSOX.parse( '123+44' ); } ).to.throw( Error );
	} );
} );
