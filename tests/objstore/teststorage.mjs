
import {sack} from "sack.vfs";
import {ObjectStorage} from "sack.vfs/object-storage"
//const sack = require( "../.." );
const db = sack.Sqlite( "psql.db" );

const config = { 
	local:"settings",
	now:new Date()
};

if( db ) {
	const store = new ObjectStorage( db );
        //console.log( "Store:", store, Object.getPrototypeOf( store ) );
	f( store );
}

async function f( store ) {

	const root = await store.getRoot();
	//console.log( "root:", root );
	const saveConfig = (file)=>file.write( config );
	const readConfig = (file)=>file.read( ).then( (cfg)=>(Object.assign( config, cfg ),file.write( config ),config) );
	await root.create( "config.jsox" )
	          .then( saveConfig )
	          .then( ()=>console.log( "Created file" ) )
	          .catch( ()=>root.open( "config.jsox" )
	                          .then( readConfig )
	                          .then( ()=>console.log( "loaded file" ) ) );
	console.log( "Now is :", new Date() );
	console.log( "Config's date is:", config );
}
