
var sack = require( ".." );
var str = sack.Volume.readAsString( "test_huge_string.js" );
console.log( "Magic string?", str.length );

str = sack.Volume.readAsString( "I:\\vm\\mac\\macOS 10.12 Sierra Final by TechReviews.rar" );
console.log( "Magic string2?", str.length );
