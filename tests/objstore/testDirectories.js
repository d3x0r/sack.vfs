
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
const waiters2 = [];


function loadConfig() {
	if( waiters.length > 0 ) {
		return new Promise( (res,rej)=>waiters.push( {res:res,rej:rej} ) );
	}

return new Promise( (res,rej)=>{
	waiters.push( {res:res,rej:rej} );
	store.getRoot().then( directory=>{
		if( storage.rootDirectory ) {
			console.log( "short circuit with already loaded" );
			res( storage.config );
		}
		storage.rootDirectory = directory;

		directory.folder( "Subdirectory/fonts/modern" ).then( (newdir)=>{
			console.log( "Subdirectory created?", newdir );
		} );
		
		function setConfig(data){
			storage.config = data;

	 		if( !storage.config ){
				console.log( "No configuration found, still, write new defaults" );
				storage.config = {
					defaults:true
				};
				saveConfig();
			}
			console.log( "And finally config:", storage.config );
			waiters.forEach(w=>w.res(storage.config));
			waiters.length = 0;
		}
		directory.open( "config.jsox" ).then( file=>{
			if( !storage.configFile ) {
				storage.configFile = file;
				storage.configFile.read().then( setConfig ).catch( ()=>{
					storage.config = { default:false };
					saveConfig().then( (a)=>{
						console.log( "SaveCOnfig got back:", a );
						storage.configFile.read().then( setConfig ).catch((err)=>console.log( "Fatal Error:", err));
					} );
				} );
			} else
				console.log( "throwing away a file:", file );
		} );
	} );
});

}

async function saveConfig() {
	return storage.configFile.write( storage.config );
}

function getConfig() {
	if( storage.config )
		return Promise.resolve( storage.config );
	return new Promise( (res,rej)=>{
		loadConfig().then( ()=>{
			Object.freeze( storage );
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

