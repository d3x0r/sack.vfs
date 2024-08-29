import {sack} from "sack.vfs"


const client = sack.Network.TCP( { toAddress:"[::1]:1234", connect( error ) {
	console.log( "connected:", error );
	client.send( "test string send" );
	setTimeout( ()=>{
	client.send( "test binary send" );}, 250 );

	const b1 = new Uint8Array( [..."test binary send"].map( c=>c.codePointAt(0) ) )
	const b2 = new Uint8Array( [..."test string send"].map( c=>c.codePointAt(0) ) )

	setTimeout( ()=>{ client.send( b1 );}, 500 );
	setTimeout( ()=>{ client.send( b2 );}, 750 );
	
	setTimeout( ()=>{ client.close();}, 1000 );
}, 
message( buf ) {
	console.log( "Got buffer:" , buf );
},
close() {
	console.log( "Socket closed..." );
}


} );


