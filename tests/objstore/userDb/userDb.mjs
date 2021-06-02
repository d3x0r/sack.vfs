
import {sack} from "sack.vfs"

import {BloomNHash} from "./bloomNHash.mjs"
import {SlabArray}  from "./accountDb/SlabArray.mjs"

const StoredObject = sack.ObjectStorage.StoredObject;
//import {StoredObject} from "../commonDb.mjs"


let inited = false;
let initResolve = null;
let initializing = new Promise( (res,rej)=>{
	initResolve = res;
} ).then( ()=>{
	inited = true;
} );

const configObject = {
	accountId : null,
	emailId : null,
	reconnectId : null,
	clientId : null,
	actAs : null,
	actIn : null,
	actBy : null,
};

const l = {
	ids : configObject,
	account   : null,
	email     : null,
	reconnect : null,
        clients : null,
	actAs : null, // relates user ids that user can act As (inhertis rights of act-as )
	actIn : null, // relates user ids that user belong to (inherit all rights of in)
	actBy : null, // relates user ids that a user can be enacted by
        storage : null,
};



export class UniqueIdentifier extends StoredObject {
	key = null;
	created = new Date();
	constructor() {
		super(l.storage);
	}
	addUser( account,user,email,pass ){
		const newUser = new User();
		newUser.hook( this );
		newUser.account = account;
		newUser.name = user;
		newUser.email = email;
		newUser.pass = pass;
		newUser.unique = this;
		return newUser;
	}
}

export class User  extends StoredObject{
	unique = null;
	account = null;
	name = null;
	email = null;
	pass = null;
	devices = [];
	created = new Date();
	
	constructor() {
		super(l.storage);
	}
	store() {
		return super.store().then( async (id)=>{	
			//console.log( "what about?", id, l );
			await l.account.set( this.account, this );
			//console.log( "Account was set" );
			await l.email.set( this.email, this );
			//console.log( "email was indexed" );
			return this;
		} );
	}
	addDevice( id, active ) {
		const device = new Device();
		device.hook( this )
		device.key = id;
		device.active = active;
		return device.store().then( ()=>
			(this.devices.push(device ),device) );
	}
	async getDevice( id ) {
		return new Promise( (res,rej)=>{
			console.log( "Returned a promise..." );	
			let results = 0;
			for( let device of this.devices ) {
				if( device instanceof Promise ) {
				console.log( "device needs to be loaded..." );
					results++;
					device.then( (dev)=>{
						if( results >= 0 ) {
							if( dev.key === id ){
								device.accessed = new Date();
								device.store();
								res( device );
								if( results > 1 )
									results = -1; 
								return;
							}
							results--;
							if( results === 0 ) {
								//console.log( "nothing more to load..." );
								res( null );
							}
						}
					} );
					this.storage.map( device );
				}
				else 
					if( device.key === id ) return device;
			}
			return null;
		});
		//return null;
	}
}

User.get = function( account ) {
	//console.log( "l?", l );
	return l.account.get( account );
}

User.getEmail = function( email ) {
	if( !inited ) {
		return initializing.then( ()=>{
			return l.email.get( email );
		} );
	}
	return l.email.get( email );
}




export class Device  extends StoredObject{
	key = null;
	active = false;
	added = new Date();
	accessed = new Date();
	constructor() {
		super(l.storage);
	}
}

let getUser = null;
let getIdentifier = null;

async function userActsAs( user, act ) {
	const active = l.actAs.get( user );
	if( active ) {
		active.push( act );
	}else {
		const array = new SlabArray(l.actAs.storage);
		array.push( active );
		l.actAs.set( user, array )
	}
	
	const users = l.actBy.get( act );
	if(users )
		users.push( user );
	else {
		const array = new SlabArray(l.actAs.storage);
		array.push( user );
		l.actBy.set( act, array )
	}

}

async function userActsIn( user, group ) {
	const active = l.actIn.get( group );
	if( active ) {
		active.push( group );
	}else {
		const array = new SlabArray(l.actAs.storage);
		array.push( user );
		l.actIn.set( user, [ group ] )
	}

	// members?

}

const UserDb = {
	async hook( storage ) {
            	l.storage = storage;
		BloomNHash.hook( storage );
		storage.addEncoders( [ { tag:"~U", p:User, f: null },  { tag:"~D", p:Device, f: null },  { tag:"~I", p:UniqueIdentifier, f: null } ] );
		storage.addDecoders( [ { tag:"~U", p:User, f: null },  { tag:"~D", p:Device, f: null },  { tag:"~I", p:UniqueIdentifier, f: null } ] );

		getUser = (id)=>{
			return User.get( id );
		};
		getIdentifier = ()=>{
			const unique = new UniqueIdentifier();
                        unique.key = sack.Id();
			unique.hook( storage );
			return unique;
		}
		const root = await storage.getRoot();
		try {
			const file = await root.open( "userdb.config.jsox" )
		
				const obj = await file.read()
				Object.assign( l.ids, obj );
				l.clients   = await storage.get( l.ids.clientId );
				l.email     = await storage.get( l.ids.emailId );
				l.account   = await storage.get( l.ids.accountId );
				l.reconnect = await storage.get( l.ids.reconnectId );
			} catch( err){
				console.log( "User Db Config ERR:", err );
				const file = await root.create( "userdb.config.jsox" );
				
				l.clients   = new BloomNHash();
				l.clients.hook( storage );
				l.account   = new BloomNHash();
				l.account.hook( storage );
				l.email     = new BloomNHash();
				l.email.hook( storage );
				l.reconnect = new BloomNHash();
				l.reconnect.hook( storage );

				l.ids.clientId    = await l.clients.store();
				l.ids.accountId   = await l.account.store();
				l.ids.emailId     = await l.email.store();
				l.ids.reconnectId = await l.reconnect.store();

				file.write( l.ids );
			}
                	if( initResolve )
	                	initResolve();
	},
	getUser(args){
		return getUser(args);
	},
	User:User,
	async getIdentifier( i ) {
		if( i ) {
			const id = await l.clients.get( i );
			console.log( "get result:", id, i );
			return id;
		}
		return getIdentifier();
	},
        async addIdentifier( i ) {
            	const id = await i.store()
		console.log( "Saving key:" );
        	return l.clients.set( i.key, i );
        },
	Device:Device,
	UniqueIdentifier:UniqueIdentifier,
}

Object.freeze( UserDb );
export {configObject as config};
export { UserDb, initializing as go } ;
