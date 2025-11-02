

export class StoredObject {
	#id = null;
	#storage = null;
	get id() { 
		return this.#id;
	} 
	async store(opts) {
		if( !this.#id ) {
			// might have been reloaded...
			const container = this.#storage.getContainer( this );
			if( container ) this.#id = container.id;
			opts = opts || {id:this.#id};
			opts.id = opts.id || this.#id;
			const id = await this.#storage.put( this, opts );
			if( !this.#id ) this.#id = id;
			else if( this.#id !== id ) { console.log( "Object has been duplicated: old/new id:", this.#id, id ); }
			return id;
		} else {
			opts = opts || {id:this.#id};
			opts.id = opts.id || this.#id;
			const id = await this.#storage.put( this, opts );
			if( this.#id !== id ) { console.log( "Object has been duplicated: old/new id:", this.#id, id ); }
			return id;
		}
	}
	hook( storage ) {
		if( storage instanceof StoredObject ) {
			this.#storage = storage.#storage;
		}else
			this.#storage = storage;
	}

	loaded( storage,id ) {
		//console.trace( "stored object loaded callback:", id );
		this.#storage = storage;
		this.#id = id;
	}
	constructor( storage ) {
        	if( !storage ) {
	        	//console.trace( "should have storage on create..", storage, currentStorage );
				storage = currentStorage;
	        	if( !storage ) {
		        	console.trace( "should have storage on create..", storage, currentStorage );
			}
		}
		if( storage ) this.#storage = storage;
	}
	get storage() {
		return this.#storage;
	}
}
