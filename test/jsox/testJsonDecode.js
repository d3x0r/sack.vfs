'use strict';
const SACK=require("../.." );
const JSOX = SACK.JSOX;
const parse = JSOX.parse;

describe('JSON decoding', function () {
	it('Unicode escapes', function () {
		const result = parse( '"\\u004D\\u004e\\u004F\\u0050"' );
		console.log( "MNOP=", result );
		expect(result).to.equal('MNOP');
	});
});
