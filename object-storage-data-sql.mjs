
import {ObjectStore} from "./object-storage-data.mjs"

export class DbStorage extends ObjectStore {
	// this is a super simple key value store in a (postgresql) database.
	// some support to fall back to sqlite - but will require manually specifying options
	// need to update personality detection in C library.
	#db = null;
	#psql = false;
	#maria = false;
	#mysql = false;
	#sqlite = false;
	constructor(db, opts ) {
		console.trace( "Options:", db, db.provider, opts );
		this.#db = db;
		if( db.provider === 3 )
			this.#psql = true;     
		else if( db.provider === 2 )
			this.#mysql = true;     
		else if( db.provider === 5 )
			this.#maria = true;  // really is mariadb   
		if( db.provider === 1 )
			this.#sqlite = true;     

		{
			if( this.#psql ) {
				const table1 = "create table if not exists os (id char(45) primary key,value varchar(4096),updated datetime default CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP)";
				db.makeTable( table1 );
			} if( this.#maria || this.#mysql ) {
				const table1 = "create table if not exists os (id char(45) primary key,value LONGTEXT,updated datetime default CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP)";
				db.makeTable( table1 );
			} else if( !this.#sqlite ) {
				db.makeTable( "create table os (id char(45) primary key,value char,updated datetime default CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP)" );
			}
		}
	}
	read( obj, ...args ) {
		let version = 0;
		let parser = null;
		let cb = null;
		//console.log( "Read in object storage interface? should it have been a file?", obj );
		for( let arg of args ) {
		if( "function" === typeof arg ) {
			cb = arg;
		} else if( "object" === typeof arg ) {
			parser = arg;
		} else if( "number" === typeof arg ) {
			version = arg;
		}
		}
		const records = this.#db.do( "select value from os where id=?", obj );
		//console.log( "Thing?", obj, records );
		if( records.length ) {
			
			const o = parser.parse( records[0].value );
			cb( o );
		} else	cb( undefined );
	}
	writeRaw( opts, obj ) {
		//console.log( "Write Raw:", opts, obj );
		if( this.#psql ) 
			this.#db.do( "insert into os (id,value)values(?,?) ON CONFLICT (id) DO UPDATE SET value=?", opts.id, obj,obj );
		else if( this.#mysql ) {
			this.#db.do( "insert into os (id,value)values(?,?) ON CONFLICT (id) DO UPDATE SET value=?", opts.id, obj,obj );
		}
		else if( this.#maria ) {
			try {
				this.#db.do( "insert into os (id,value)values(?,?)  ON DUPLICATE KEY UPDATE value=?", opts.id, obj, obj );
				//this.#db.do( "insert into os (id,value)values(?,?)  ON CONFLICT (id) DO UPDATE SET value=?", opts.id, obj, obj );
			} catch( err ) {
				console.log( "Insert error:", err );
			}
		} else
			this.#db.do( "insert into os (id,value)values(?,?) ON CONFLICT (id) DO UPDATE SET value=excluded.value", opts.id, obj );
	}
}


