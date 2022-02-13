'use strict';
const SACK=require("../.." );
const JSOX = SACK.JSOX;
const JSON6 = SACK.JSOX;
const JSON = SACK.JSOX;
const parse = JSOX.parse;

describe('Object keys', function () {
	describe('Erring', function () {
		it('Unencapsulated hyphenated key with space', function () {
			expect(function () {
				const result = JSON6.parse( "{ my- key:3}" );
			}).to.throw(Error);
		});

		it('Unencapsulated hyphenated key with spaces', function () {
			expect(function () {
				JSON6.parse( "{ my -  key:3}" );
			}).to.throw(Error);
		});

		/* JSOX handles this by reviving as a class of type 'my-key'
		it('Unencapsulated hyphenated key with nested object', function () {
			expect(function () {
				const result = JSON6.parse( "{ my-key { failure:true}:3}" );
				console.log( "result:", result );
			}).to.throw(Error);
		});
		*/
		
		/* JSOX handles this as a valid string... 		
		it('Unencapsulated key with nested object', function () {
			expect(function () {
				const result = JSON6.parse( "{ { my-key:3 } }" );
				console.log( "result:", result );
			}).to.throw(Error);
		});
		*/

		it('Unencapsulated key with nested array', function () {
			expect(function () {
				const result = JSON6.parse( "{ [ my-key:3 } }" );
			}).to.throw(Error);
		});

		it('Unencapsulated key with opening array bracket', function () {
			expect(function () {
				const result = JSON6.parse( "{ my-key[:3 } }" );
			}).to.throw(Error);
		});

		it('Unencapsulated key with closing array bracket', function () {
			expect(function () {
				const result = JSON6.parse( "{ my-key]:3 } }" );
			}).to.throw(Error);
		});

		it('Unencapsulated key with opening array bracket and space', function () {
			expect(function () {
				const result = JSON6.parse( "{ my-key [:3 } }" );
			}).to.throw(Error);
		});

		it('Unencapsulated key with closing array bracket and space', function () {
			expect(function () {
				const result = JSON6.parse( "{ my-key ]:3 } }" );
			}).to.throw(Error);
		});

		it('Key with quote not at beginning', function () {
			expect(function () {
				JSON6.parse( "{A': 3}" );
			}).to.throw(Error, /unexpected end of stream/);
		});
	});

	describe('Functional', function () {
		it('Parses encapsulated key', function () {
			const result = JSON6.parse( "{ 'my  -  key':3}" );

			expect(result).to.deep.equal({
				'my  -  key': 3
			});
		});

		it('Parses encapsulated key with carriage return', function () {
			const result = JSON6.parse( "{ '\\\rmy  -  key':3}" );

			expect(result).to.deep.equal({
				'my  -  key': 3
			});
		});

		it('Parses key with special characters but no spaces', function () {
			const result = JSON6.parse( "{ my-key\\m&m+*|:3}" );
			expect(result).to.deep.equal({
				'my-key\\m&m+*|' : 3
			});
		});

		it('Parses key with special characters and comment', function () {
			const result = JSON6.parse( "{ my-key //test \n :3}" );
			/*
			{ my-key //test
			   :3}   // valid
			   */
			expect(result).to.deep.equal({
				'my-key': 3
			});
		});

		it('Parses key with special characters and multi-line comment', function () {
			const result = JSON6.parse( "{ my-key /*test */ :3}" );

			/*
			{ my-key /*test * / :3}  // valid
			*/
			expect(result).to.deep.equal({
				'my-key': 3
			});
		});

		it('Parses key with ZWNBS (ignoring it)', function () {
			const result = JSON6.parse( "{A\uFEFF: 3}" );

			expect(result).to.deep.equal({
				'A': 3
			});
		});
	});
});
