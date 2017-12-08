

var sack = require('..');
var JSON6 = sack.JSON
var d = '[ {"a": "", "b": ""}, {"a": "", "b": ""} ]';

console.log( JSON6.parse( d ) );

var d = '[ {"a": "", "b": ""}, [1,2], {"a": "", "b": ""} ]';
console.log( JSON6.parse( d ) );

var d = '{ "a":{"a": "", "b": ""}, "b":[{"d":"","e":""},{"f":"","g":""}], "c":{"a": "", "b": ""} }';
console.log( JSON6.parse( d ) );
