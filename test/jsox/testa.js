
const SACK=require("../.." );
const JSOX = SACK.JSOX;
const parse = JSOX.parse;

    var result = parse( "{ 'my  \\\r  key':3}" );

    console.log( "result:", result );

console.log( parse( "'\\\u07ec'" ).length );

