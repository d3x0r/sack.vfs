'use strict';

const SACK=require("../.." );
const JSOX = SACK.JSOX;

describe('Added in 1.3.133 - a literal __proto__ key is data, not a prototype', function () {

	// Object::Set() is an ordinary assignment, and '__proto__' is the one name
	// Object.prototype carries an accessor for, so the field set the parsed object's
	// prototype instead of becoming a field: Object.keys(), JSOX.stringify() and any
	// schema check saw nothing, while a plain read of the field still returned the
	// document's value.  A primitive there was dropped outright.

	it( 'does not let an object value become the prototype', function () {
		const o = JSOX.parse( '{"__proto__":{"x":1}}' );
		expect( Object.getPrototypeOf( o ) ).to.equal( Object.prototype );
		expect( Object.keys( o ) ).to.deep.equal( [ '__proto__' ] );
		expect( o.__proto__ ).to.deep.equal( { x:1 } );
	} );
	it( 'keeps the field visible to every reader', function () {
		const o = JSOX.parse( '{"__proto__":{"isAdmin":true},"user":"bob"}' );
		expect( Object.keys( o ) ).to.deep.equal( [ '__proto__', 'user' ] );
		expect( o.isAdmin ).to.equal( undefined );
		expect( Object.prototype.hasOwnProperty.call( o, '__proto__' ) ).to.equal( true );
	} );
	it( 'does not drop a primitive there', function () {
		expect( JSOX.parse( '{"__proto__":5}' ).__proto__ ).to.equal( 5 );
		expect( JSOX.parse( '{"__proto__":"s"}' ).__proto__ ).to.equal( "s" );
		expect( JSOX.parse( '{"__proto__":null}' ).__proto__ ).to.equal( null );
	} );
	it( 'handles it unquoted, nested, as an array and as a class field', function () {
		const n = JSOX.parse( '{a:{__proto__:{y:2}}}' );
		expect( Object.getPrototypeOf( n.a ) ).to.equal( Object.prototype );
		expect( n.a.__proto__ ).to.deep.equal( { y:2 } );
		expect( JSOX.parse( '{"__proto__":[1,2]}' ).__proto__ ).to.deep.equal( [ 1, 2 ] );
		const c = JSOX.parse( 'p{__proto__,b}p{{"x":1},2}' );
		expect( c.__proto__ ).to.deep.equal( { x:1 } );
		expect( c.b ).to.equal( 2 );
	} );
	it( 'round-trips, and agrees with JSON.parse', function () {
		const doc = '{"__proto__":{"x":1},"b":2}';
		const o = JSOX.parse( doc );
		// this used to stringify as '{b:2}' -- the data was simply gone
		expect( JSOX.stringify( o ) ).to.contain( '__proto__' );
		expect( JSOX.parse( JSOX.stringify( o ) ).__proto__ ).to.deep.equal( { x:1 } );
		expect( JSON.stringify( o ) ).to.equal( JSON.stringify( JSON.parse( doc ) ) );
	} );
	it( 'leaves other inherited names, and Object.prototype, alone', function () {
		// only '__proto__' has an accessor on Object.prototype; the rest are plain data
		// properties that assignment already shadows, which is why there is no list of
		// names here and every other field keeps the fast path
		const o = JSOX.parse( '{"constructor":1,"toString":2,"valueOf":3}' );
		expect( Object.keys( o ) ).to.deep.equal( [ 'constructor', 'toString', 'valueOf' ] );
		JSOX.parse( '{"__proto__":{"polluted":"yes"}}' );
		expect( ( {} ).polluted ).to.equal( undefined );
		expect( Object.prototype.polluted ).to.equal( undefined );
	} );
} );
