'use strict';

const SACK=require("../.." );
const JSON6 = SACK.JSOX;
const JSON = SACK.JSOX;
const JSOX = SACK.JSOX;
const parse = JSOX.parse;

let o;

/*
process.on("beforeExit", ()=>{ console.log( "EXITING" ) } );
process.on("uncaughtException",(a,b)=>{
	console.log( "test", a, b );
} );

function describe(a,b) { console.log( "doing:", a ); return b() };
function it(a,b) { console.log( "test:", a ); return b() };
let threw = null;
function expect(a) { if( "function" === typeof a ) { try { threw = null; a(); } catch(err){threw=err}; }
					 return ({ to: {deep:{  equal(b) { console.log( "did",a,"=",b);  } }
					, equal(b) { console.log( "did",a,"=",b); }
				  , throw(a) {console.log( "Success:error?", threw ) } } }); }

*/

describe('Incomplete String Escape tests', function () {



	it('Parses string octal escape followed by character', function () {
		const result = JSOX.parse( '"\\012"' );
		expect(result).to.equal('\0' + '12');
	expect(parse( "'\\x1'" )).to.equal( "\x01" );

	expect( parse( "'\\u31'" ) )

/*
function () {
		o = parse( "'\\u31'" );
		console.log( "got back:", o );
	})*/.to.equal( "\u0031" );//throw(Error);

	expect( parse( "'\\u{0'" )).to.equal("\0");


	});
} );
