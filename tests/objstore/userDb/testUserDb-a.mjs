const from = Number( process.argv[2] ) || 1
const count = Number( process.argv[3] ) || 100

console.log( "Making from;", from );
import {sack} from "../../../vfs_module.mjs"
import {User,Device,UniqueIdentifier,go} from "./userDb.mjs"


async function makeUsers() {
	for( let i = 1; i < count; i++ ) {
		const unique = new UniqueIdentifier();
		unique.key = sack.Id();
		//console.log( "user:", i );
		await unique.store().then( ((i)=> ()=>{
	 			const user = unique.addUser( i, "User "+i, '' + i + "@email.com", Math.random()*(1<<54) );
				return user.addDevice( sack.Id(), true ).then( ()=>{
					//console.log( "storing user", i );
					return user.store();
				} );
			} )(i+from) );
	}
}	


function getUsers() {
	console.log( "Getting..." );
	User.get( 3 ).then( (user)=>{
		console.log( "Got 3 :", user );
	} );
	User.get( 203 ).then( (user)=>{
		console.log( "Got 203:", user );
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
