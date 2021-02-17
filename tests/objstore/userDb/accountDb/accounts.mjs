



import {StoredObject} from "../commonDb.mjs"
import { SlabArray } from "./SlabArray.mjs";
import {BloomNHash} from "../bloomNHash.mjs"


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
		return super.store( { sign: true, read:null } ).then( async (id)=>{

			//await l.userMap.set( this.ledger.this.account, this );
			//await l.email.set( this.email, this );
			return this;
		} );
	}
}
		

class Account extends StoredObject{
	ledger = new Transaction( this.storage );
	#user = null;
	constructor(storage) {
		super(storage);
		this.ledger.ledger = this;
		this.ledger.history = this.ledger;
		this.ledger.to = this.ledger.to;
		//
	}
	get user() {
		return this.#user;
	}
	async store() {
		await this.ledger.store();
		await super.store();
	}
}



let storage_ = null;
const AccountDb = {
	async getAccounts( userId ) {
		const accounts = await l.userMap.get( userId );
		return accounts || [];
	},
	async makeAccount( userId ) {
		const accounts = await l.userMap.get( userId );
		if( accounts ) {
			const newAccount = new Account( storage_ );
			newAccount.store();
			accounts.push( newAccount );
			l.accounts.push( newAccount );
			return newAccount;
		}else {
			const accounts = [new Account(storage_)];
			accounts[0].store();
			l.accounts.push( accounts[0] );
			l.userMap.set( userId, accounts );
			return accounts[0]
		}
	},
	async hook( storage ) {
		console.log( "setting hook storage accounts:", storage );
		storage_ = storage;
		BloomNHash.hook( storage );
		SlabArray.hook( storage );
		storage.addEncoders( [ { tag:"~A", p:Account, f: null },  { tag:"~T", p:Transaction, f: null } ] );
		storage.addDecoders( [ { tag:"~A", p:Account, f: null },  { tag:"~T", p:Transaction, f: null } ] );

		const root = await storage.getRoot();
		console.log( "Test:", root );
			root.open( "accounts.config.jsox" ).then( (file)=>{
				return file.read().then( async (obj)=>{
					Object.assign( l.ids, obj );
					console.log( "GOT:", l.ids );
					l.accounts     = await storage.get( l.ids.accountsId );
					l.userMap = await storage.get( l.ids.userMapId );
					l.configCommit = ()=>{
						file.write( l.ids );
					};
				} );
			} ).catch( (err)=>{
				root.create( "accounts.config.jsox" ).then( async (file)=>{
				l.accounts = new SlabArray( );
				l.accounts.hook( storage );
				l.userMap = new BloomNHash( );
				l.userMap.hook( storage );
					l.ids.accountsId   = await l.accounts.store();
					l.ids.userMapId   = await l.userMap.store();
					console.log( "So?", l.ids );
		
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
