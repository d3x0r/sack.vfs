
var sack=require('..');
var JSON=sack.JSON6;

console.log( JSON.parse( "'\\\\ \\\" \\\' '" ) );