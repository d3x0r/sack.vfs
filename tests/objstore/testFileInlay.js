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
	config : {defaults:false},
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
			// store.root is also set at this time.
			if( storage.configFile ) {
				console.log( "ping" );
				res( storage.config );
			}
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
	console.log( "Storage.config:", storage.config );
	return storage.configFile.write( storage.config );
}

function getConfig() {
	if( storage.configFile )
		return Promise.resolve( storage.config );

return new Promise( (res,rej)=>{
	waiters2.push( res );

	if( waiters2.length == 1 )	
		loadConfig().then( ()=>{
			Object.freeze( storage );
			waiters2.forEach( w=>w( storage.config ) );
			waiters2.length = 0;
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

