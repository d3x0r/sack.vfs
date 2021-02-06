

import {sack} from "../../../vfs_module.mjs"
import {BloomNHash} from "./bloomNHash.mjs"

const configObject = {
		accountId : null,
		emailId : null,
		reconnectId : null,
		lastUser : 0,
		commit() {
			if( l.configCommit ) l.configCommit();
		},
	}

const l = {
	configCommit : null,
	ids : configObject,
	account   : null,
	email     : null,
	reconnect : null
};

let inited = false;
let initResolve = null;
let initializing = new Promise( (res,rej)=>{
	initResolve = res;
} ).then( ()=>{
	inited = true;
} );

//const volume = sack.Volume( "storage", "data.vol" );
//const storage = sack.ObjectStorage( "storage@data.os" );
const storage = sack.ObjectStorage( "data.os" );

BloomNHash.hook( storage );


storage.getRoot().then( (root)=>{
	root.open( "config.jsox" ).then( (file)=>{
		return file.read().then( async (obj)=>{
			Object.assign( l.ids, obj );

			l.email     = await storage.get( l.ids.emailId );
			l.account   = await storage.get( l.ids.accountId );
			l.reconnect = await storage.get( l.ids.reconnectId );

			l.configCommit = ()=>{
				file.write( l.ids );
			};
			initResolve();
		} );
	} ).catch( (err)=>{
		root.create( "config.jsox" ).then( async (file)=>{
			l.account   = new BloomNHash();
			l.email     = new BloomNHash();
			l.reconnect = new BloomNHash();

			l.ids.accountId   = await l.account.store();
			l.ids.emailId     = await l.email.store();
			l.ids.reconnectId = await l.reconnect.store();

			file.write( l.ids );

			l.configCommit = ()=>{
				file.write( l.ids );
			};
			initResolve();
		} );
	} );
} );

class StoredObject {
	#id = null;
	get id() { return this.#id } ;
	store() {
		if( !this.#id ) {
			// might have been reloaded...
			const container = storage.getContainer( this );
			if( container ) this.#id = container.id;
			return storage.put( this, {id:this.#id} ).then( (id)=>{
				if( !this.#id ) this.#id = id;
				else if( this.#id !== id ) { console.log( "Object has been duplicated: old/new id:", this.#id, id ); }
				return this;
			} );
		} else {
			return storage.put( this, {id:this.#id} ).then( (id)=>{
				if( this.#id !== id ) { console.log( "Object has been duplicated: old/new id:", this.#id, id ); }
				return this;
			} );
		}
	}		

}


class UniqueIdentifier extends StoredObject {
	key = null;
	constructor() {
		super();
	}
	create( account,user,email,pass ){
		const newUser = new User();
		newUser.account = account;
		newUser.user = user;
		newUser.email = email;
		newUser.pass = pass;
		newUser.unique = this;
		return newUser;
	}
}

class User  extends StoredObject{
	unique = null;
	account = null;
	name = null;
	email = null;
	password = null;
	devices = [];
	
	constructor() {
		super();
	}
	store() {
		return super.store().then( async (id)=>{
			await l.account.set( this.account, this );
			await l.email.set( this.email, this );
			return this;
		} );
	}
	addDevice( id, active ) {
		const device = new Device();
		device.key = id;
		device.active = active;
		return device.store().then( ()=>
			this.devices.push(device ) );
	}
}

User.get = function( account ) {
	if( !inited ) {
		return initializing.then( ()=>{
			return l.account.get( account );
		} );
	}
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

class Device  extends StoredObject{
	key = null;
	active = false;
	constructor() {
		super();
	}
}


storage.addEncoders( [ { tag:"~U", p:User, f: null },  { tag:"~D", p:Device, f: null },  { tag:"~I", p:UniqueIdentifier, f: null } ] );
storage.addDecoders( [ { tag:"~U", p:User, f: null },  { tag:"~D", p:Device, f: null },  { tag:"~I", p:UniqueIdentifier, f: null } ] );

export {configObject as config};
export {User,Device,UniqueIdentifier,initializing as go} ;
