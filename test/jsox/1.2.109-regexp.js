'use strict';
const SACK=require("../.." );
const JSOX = SACK.JSOX;


describe('Added in 1.2.109 (regexp)', function () {

	it( 'handles stringify', function() {

		const obj = /a+/;
		const strObj = JSOX.stringify( obj );
		expect( strObj ).to.equal( "regex'a+'" );
		//expect( strObj ).to.equal( "Ρεχ'a+'" );
	});

	it( 'handles parse', function() {
		const str = "regex'a+'";
		const objStr = JSOX.parse( str );
		expect( objStr).to.deep.equal( /a+/ );
	} );

} );