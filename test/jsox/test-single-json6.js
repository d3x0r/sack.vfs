'use strict';
const SACK=require("../.." );
const JSOX = SACK.JSOX;
const JSON6 = SACK.JSOX;
const JSON = SACK.JSOX;
const parse = JSOX.parse;

describe('Single JSON6', function () {
	it('Single JSON6', function () {
		const obj = JSON6.parse( "{ asdf : 1234 } " );
		console.log( "Got:", obj );
		expect(obj).to.deep.equal({
			asdf: 1234
		});
	});
});
