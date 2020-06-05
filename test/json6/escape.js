'use strict';
const sack = require( "../.." );
const JSON6 = sack.JSON6;

describe('JSON6.escape', function () {
	it('Escapes', function () {
		const str = JSON6.escape('a"b\\c`d\'e');
		expect(str).to.equal('a\\"b\\\\c\\`d\\\'e');
	});
	it('Handles empty string', function () {
		const str = JSON6.escape('');
		expect(str).to.equal('');
	});
});
