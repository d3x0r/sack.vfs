
import {sack} from "sack.vfs"

import {BloomNHash} from "../../../node_modules/@d3x0r/bloomnhash/bloomNHash.mjs"
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
	orgId : null,
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
        orgs : null,
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
	addUser( user,account,email,pass ){
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

export class Sash extends StoredObject{
	org = null;
	name = null;  // name of the sash
	badges = []; // this sash has these badges.
	constructor( ) {
		super( l.storage );
	}
	set( org, name ) {
		this.org = org;
		this.name = name;
		return this.store();
	}
}

export class SashAlias extends StoredObject{
	name = null;  // name of the sash
	sash = null;
	constructor( name, sash ) {
		super( l.storage );
		this.name = name;
		this.sash = sash;
	}	
}

export class Badge  extends StoredObject{
	badgeId = null;
	name = null;  // token name
	description = null; // be nice to include a description?
	constructor() {
		super( l.storage );
	}	
	
}


export class Organization  extends StoredObject{
	orgId = null;
	name = null;
	createdBy = null;
	members = new SlabArray( l.storage );
	domains = [];
	constructor() {
		super( l.storage );
	}

	async store() {
		await super.store();
		await l.orgs.set( this.name, this ); 
		//for( n = 0; 
	}

	// get a badge for this org.
	// users have sashes with badges 
	//  after getting a badge, then user's active sash should be used.
	// 
	async getBadge( name, forUser ) {
		const badge = this.badges.find( badge=>badge.name===name );	
		if( !badge ) {
			
		}
	}

	async getDomain( name, forUser ) {
		const domain = this.domains.find( domain=>domain.name===name );	
		if( !domain ) {
			const newDomain = new Domain( this, name, forUser );
			this.domains.push( newDomain );

			this.store();
			UserDb.on( "newDomain", newDomain );
			return newDomain;
		} else {
			return domain;
		}
	}

}

Organization.get = async function ( name, forClient ) {
	const org = await l.orgs.get( name );
	if( !org ) {
		const org = new Organization( name );
		//forClient.

		UserDb.on( "newOrg", this );
	} else {
		for( let n = 0; n < org.length; n++ ) {
			
		}
	}
}



export class Domain  extends StoredObject{
	domainId = null;
	org = null;
	name = null;
	createdBy = null;
	members = new SlabArray( l.storage );
	badges = []; // badges that this org has created.
	servicess = []; // services this domain has available.
	constructor( org, name, forUser ) {
		super( l.storage );
		this.domainId = sack.Id();
		this.org = org;
		this.name = name;
		this.createdBy = forUser;
	}

	async store() {
		await super.store();
		await l.orgs.set( this.name, this ); 
		//for( n = 0; 
	}

	// get a badge for this org.
	// users have sashes with badges 
	//  after getting a badge, then user's active sash should be used.
	// 
	async getBadge( name, forUser ) {
		const badge = this.badges.find( badge=>badge.name===name );	
		if( !badge ) {
			
		}
	}


	async getService( name, forUser ) {
		const srvc = this.srevices.find( srvc=>srvc.name===name );	
		if( !srvc ) {
			const newSrvc = new Service( this, name, forUser );
			UserDb.on( "newService", newSrvc );
		}
				
	}
}

Domain.get = async function( name, forClient ) {
}


export class Service  extends StoredObject{
	orgId = null;
	name = null;
	createdBy = null;
	members = new SlabArray( l.storage );
	badges = []; // badges that this org has created.
	instances = []; // badges that this org has created.
	constructor() {
		super( l.storage );
	}

	async store() {
		await super.store();
		await l.orgs.set( this.name, this ); 
		//for( n = 0; 
	}

	// get a badge for this org.
	// users have sashes with badges 
	//  after getting a badge, then user's active sash should be used.
	// 
	async getBadge( name, forUser ) {
		const badge = this.badges.find( badge=>badge.name===name );	
		if( !badge ) {
			
		}
	}
}




export class User  extends StoredObject{
	userId = null; 
	unique = null;
	account = null;
	name = null;
	email = null;
	pass = null;
	devices = [];
	sashes = []; 
	created = new Date();
	
	constructor() {
		super(l.storage);
		this.userId = sack.Id();
	}
	store() {
		return super.store().then( async (id)=>{	
			//console.log( "what about?", id, l );
			console.log( "Setting account to:", this.account, this );
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
			(this.devices.push(device ),this.store(),device) );
	}
	async getDevice( id ) {
		return new Promise( (res,rej)=>{
			let results = 0;
			for( let device of this.devices ) {
				if( device instanceof Promise ) {
					//console.log( "device needs to be loaded..." );
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
				else  {
					if( device.key === id ) {
						results = -1; // make sure nothing else checks.
						res( device );
                                                return;
                                        }
                                }
			}
                        if( results === 0 ) res( null );
		});
	}
	async getSash( org ) {
		const found = [];
		this.sashes.forEach( sash=>{
			if( sash.org===org )
				found.push(sash);
		} );
		if( found.length ) {
			// ask user to select a sash to wear.
		}
	}
}

User.get = function( account ) {
	console.log( "l?", l.account, account );
	return l.account.get( account );
}

User.getEmail = function( email ) {
	if( email && email === "" ) return null; // all NULL email addresses are allowed.
	//console.log( "l?", l.email, email );
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

const eventMap = {};

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
	on( event, data ) {
		if( "function" === typeof data ) {
			const a = eventMap[event];
			if( !a ) a = eventMap[event] = [];
			a.push( data );
		} else {
			const a = eventMap[event];
			if( a ) for( let f of a ) f( data );
		}
	},
	off( event, f ) {
		console.log( "disabling events not enabled" );
	},
	getUser(args){
		return getUser(args);
	},
	User:User,
	async getIdentifier( i ) {
		if( i ) return await l.clients.get( i );
		return getIdentifier();
	},
        async addIdentifier( i ) {
		
            	return l.clients.set( i.key, i );
        },
        async getOrg( i ) {
		
            	return l.clients.set( i.key, i );
        },
	Device:Device,
	UniqueIdentifier:UniqueIdentifier,
	addService( org ) {
		const oldOrg = l.orgs.get
		const newOrg = new Organization();
	},

	getBadge( org ) {
		
	},

}

Object.freeze( UserDb );
export {configObject as config};
export { UserDb, initializing as go } ;
