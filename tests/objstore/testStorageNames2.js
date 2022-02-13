
const sack = require( "../.." );

function f1() {
	sack.Volume().unlink( "container.vfs" );
	const vol = sack.Volume( "container.vfs" );
	return sack.ObjectStorage( vol, "storage.os" );
}
function f2() {
//	sack.Volume().unlink( "storage.os" );
	return sack.ObjectStorage( "storage.os" );
}

const store = f2(); 


var object = { 
	data: "This object contains data.",
        get date() { return new Date()  }
};

var ids = [];
var nMax = Number( process.argv[2] );
console.log( "input?", process.argv );
if( nMax === NaN || !nMax ) nMax = 5;
for( var n = 0; n < nMax; n++ )
	store.put( Object.assign({},object) ).then( (id)=>{
		ids.push(id);
		if( ids.length  === nMax ) {
			console.log( "Done." );
		}
		//console.log( "ID:", id );
	} );
