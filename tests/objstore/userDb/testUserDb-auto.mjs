const from = Number( process.argv[2] ) || 1
const count = Number( process.argv[3] ) || 100

import {sack} from "../../../vfs_module.mjs"
//const volume = sack.Volume( "storage", "data.vol" );
//const storage = sack.ObjectStorage( "storage@data.os" );
const storage = sack.ObjectStorage( "data.os" );


console.log( "Making from;", from );
import {UserDb} from "./userDb.mjs"
import {AccountDb} from "./accountDb/accounts.mjs"

init().then( run );

async function init() {
	await AccountDb.hook( storage );
	await UserDb.hook( storage );
	await reloadConfig()
}

async function run() {
	await makeUsers()
	await getUsers();	
}

const config = {
	lastUser : 0,
	commit : null,
}

async function reloadConfig() {
	const root = await storage.getRoot();
	try {
		const file = await root.open( "test.config.jsox" );
		const obj = await file.read()
		console.log( "reloadConfig:", obj );
		config.commit = ()=>file.write(config);
		Object.assign( config, obj );
	} catch(err){
		console.log( "Error is:", err );
		const file = await root.create( "test.config.jsox" )
		file.write( config );
		config.commit = ()=>file.write(config);
	} ;

}

function makeAccount( user ) {
	AccountDb.createAccount( user );
}

async function makeUsers() {
	const pad = config.lastUser;
	console.log( "Make users is using a previous pad:", pad );
	const target =  config.lastUser+count;
	for( let i = config.lastUser+1; i < target; i++ ) {
		//console.log( "Tick:", i );
		const unique = UserDb.getIdentifier();
		unique.key = sack.Id();
		if( i && i % ( count / 10 ) === 0 )
			console.log( "user:", i );

			await unique.store();

			config.lastUser++;
 			const user = unique.addUser( i, "User "+i, '' + i + "@email.com", Math.random()*(1<<54) );
			//console.log( "Created user" );
			await user.addDevice( sack.Id(), true )
			//console.log( "created device" );
			await user.store(); // have to wait for it to be stored to have an ID.
			AccountDb.makeAccount( user.id );
			//console.log( "something:", userId );
			//console.log( "And stored user." );
	}

	//config.lastUser = config.lastUser+count;
	console.log( "So this should save:", config.lastUser );
	config.commit();
}	


function getUsers() {
	UserDb.getUser( 3 ).then( (user)=>{
		
		console.log( "Got 3 :", user.id, user );
		AccountDb.getAccounts( user.id ).then( list=>{
			console.log( "user 3's accounts:", list );
		})
	} );
	UserDb.getUser( 203 ).then( (user)=>{
		console.log( "Got 203:", user );
		if( user )
		AccountDb.getAccounts( user.id ).then( list=>{
			console.log( "user 203's accounts:", list );
		})
	} );
	UserDb.getUser( 835 ).then( (user)=>{
		console.log( "Got 835:", user );
		if( user )
		AccountDb.getAccounts( user.id ).then( list=>{
			console.log( "user 835's accounts:", list );
		})
	} );
}

//go.then( ()=>{
//	console.log( "waited until initialized..." );
//} );
