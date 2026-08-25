/**
 * Events2 regression suite.
 *
 * Every case here corresponds to a bug that was live at some point, and several
 * of them were mutually masking -- while dispatch threw, no ordering test could
 * run at all, so the priority bugs were invisible underneath it.  That is the
 * argument for running the whole set rather than the one you are working on.
 *
 * A browser will happily serve a stale cached copy of events2.mjs, which is how
 * a completely broken version can look fine in an app for hours.  Run this in
 * node, where there is no such cache.
 *
 *   node apps/events/tests/test_events2.mjs
 */

import { Events } from "../events2.mjs";

class E extends Events {}

let pass = 0, fail = 0;

function check( name, ok, detail ) {
	ok ? pass++ : fail++;
	console.log( ( ok ? "  ok  " : "FAIL  " ) + name.padEnd( 30 ) + ( detail || "" ) );
}

function expect( name, fn, want ) {
	try {
		const got = fn();
		check( name, got === want, got + ( got === want ? "" : "   want " + want ) );
	} catch( err ) {
		check( name, false, "THREW: " + err.message );
	}
}

// -- dispatch --------------------------------------------------------------

{
	const e = new E();
	let hits = 0;
	e.on( "tick", () => hits++ );
	e.on( "tick", [ "a" ] );
	check( "dispatch calls handler", hits === 1, "hits=" + hits );
}

// -- removal ---------------------------------------------------------------
// on() returns an EventHandle that knows its own list, so off(handle) can
// splice directly instead of searching by name and function.

{
	const e = new E();
	let hits = 0;
	const handle = e.on( "tick", () => hits++ );
	check( "on() returns a handle", handle && handle.constructor.name === "EventHandle",
	       handle && handle.constructor.name );
	check( "handle knows its list", Array.isArray( handle && handle.list ) );

	e.on( "tick", [ 1 ] );
	e.off( handle );
	e.on( "tick", [ 1 ] );
	check( "off(handle) unsubscribes", hits === 1, "hits=" + hits );
}

{
	const e = new E();
	let hits = 0;
	const fn = () => hits++;
	e.on( "tick", fn );
	e.on( "tick", [ 1 ] );
	e.off( "tick", fn );
	e.on( "tick", [ 1 ] );
	check( "off(evt,fn) unsubscribes", hits === 1, "hits=" + hits );
}

{
	// Removing from the middle must not disturb its neighbours.
	const e = new E();
	const seen = [];
	e.on( "m", () => seen.push( 1 ) );
	const mid = e.on( "m", () => seen.push( 2 ) );
	e.on( "m", () => seen.push( 3 ) );
	e.off( mid );
	e.on( "m", [ 0 ] );
	check( "remove from middle", seen.join( "," ) === "1,3", seen.join( "," ) );
}

// -- priority --------------------------------------------------------------
// Higher runs earlier; equal priorities keep registration order.

const order = ( regs ) => () => {
	const seen = [], e = new E();
	for( const [ name, priority ] of regs )
		priority === undefined
			? e.on( "go", () => seen.push( name ) )
			: e.on( "go", () => seen.push( name ), priority );
	e.on( "go", [ 1 ] );
	return seen.join( "," );
};

// The first registration used to skip the priority assignment entirely.
expect( "first registration's priority", order( [ [ "low", -5 ], [ "high", 10 ], [ "mid", 0 ] ] ),
        "high,mid,low" );
expect( "first registered is highest", order( [ [ "high", 10 ], [ "mid", 0 ], [ "low", -5 ] ] ),
        "high,mid,low" );
expect( "equal priorities keep order", order( [ [ "a", 0 ], [ "b", 0 ], [ "c", 0 ] ] ),
        "a,b,c" );
expect( "no priority given", order( [ [ "a" ], [ "b" ], [ "c" ] ] ),
        "a,b,c" );
expect( "negative runs last", order( [ [ "a", 0 ], [ "z", -99 ], [ "b", 0 ] ] ),
        "a,b,z" );
// Registered c,a,d,b -- an off-by-one in the insert showed up only here.
expect( "dense priorities sort", order( [ [ "c", 1 ], [ "a", 3 ], [ "d", 0 ], [ "b", 2 ] ] ),
        "a,b,c,d" );

/*
 * Strictly descending registration is the worst case for the insert: every new
 * handler is lower than everything present, so findIndex returns -1 every time.
 * splice(-1,...) counts from the END, so it lands second-from-last -- and on a
 * one-element list it clamps to 0 and behaves as unshift, flipping the pair.
 * Without the `index < 0 ? length : index` guard this comes out "-2,-3,-4,-1":
 * the highest-priority handler running LAST.
 */
expect( "descending registration",
        order( [ [ "p1", -1 ], [ "p2", -2 ], [ "p3", -3 ], [ "p4", -4 ] ] ),
        "p1,p2,p3,p4" );
expect( "ascending registration",
        order( [ [ "p4", -4 ], [ "p3", -3 ], [ "p2", -2 ], [ "p1", -1 ] ] ),
        "p1,p2,p3,p4" );

// -- payloads --------------------------------------------------------------
// An array payload is spread into the callback's parameters, so passing an
// array AS one argument means wrapping it twice.

{
	const e = new E();
	const got = [];
	e.on( "x", ( v ) => got.push( Array.isArray( v ) ? "array(" + v.length + ")" : String( v ) ) );
	e.on( "x", [ "one" ] );
	e.on( "x", [ [ 1, 2, 3 ] ] );
	check( "array payload spreads", got.join( "|" ) === "one|array(3)", got.join( "|" ) );
}

{
	const e = new E();
	const two = [];
	e.on( "y", ( a, b ) => two.push( a + ":" + b ) );
	e.on( "y", [ "p", "q" ] );
	check( "multiple arguments", two.join( "" ).startsWith( "p:q" ), two.join( "" ) );
}

// -- results ---------------------------------------------------------------
// Dispatch collects every return value, and each handler can see the ones
// before it.

{
	const e = new E();
	let sawPrevious = null;
	e.on( "q", () => "first" );
	e.on( "q", ( v, previous ) => { sawPrevious = previous && previous.slice(); return "second"; } );
	const results = e.on( "q", [ 1 ] );
	check( "results are collected",
	       Array.isArray( results ) && results.join( "," ) === "first,second", String( results ) );
	check( "earlier results visible",
	       sawPrevious && sawPrevious.join( "," ) === "first", String( sawPrevious ) );
}

// -- probe -----------------------------------------------------------------

{
	const e = new E();
	e.on( "z", () => {} );
	check( "on(name) probes handlers", e.on( "z" ) === true && !e.on( "absent" ) );
}

// -- isolation -------------------------------------------------------------

{
	const a = new E(), b = new E();
	let aHits = 0;
	a.on( "shared", () => aHits++ );
	b.on( "shared", [ 1 ] );
	check( "instances are isolated", aHits === 0, "aHits=" + aHits );
}

console.log( `\n${pass} passed, ${fail} failed` );
if( fail ) process.exitCode = 1;
