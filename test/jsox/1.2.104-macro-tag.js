'use strict';
const SACK=require("../.." );
const JSOX = SACK.JSOX;

/*
process.on("beforeExit", ()=>{ console.log( "EXITING" ) } );
process.on("uncaughtException",(a,b)=>{
	console.log( "test", a, b );
} );

function describe(a,b) { return b() };
function it(a,b) { return b() };
let threw = null;
function expect(a) { if( "function" === typeof a ) { try { threw = null; a(); } catch(err){threw=err}; }
					 return ({ to: {deep:{  equal(b) { console.log( "did",a,"=",b);  } }
					, equal(b) { console.log( "did",a,"=",b); }
				  , throw(a) {console.log( "Success:error?", threw ) } } }); }

*/

describe('Added in 1.2.104 Macro Tags', function () {
	it( 'handles macro tags', function() {
                const object = JSOX.parse( "vec{x,y} [vec{0,0}, vec{1,1}]" );
                //console.log( "object is:", object);//[{0,0},{1,1}]
                expect(object).to.deep.equal( [{x:0,y:0},{x:1,y:1}] );

	} );

	it( 'handles macro tags (with space)', function() {
		//console.log( "This is a false success..." );
                const object = JSOX.parse( "vec {x,y} [vec{0,0}, vec{1,1}]" );
                //console.log( "object is:", object);//[{0,0},{1,1}]
                expect(object).to.deep.equal( [{x:0,y:0},{x:1,y:1}] );

	} );

	it( 'handles stringifies to macro tags', function() {
        	const objectType = { face:null, suit:null };
                JSOX.defineClass( "card", objectType );

		const str = JSOX.stringify( [{face:0,suit:0},{face:1,suit:1}] ) 
		expect(str).to.equal( "card{face,suit}[card{0,0},card{1,1}]");
	} );

} );
