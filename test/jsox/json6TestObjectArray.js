'use strict';
const SACK=require("../.." );
const JSOX = SACK.JSOX;
const JSON6 = SACK.JSOX;
const JSON = SACK.JSOX;
const parse = JSOX.parse;


describe('Objects and arrays', function () {
	it('Simple array with number', function () {
		const result = JSON6.parse( "[1234]" );
		expect(result).to.deep.equal([1234]);
	});
	it('Simple nested array with number', function () {
		const result = JSON6.parse( "[1,[2,[3,[4,5]]]]" );
		expect(result).to.deep.equal([1, [2, [3, [4, 5]]]]);
	});
	it('Array of objects', function () {
		const d = '[ {a: "", b: ""}, {a: "", b: ""} ]';
		const result = JSON6.parse( d );
		console.log( result );
		expect(result).to.deep.equal([ {a: "", b: ""}, {a: "", b: ""} ]);
	});
	it('Array with various types', function () {
		const d = '[true, false, -NaN, NaN, -Infinity, Infinity, undefined]';
		const result = JSON6.parse( d );
		console.log( result );
		expect(result).to.deep.equal([
			true, false, NaN, NaN, -Infinity, Infinity, undefined
		]);
	});

	// JSON6 will return g:undefined
	// Because of the potential for class and reviver routines to return undefined
	// really meaning 'forget this' undefined is filtered at this time.

	it('Object with various types', function () {
		const d = '{a: true, b: false, c: -NaN, d: NaN, e: -Infinity, f: Infinity, g: undefined, h: null}';
		const result = JSON6.parse( d );
		console.log( result );
		expect(result).to.deep.equal({
			a: true, b: false, c: -NaN, d: NaN, e: -Infinity, f: Infinity, g:undefined, h: null
		});
	});
	it('Array with empty object', function () {
		const d = '[{}]';
		const result = JSON6.parse( d );
		console.log( result );
		expect(result).to.deep.equal([
			{}
		]);
	});
	it('Array of objects and array', function () {
		const d = '[ {a: "", b: ""}, [1,2], {a: "", b: ""} ]';
		const result = JSON6.parse( d );
		console.log( result );
		expect(result).to.deep.equal([ {a: "", b: ""}, [1, 2], {a: "", b: ""} ]);
	});
	it('Object with child objects and arrays', function () {
		const d = '{ a:{a: "", b: ""}, b:[{d:"",e:""},{f:"",g:""}], c:{a: "", b: ""} }';
		const result = JSON6.parse( d );
		console.log( result );
		expect(result).to.deep.equal({
			a: {a: "", b: ""}, b: [{d:"",e:""}, {f:"",g:""}], c: {a: "", b: ""}
		});
	});
});
