
var sack = require( ".." );
var db = sack.Sqlite( "testFunction.db" );

db.function( "generate", ()=>{
		console.log("Generate called" );
		return "adsf";
	} );

console.log( db.do( "select generate()" ) );

db.close();  // user function makes thread that needs to close...
