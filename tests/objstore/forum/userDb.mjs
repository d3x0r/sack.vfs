
// users can .. view, write, edit
//    users cannot view write edit
//    users can, on some forums, view write edit
//
// admin can ... modify users abilities to do the above.
//

const l = {
	ids : {
            	userId : null,
            }
        users : null;
}

class User extends StoredObject {
    name = null;
    permissions = {

    };
    banned = false;

    constructor( opt ) {
        this.UID = opt.UID;
        this.name = opt.name;
    }

}

User.get = async function() {
	return await l.users.get( id );
}

User.create = async function( id, name ) {
    	const user = new User( { UID:id, name:name } );
	await user.store();
        return user;
}




const UserDb = {
	async hook( storage ) {
            	l.storage = storage;
		BloomNHash.hook( storage );
		storage.addEncoders( [ { tag:"~U", p:User, f: null }
                	//,  { tag:"~D", p:Device, f: null }
                	//,  { tag:"~I", p:UniqueIdentifier, f: null }
                                     ] );
		storage.addDecoders( [ { tag:"~U", p:User, f: null }
                	//,  { tag:"~D", p:Device, f: null }
                	//,  { tag:"~I", p:UniqueIdentifier, f: null }
                                     ] );

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
				l.users   = await storage.get( l.ids.userId );
				//l.email     = await storage.get( l.ids.emailId );
				//l.account   = await storage.get( l.ids.accountId );
				//l.reconnect = await storage.get( l.ids.reconnectId );
			} catch( err){
				console.log( "User Db Config ERR:", err );
				const file = await root.create( "userdb.config.jsox" );
				
				l.users   = new BloomNHash();
				l.users.hook( storage );
                                /*
				l.account   = new BloomNHash();
				l.account.hook( storage );
				l.email     = new BloomNHash();
				l.email.hook( storage );
				l.reconnect = new BloomNHash();
				l.reconnect.hook( storage );
                                */
				l.ids.userId    = await l.users.store();
				//l.ids.accountId   = await l.account.store();
				//l.ids.emailId     = await l.email.store();
				//l.ids.reconnectId = await l.reconnect.store();

				file.write( l.ids );
			}
                	//if( initResolve )
	                //	initResolve();
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
	Device:Device,
	UniqueIdentifier:UniqueIdentifier,
}
