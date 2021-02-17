const id = Number( process.argv[2] ) || 123;

import {sack} from "../../../vfs_module.mjs"
//const volume = sack.Volume( "storage", "data.vol" );
//const storage = sack.ObjectStorage( "storage@data.os" );
const storage = sack.ObjectStorage( "data.os" );


import {UserDb} from "./userDb.mjs"
const wait = UserDb.hook( storage );

function getUsers() {
	console.log( "Try to get:", id );
	UserDb.getUser( id ).then( (user)=>{
		console.log( "Got:", user );
	} );
}

// go is a promise that resolves when userdb is initialized
wait.then( ()=>getUsers( ) );
