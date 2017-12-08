
var sack=require( ".." );
try {
	var vol = sack.Volume( null, "test.vfs" );
}catch(err) {
	console.log( "Already rekeyed..." );
}
console.log( "vol is:", vol );
var vol2;

try {
	vol2 = sack.Volume( null, "test.vfs", "key1", "key2" );
	console.log( "vol2 is:", vol2 );
	vol2.rekey();
        
} catch( err ) {
	console.log( "(dorekey) Failed:", err );
	vol.rekey( "key1", "key2" );
	console.log( "rekeyed" );
}
