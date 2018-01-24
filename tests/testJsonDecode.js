
var sack=require( ".." );
console.log( "MNOP=", sack.JSON6.parse( '"\\u004D\\u004e\\u004F\\u0050"' ) );
console.log( "MNOP=", sack.JSON.parse( '"\\u004D\\u004e\\u004F\\u0050"' ) );
