import {sack} from "sack.vfs";
const StoredObject = sack.ObjectStorage.StoredObject;//import {StoredObject}

//import {StoredObject} from "../commonDb.mjs"
import { SlabArray } from "@d3x0r/slab-array";
import {BloomNHash} from "@d3x0r/bloomnhash"


const configObject = {
		accountsId : null,
		userMapId : null,
		commit() {
				if( l.configCommit ) l.configCommit();
		},
}

const l = {
		configCommit : null,
		ids : configObject,
		accounts   : null,
		userMap : null,
};


class Transaction extends StoredObject {
	ledger  = null;
	balance = 0.0;
	history = null;
	hap     = new Date();
	to      = null;
	del     = 0.0;
	constructor(storage) {
		//console.trace( "Init transaction with storage:", storage );
		super(storage);
	}
	
	store() {
		//console.log( "Transaction store... this does signing and can take a while" );
		return super.store( /*{ sign: {pad1:-9, pad2:-9}, read:null }*/ ).then( async (id)=>{

			//await l.userMap.set( this.ledger.this.account, this );
			//await l.email.set( this.email, this );
			return this;
		} );
	}
}
		

class Account extends StoredObject{
	ledger = null;//new Transaction( this.storage );
	lastTransaction = null;//this.ledger;
	#user = null;
	constructor(storage) {
		super(storage);

		//this.ledger.to = this.ledger.to;
		//this.store();
		//
	}
	get user() {
		return this.#user;
	}
	set user(val) {
		this.#user=val;
	}
	async store() {
		//console.log( "ledger store here in account..." );
		if( !this.ledger ) {
			// storing this account, but it's new
			await super.store();
			this.ledger = new Transaction( this.storage );
			this.lastTransaction = this.ledger;
			this.ledger.ledger = this;
			this.ledger.history = this.ledger;
			await this.ledger.store();
		}
		
		await super.store(); // and re-store this if it was the first store too.
	}
}



let storage_ = null;
const AccountDb = {
	async getAccounts( userId ) {
		const accounts = await l.userMap.get( userId );
		for( let account of accounts ) {
			const account2 = await storage_.map( account, {depth:2, paths:["ledger"]} );
			// this should be able to be handled by otions...
			await storage_.map( account2.ledger );
		}
		//console.log( "return is still a promise?", accounts );
		return accounts || [];
	},
	async makeAccount( userId ) {
		const accounts = await l.userMap.get( userId );
		if( accounts ) {
			const newAccount = new Account( storage_ );
			newAccount.store(); console.log( "Stores account here, so it shouldn't be a direct object?");
			accounts.push( newAccount );
			l.accounts.push( newAccount );
			return newAccount;
		}else {
			const accounts = [new Account(storage_)];
			await accounts[0].store();
			l.accounts.push( accounts[0] );
			//console.log( "Setting user Id to accounts, which should save");
			l.userMap.set( userId, accounts );
			return accounts[0]
		}
	},
	async hook( storage ) {
		if( !storage ) 
			console.trace( "Maybe have storage before hook??" );
		//console.log( "setting hook storage accounts:", storage );
		storage_ = storage;
		BloomNHash.hook( storage );
		SlabArray.hook( storage );
		storage.addEncoders( [ { tag:"~A", p:Account, f: null },  { tag:"~T", p:Transaction, f: null } ] );
		storage.addDecoders( [ { tag:"~A", p:Account, f: null },  { tag:"~T", p:Transaction, f: null } ] );

		const root = await storage.getRoot();
		root.open( "accounts.config.jsox" ).then( (file)=>{
			return file.read().then( async (obj)=>{
				Object.assign( l.ids, obj );
				l.accounts     = await storage.get( l.ids.accountsId );
				l.userMap = await storage.get( l.ids.userMapId );
				l.configCommit = ()=>{
					file.write( l.ids );
				};
			} );
		} ).catch( (err)=>{
			root.create( "accounts.config.jsox" ).then( async (file)=>{
			l.accounts = new SlabArray( storage );
			l.accounts.hook( storage );
			l.userMap = new BloomNHash( storage );
			l.userMap.hook( storage );
				l.ids.accountsId   = await l.accounts.store();
				l.ids.userMapId   = await l.userMap.store();
				//console.log( "So?", l.ids );
		
				file.write( l.ids );
		
				l.configCommit = ()=>{
					file.write( l.ids );
				};
			} );
		} );

	},
}

Object.freeze( AccountDb );
export { AccountDb } ;
