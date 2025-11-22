
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

	dbdo( s, ...args ) {
		//args && console.trace( "SQL:", s, "with", args.join(',') )|| console.log( "SQL:", s );
		return this.#db.do( s, ...args );
	}
	constructor(db, opts ) {
		super();
		//console.trace( "Options:", db, db.provider, opts );
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
			} else if( this.#sqlite ) {
				this.dbdo( "create table if not exists os (id char(45) primary key,value char,updated datetime default CURRENT_TIMESTAMP )" );
				this.dbdo( "create trigger if not exists update_os_timestamp after update on os for each row begin update os set updated=CURRENT_TIMESTAMP where id=OLD.id; end;" );
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
		const records = this.dbdo( "select value from os where id=?", obj );

		if( records.length ) {
			//console.trace( "Got result:", typeof records[0].value, records[0].value, obj, !!parser );
			if( parser ) cb( parser.parse( records[0].value ) );
			else         cb( records[0].value );
		} else	cb( undefined );
	}
	writeRaw( opts, obj ) {
		//console.log( "Write Raw:", opts, obj );
		if( "string" === typeof opts ) 
			opts = {id:opts};

		if( this.#psql ) 
			this.dbdo( "insert into os (id,value)values(?,?) ON CONFLICT (id) DO UPDATE SET value=?", opts.id, obj,obj );
		else if( this.#mysql ) {
			this.dbdo( "insert into os (id,value)values(?,?) ON CONFLICT (id) DO UPDATE SET value=?", opts.id, obj,obj );
		}
		else if( this.#maria ) {
			try {
				this.dbdo( "insert into os (id,value)values(?,?)  ON DUPLICATE KEY UPDATE value=?", opts.id, obj, obj );
				//this.dbdo( "insert into os (id,value)values(?,?)  ON CONFLICT (id) DO UPDATE SET value=?", opts.id, obj, obj );
			} catch( err ) {
				console.log( "Insert error:", err );
			}
		} else
			this.dbdo( "insert into os (id,value)values(?,?) ON CONFLICT (id) DO UPDATE SET value=excluded.value", opts.id, obj );
	}
}


