

const sack = require( "../../.." );
//sack.Volume().unlink( "container.vfs" );

const store = sack.ObjectStorage(  "storage.os" );

const d = new Date( "2022-02-07T06:03:30.877-08:00");

//console.log( "Date shows as:", d.toString(), d );//+08:00" ) );
store.get( "ReloadId" ).then( (obj)=>{
	console.log( "What did reload get?", obj );

	store.put( {Object:"Value"}, {id:obj} ).then( (id)=>{
		store.put( sack.JSOX.stringify(id), {id:"ReloadId"} ) 
	        
		store.get( id ).then( (obj)=>{
			const times = store.getTimes( id );
			store.setTime( id, d );
			const times2 = store.getTimes( id );
			console.log( "Got back:", obj, times, times.map( t=>t.toString()) ) ;
			console.log( "Got back2:", obj, times2, times2.map( t=>t.toString()) ) ;
		} );

	} );

} );


//while( 1 );