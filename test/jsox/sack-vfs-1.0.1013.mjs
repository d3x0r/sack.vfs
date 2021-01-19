import {sack} from "sack.vfs";
const JSOX = sack.JSOX;

describe('Added in 1.0.1013', function () {

	it( "parser instance parses simple message?", function() {
        	const parser = JSOX.begin();
		const o1 = parser.parse( '{op:worlds}' );
		const o2 = parser.parse( '{op:"world",world:{name:"My World"}}' );
		//console.log( o1, o2 );
		expect( o1 ).to.deep.equal( {op:"worlds"} );
                
		expect( o2 ).to.deep.equal( {op:"world",world:{name:"My World"}} );
        } );
} );
