

export class ObjectStore {
	// this is a super simple key value store in a (postgresql) database.
	// some support to fall back to sqlite - but will require manually specifying options
	// need to update personality detection in C library.
	constructor() {
	}
	put( obj, id, res, rej ) {
		return this.writeRaw( id, obj ).then( res ).catch(rej);
	}
	get( opts ) {
		throw new Error( "get method of object storage driver must be overridden!" );
	}
	read( obj, ...args ) {
		throw new Error( "get or read method of object storage driver must be overridden!" );
	}
	writeRaw( opts, obj ) {
		throw new Error( "put or writeRaw method of object storage driver must be overridden!" );
	}
}


