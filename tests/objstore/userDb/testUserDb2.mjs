
import {User,go} from "./userDb.mjs"

function getUsers() {
	console.log( "Try to get:", 1523 );
	User.get( 1523 ).then( (user)=>{
		console.log( "Got 1523:", user );
	} );

	console.log( "Try to get:", 3 );
	User.get( 3 ).then( (user)=>{
		console.log( "Got 3:", user );
	} );

	console.log( "Try to get:", 835 );
	User.get( 835 ).then( (user)=>{
		console.log( "Got 835:", user );
	} );
}

//console.log( "Go:", go );
// go is a promise that resolves when userdb is initialized
go.then( ()=>{
	console.log( "waited until initialized..." );
	getUsers();
} );
