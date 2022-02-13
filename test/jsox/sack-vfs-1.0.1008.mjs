import {sack} from "sack.vfs";
const JSOX = sack.JSOX;
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

describe('Added in 1.0.1008', function () {

	it( "parses simple message?", function() {
        
		const o1 = JSOX.parse( '{op:worlds}' );
		const o2 = JSOX.parse( '{op:"world",world:{name:"My World"}}' );
        } );
} );
