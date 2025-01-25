
'use strict';
const sack=require("../.." );
//import {sack} from "../.."
const JSOX = sack.JSOX;


describe('Added in JSOX@1.2.117', function () {
	// this already worked in sack.vfs parser... 
	it( 'recovers 0 reference', function() {
		const string = '[{self:ref[0]}]';
		const expects = JSOX.parse(string);
		console.log( "Expects to not be an error:", expects );
		const recurs = [{self:null}];
		recurs[0].self = recurs[0];
		//console.log( "Expects to not be an error:", expects );
		expect( expects ).to.deep.equal( recurs );

	} );
        
} );
