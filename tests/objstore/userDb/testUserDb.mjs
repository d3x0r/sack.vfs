
import {sack} from "../../../vfs_module.mjs"
import {UserDb,User,Device,UniqueIdentifier,go} from "./userDb.mjs"

const storage = sack.ObjectStorage( "data.os" );
UserDb.hook( storage );

function makeUsers() {
	const wait = [];
	for( let i = 1; i < 10000; i++ ) {
		const unique = new UniqueIdentifier();
		unique.key = sack.Id();
		//console.log( "user:", i );
		wait.push( unique.store().then( ((i)=> ()=>{
	 			const user = unique.addUser( i, "User "+i, '' + i + "@email.com", Math.random()*(1<<54) );
				//console.log( "storing user", i );
				return user.store();
			} )(i) ) );
	}
	return Promise.all( wait );
}	


function getUsers() {
	console.log( "Getting..." );
	User.get( 1523 ).then( (user)=>{
		console.log( "Got 1523:", user );
	} );
	User.get( 835 ).then( (user)=>{
		console.log( "Got 835:", user );
	} );
}
console.log( "Go:", go );
go.then( ()=>{
	console.log( "waited until initialized..." );
	makeUsers().then( ()=>{			
		// after creating all users, get some users
		getUsers();
	} );
} );
