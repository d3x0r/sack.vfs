

const sack = require( "../../.." );
//sack.Volume().unlink( "container.vfs" );

const store = sack.ObjectStorage(  "storage.os" );

const d = new Date( "2022-02-07T06:03:30.877-08:00");

console.log( "Date shows as:", d.toString(), d );//+08:00" ) );

store.put( {Object:"Value"}, {id:"asdf"/*, time:d*/} );
store.put( {Object:"Value2"}, {id:"asdg"/*, time:d*/} );
store.put( {Object:"Value3"}, {id:"asdh"/*, time:d*/} );
store.put( {Object:"Value4"}, {id:"asdi"/*, time:d*/} );
store.put( {Object:"Value5"}, {id:"asdj"/*, time:d*/} );
store.put( {Object:"Value6"}, {id:"asdk"/*, time:d*/} );
store.put( {Object:"Value7"}, {id:"asdl"/*, time:d*/} );
store.put( {Object:"Value8"}, {id:"asdm"/*, time:d*/} );
store.put( {Object:"Value9"}, {id:"asdn"/*, time:d*/} );
store.put( {Object:"Value10"}, {id:"asdo"/*, time:d*/} );
store.put( {Object:"Value11"}, {id:"asdp"/*, time:d*/} );

store.get( "asdf" ).then( (obj)=>{
	const times = store.getTimes( "asdf" );
	store.setTime( "asdf", d );
	const times2 = store.getTimes( "asdf" );
	console.log( "Got back:", obj, times, times.map( t=>t.toString()) ) ;
	console.log( "Got back2:", obj, times2, times2.map( t=>t.toString()) ) ;
} );



//while( 1 );