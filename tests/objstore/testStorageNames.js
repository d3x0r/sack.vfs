
const sack = require( "../.." );
//sack.Volume().unlink( "storage.os" );

const store = sack.ObjectStorage( "storage.os" );

var object = { 
	data: "This object contains data.",
        get date() { return new Date()  }
};

store.put( object ).then( (id)=>{
	store.delete( id );
} );
