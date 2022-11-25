
'use strict';
const SACK=require("../.." );
const JSOX = SACK.JSOX;
/*
process.on("beforeExit", (asdf)=>{ console.log( "EXITING", asdf ) } );
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

describe('Added in 1.2.102', function () {



	it( 'handles ref revivals', function() {
		const w1 = {end:null,start:null};
		const w2 = {end:null,start:null};
		const w3 = {end:null,start:null};
		const w4 = {end:null,start:null};
		w1.start = w2;
		w1.end = w3;
		w2.start = w1;
		w2.end = w4;
		w3.start = w1;
		w3.end = w4;
		w4.start = w2;
		w4.end = w3;
		
		// {op:"move",walls:[{end:{end:{end:ref["walls",0,"end"],start:{end:ref["walls",0,"end","end"],start:ref["walls",0]}},start:ref["walls",0]},start:ref["walls",0,"end","end","start"]},ref["walls",0,"end","end","start"],ref["walls",0,"end"],ref["walls",0,"end","end"]]}
                // {op:"move",walls:[{end:{end:{end:ref["walls",0,"end"],start:{end:ref["walls",0,"end","end"],start:ref["walls",0,"end","end","start"]}},start:ref["walls",0,"end"]},start:ref["walls"]},ref["walls",0,"end","end","start"],ref["walls",0,"end"],ref["walls",0,"end","end"]]}
		const o = {op:"move", walls: [w1,w2,w3,w4] }
		const str = JSOX.stringify( o );

		//console.log( "RESULT:",str );
		const obj = JSOX.parse( str );
		//console.log( "RESULT:", obj );
		if( obj.walls[0].start === obj.walls[1] &&
		   obj.walls[0].end === obj.walls[2] &&
		  obj.walls[3].start === obj.walls[1] &&
		   obj.walls[3].end === obj.walls[2] &&
		  obj.walls[1].start === obj.walls[0] &&
		  obj.walls[1].end === obj.walls[3] &&
		  obj.walls[2].start === obj.walls[0] &&
		  obj.walls[1].end === obj.walls[3] ) {
		 	/* success */;
		} else {
			console.log( "Failed TEST:", obj, o );
			throw new Error( "Object Mis-Match") ;
		}
		const str2 = JSOX.stringify( obj );
		//console.log( "got:", str );
		expect( str2 ).to.equal( str2 );

	} );
        
} );
