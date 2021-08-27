'use strict';
const SACK=require("../.." );
const JSOX = SACK.JSOX;
const parse = JSOX.parse;

describe('JSON6.escape', function () {
	it('Escapes', function () {
		const str = JSOX.escape('a"b\\c`d\'e');
		expect(str).to.equal('a\\"b\\\\c\\`d\\\'e');
	});
	it('Handles empty string', function () {
		const str = JSOX.escape('');
		expect(str).to.equal('');
	});
});
