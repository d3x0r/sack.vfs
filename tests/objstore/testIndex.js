
var sack = require( "../.." );


const orgRoot = "org.example.domain"
const serviceRoot = "data";
const appIdentifier = sack.SaltyRNG.id(3);


//sack.Volume().unlink( "container.vfs" );

var vfs = sack.Volume( "cmount", "container.vfs" );

var store = sack.ObjectStorage( "cmount@storage.os" );

var root = null;
var dbRoot = null;
var config = null;


function createUser( name ){
	var o = {
		name : name,
		created : new Date(),
		user_id : sack.SaltyRNG.id(),
		siblings:[],
		parents:[],
		children:[],
		spice : [],

		
	}
	return o;
}


//https://gist.github.com/d3x0r/5c4220207c6469c5b946b5a9b08f4ef6

async function initConfig() {
	 await vfs.readJSOX( "config.jsox", async (readConfig)=>{
		config = readConfig;
		store.get( config.root ).then( async (data)=>{
			dbRoot = data;
			if( !dbRoot.tick ) {
				dbRoot.tick = new Date();
				store.put( dbRoot );
				return;
			}
			if( !dbRoot.users ) {
				//dbRoot.users = [];  // can optionally assign an array first
				// if the member does not exist, or has a value of undefined
				// this will automatically assign the array.
				store.index( dbRoot, "users.name" );
				store.index( dbRoot, "users.created" );
				var a,b,c,d,e,f,g,h;
				dbRoot.users.push( a=createUser( "Tom" ) );
				dbRoot.users.push( b=createUser( "Alice" ) );
				dbRoot.users.push( c=createUser( "Bob" ) );
				dbRoot.users.push( d=createUser( "Jerrie" ) );
				dbRoot.users.push( e=createUser( "Harry" ) );
				dbRoot.users.push( f=createUser( "Barb" ) );
				dbRoot.users.push( g=createUser( "Nick" ) );
				dbRoot.users.push( h=createUser( "Monica" ) );
				a.siblings = a.siblings.concat([b,c]);
				b.siblings = b.siblings.concat([a,c]);
				c.siblings = c.siblings.concat([a,b]);
				d.children = d.children.concat([a,b,c]);
				e.children = e.children.concat([a,b,c]);
				a.parents = a.parents.concat([d,e])
				d.spice.push( e );
				d.spice.push( d );
				f.siblings.push( g );
				f.parents.push( h );
				h.children.push(f);
				store.put( dbRoot );
				return;
			}
			if( !dbRoot.groups5 ) {
				dbRoot.groups5 = [];
				dbRoot.groups5.push( {
					name: "Group 1",
					captain : dbRoot.users.find( user=>user.name==="Tom" ),
					members : dbRoot.users.find( user=>user.name==="Nick" ),
					members : dbRoot.users.find( user=>user.name==="Barb" ),
				})
				await dbRoot.users.forEach( async (u)=>await store.put(u) );
				store.put( dbRoot );
				return;
			}
		})
	} );

	if( !config ){
		config = {
			appIdentifier : appIdentifier,
			serviceRoot : serviceRoot,
			orgRoot : orgRoot,
		};
		config.root = await store.put( dbRoot = {} );
		
		vfs.write( "config.jsox", sack.JSOX.stringify( config ) );
	} 
}

initConfig();

var userIndex = {
	field : "firstName"
};


