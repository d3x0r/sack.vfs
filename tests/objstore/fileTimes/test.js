

const sack = require( "../../.." );
const {ObjectStorage} = require( "sack.vfs/object-storage" );
//sack.Volume().unlink( "container.vfs" );
console.log( "module?", ObjectStorage );
const store = new ObjectStorage(  "storage.os" );
store.dir().then( (dir)=>{;
	console.log( "Updated maybe? ", dir );
} )
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
	console.log( "object?", obj );

} );



//while( 1 );