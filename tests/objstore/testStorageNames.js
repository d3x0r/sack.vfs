
const sack = require( "../.." );
//sack.Volume().unlink( "storage.os" );

const store = sack.ObjectStorage( "storage.os" );

var object = { 
	data: "This object contains data.",
        get date() { return new Date()  }
};

for( var n = 0; n < 5; n++ )
	store.put( Object.assign({},object) ).then( (id)=>{
		console.log( "ID:", id );
		setTimeout( ()=>{ store.delete( id ); }, 1000 );
	} );
