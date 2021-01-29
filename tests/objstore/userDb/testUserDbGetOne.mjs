const id = Number( process.argv[2] ) || 123;
import {User,go} from "./userDb.mjs"

function getUsers() {
	console.log( "Try to get:", id );
	User.get( id ).then( (user)=>{
		console.log( "Got:", user );
	} );
}

// go is a promise that resolves when userdb is initialized
go.then( getUsers );
