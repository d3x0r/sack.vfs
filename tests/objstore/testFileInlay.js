console.log( "Untested... shell to test" );

const orgRoot = "org.example.domain"
const serviceRoot = "data";
const dbRoot = "users";

const appIdentifier = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";



const sack = require( "../.." );
//sack.Volume().unlink( "container.vfs" );

const vfs = sack.Volume( "cmount", "container.vfs" );
//console.log( "sack:", sack );

const store = sack.ObjectStorage( vfs, "storage.os" );
// consts...
const storage = {
	rootDirectory : null,
	config : null,
	configFile : null,
};
const waiters = [];


function loadConfig() {
return new Promise( (res,rej)=>{
	store.getRoot().then( directory=>{
		// store.root is also set at this time.
		function setConfig(data){
			storage.config = sack.JSOX.parse( data );

	 		if( !storage.config ){
				console.log( "No configuration found, still, write new defaults" );
				storage.config = {
					defaults:true
				};
				saveConfig();
			}
			console.log( "And finally config:", storage.config );
			res();
		}
		directory.open( "config.jsox" ).then( f=>{
			storage.configFile = f;
			storage.configFile.read().then( setConfig ).catch( ()=>{
				saveConfig().then( ()=>{
					storage.configFile.read().then( setConfig );
				} );
			} );
		} );
	} );
});

}

async function saveConfig() {
	return storage.configFile.write( sack.JSOX.stringify( storage.config ) );
}

function getConfig() {
	if( storage.config )
		return Promise.resolve( storage.config );
return new Promise( (res,rej)=>{
	waiters.push( res );
	loadConfig().then( ()=>{
		Object.freeze( storage );
		waiters.forEach( w=>w( storage.config ) );
		waiters.length = 0;
	} )
} );
}

getConfig().then( (config)=>{
		if( !config.counter )
			config.counter = 1;
		else
			config.counter++;
		saveConfig();
} );


getConfig().then( (config)=>{
		if( config.lastUpdate ) {
			console.log( "Last Update was:", config.lastUpdate.toString() );
		}
		config.lastUpdate = new Date();
		saveConfig();
} );

