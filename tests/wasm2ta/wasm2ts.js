
var sack=require( "../..");

console.log( "new TypedArray([" + new Uint8Array( sack.Volume.mapFile( process.argv[2] ) ).join(",") + "])" );
