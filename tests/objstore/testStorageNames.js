
const sack = require( "../.." );

//sack.Volume().unlink( "container.vfs" );
//const vol = sack.Volume( "container.vfs" );
//const store = sack.ObjectStorage( vol, "storage.os" );

sack.Volume().unlink( "storage.os" );
const store = sack.ObjectStorage( "storage.os" );

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
			setTimeout( ()=>{ ids.sort( (a,b)=>((a<b)?-1:1) ).forEach( id=>store.delete( id ) ); }, 1000 );
		}
		//console.log( "ID:", id );
	} );
