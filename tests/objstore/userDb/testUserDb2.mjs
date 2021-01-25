
import {sack} from "../../../vfs_module.mjs"
import {User,Device,UniqueIdentifier,go} from "./userDb.mjs"


function makeUsers() {
	for( let i = 0; i < 10000; i++ ) {
		const unique = new UniqueIdentifier();
		unique.key = sack.Id();
		console.log( "user:", i );
		unique.store().then( (userKeyId)=>{
			
		} );
 		const user = unique.create( i, "User "+i, '' + i + "@email.com", Math.random()*1<<54 );
		user.store().then( ()=>{
		} );
	}

}


function getUsers() {
	console.log( "Try to get:", 1523 );
	User.get( 1523 ).then( (user)=>{
		console.log( "Got 1523:", user );
	} );

	console.log( "Try to get:", 835 );
	User.get( 835 ).then( (user)=>{
		console.log( "Got 1523:", user );
	} );
}
console.log( "Go:", go );
go.then( ()=>{
	console.log( "waited until initialized..." );
	getUsers();
} );
