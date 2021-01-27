const from = Number( process.argv[2] ) || 1
const count = Number( process.argv[3] ) || 100

console.log( "Making from;", from );
import {sack} from "../../../vfs_module.mjs"
import {User,Device,UniqueIdentifier,go} from "./userDb.mjs"


function makeUsers() {
	const wait = [];
	for( let i = 1; i < count; i++ ) {
		const unique = new UniqueIdentifier();
		unique.key = sack.Id();
		//console.log( "user:", i );
		wait.push( unique.store().then( ((i)=> ()=>{
	 			const user = unique.create( i, "User "+i, '' + i + "@email.com", Math.random()*(1<<54) );
				return user.addDevice( sack.Id(), true ).then( ()=>{
					//console.log( "storing user", i );
					return user.store();
				} );
			} )(i+from) ) );
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
		console.log( "So sdid all that?" );
		getUsers();
	} );
} );
