const sack = require( "../../.." );
var JSOX = sack.JSOX;
var os = sack.ObjectStorage( "test.os" );

var m = new Map();
var o = {keys:m};
const c1 = {obj1:0};
m.set( "asdf", c1 );

var m2 = new Map();
m.set( "mapsub", m2 );

os.put(m2);
os.put(c1);
os.put(m );

console.log( "map only", JSOX.stringify( m ) );
console.log( "object", JSOX.stringify( o ) );
