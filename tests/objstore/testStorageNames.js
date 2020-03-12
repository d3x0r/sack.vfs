
const sack = require( "../.." );
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
		if( !ids.length )
			setTimeout( ()=>{ ids.sort( (a,b)=>a<b ); ids.forEach( id=>store.delete( id ) ); }, 1000 );
		ids.push(id);
		console.log( "ID:", id );
	} );
