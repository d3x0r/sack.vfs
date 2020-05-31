'use strict';
const sack = require( "../.." );
const JSON6 = sack.JSON6;

describe('JSON decoding', function () {
	it('Unicode escapes', function () {
		const result = JSON6.parse( '"\\u004D\\u004e\\u004F\\u0050"' );
		//console.log( "MNOP=", result );
		expect(result).to.equal('MNOP');
	});
});
