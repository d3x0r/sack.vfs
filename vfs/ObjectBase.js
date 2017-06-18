"use strict";

if (typeof Object.summon !== 'function') {
    Object.summon = function (o) {
        var object = {};
        console.log( Object.keys(o) );
        Object.setPrototypeOf(object, Object.getPrototypeOf( o ) );
        return object;
    };
}

if (typeof Object.develop !== 'function') {
    Object.develop = function (o) {
        function F() {};
        console.log( Object.keys(o) );
        F.prototype = o;
        return new F();
    };
}

var test = Object.summon( { a : 1} );
var test2 = Object.develop( { a:2 } );

console.log( Object.keys(test) ) ;
console.log( Object.keys(test2) ) ;

console.log( test.a );
console.log( test2.a );

console.log( "one" );
for (prop in test) {
        console.log( prop );
      }
console.log( "two" );

for (prop in test2) {
        console.log( prop );
      }

