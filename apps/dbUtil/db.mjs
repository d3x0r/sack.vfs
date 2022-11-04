
// what does this do with database?
// Mysql -
//   get table format... 
//   like parse the creates statement... 
//     or query information schema?

import {sack} from "sack.vfs";

class Db2 extends sack.Sqlite {
	
	constructor(db) {
		super(db);
	}
}

class Sqlite {
	static loadSchema( db, table ) {
		const newtable = new Table(db, table, true);
		return newtable;
	}

}

class MySQL {
	
	static loadSchema( db, table ) {
		const newtable = new Table(db, table);
		return newtable;
	}
}

class Table {
	name = null;
	columns = [];
	cols = {};

	indexes = [];
	keys = {};

	db = null;
	sqlite = false;
	constructor( db_, tableName, sqlite ) {
		this.db = db_;
		this.name = tableName;
		this.sqlite = sqlite;
		if( db_ ) 
			if( sqlite ) 
				this.loadSqliteColumns( db_ )
			else
				this.loadColumns(db_);
	}

	has( col ) {
		return( col in this.cols )
	}

	addColumn( name, type, extra ) {
			const t = type.split( '(');
			const p = ( t.length > 1 )?Number( t[1].slice( 0, -1 )):0;

			const newCol = new Column();
			newCol.name = name;
			newCol.type = t[0];
			newCol.precision = p;
			if( extra && extra.includes( "DEFAULT" ) ) {
				newCol.default = tokenAfter( extra, "DEFAULT" );				
			}
			if( extra && (extra.includes( "AUTO_INCREMENT" ) || extra.includes( "auto_increment" )) ) {
				newCol.auto_increment = true;
				newCol.primary_key = true;
			}
			if( extra && !extra.includes( "NOT NULL" ) )
				newCol.nullable = false;

			this.columns.push( newCol );
			this.cols[name] = newCol;
			this.db.do( `ALTER TABLE ${this.name} ADD COLUMN ${newCol.name} ${type} ${extra || ""}` );
	}

	loadColumns(db) {
		const cols = db.do( "select * from INFORMATION_SCHEMA.COLUMNS where TABLE_NAME=? and TABLE_SCHEMA=DATABASE() ORDER BY ORDINAL_POSITION", this.name );
		for( let col of cols ) {
			const newcol = new Column();
			newcol.name = col.COLUMN_NAME;
			const t = col.COLUMN_TYPE.split( '(');
			if( t.length > 1 ) {
				newcol.precision = Number(t[1].slice( 0, -1 ));
			}
			newcol.nullable = col.IS_NULLABLE==="YES";
			newcol.default = col.COLUMN_DEFAULT;
			newcol.type = t[0];
			if( col.COLUMN_KEY==="PRI" ) 
				newcol.primary_key = true;
			if( col.EXTRA ==="auto_incrememnt" )
				newcol.auto_increment = true
			//console.log( "COL:", newcol );
			this.columns.push( newcol );
			this.cols[newcol.name] = newcol;
			//const 
			//col.COLUMN_NAME
			//col.DATA_TYPE
			//col.COLUMN_TYPE // includes DATA_TYPE + (precision)
		}		
		const keys = db.do( "select * from INFORMATION_SCHEMA.STATISTICS where TABLE_NAME=? and TABLE_SCHEMA=DATABASE()", this.name );
		for( let key of keys ) {
			const index = this.keys[key.INDEX_NAME] || new Index();
			//console.log( "index?", index );
			index.name = key.INDEX_NAME;
			index.unique = !key.NON_UNIQUE;	
			index.columns.push( key.COLUMN_NAME );
			if( !this.keys[key.INDEX_NAME] ) {
				this.keys[key.INDEX_NAME] = index;
				this.indexes.push( index );
			} else {
				
			}
		}

	}	
	
	loadSqliteColumns(db) {
		//const cols_ = db.do( "select * from sqlite_schema" );
		const cols = db.do( `PRAGMA table_info(${this.name})` );
		//const cols2 = db.do( `PRAGMA table_info('stuffkey')` );
		/*
  {
    type: 'table',
    name: 'table1',
    tbl_name: 'table1',
    rootpage: 2,
    sql: 'CREATE TABLE `table1` (`id` INTEGER PRIMARY KEY,`stuff` int(11))'
  }
  {
    type: 'index',
    name: 'stuffkey',
    tbl_name: 'table2',
    rootpage: 4,
    sql: "CREATE INDEX 'stuffkey' ON 'table2'('stuff')"
  }

  {
    cid: 0,
    name: 'id',
    type: 'INTEGER',
    notnull: 0,
    dflt_value: null,
    pk: 1
  },

*/
		//console.log( "Sqlite info:", cols_ );
		//console.log( "Sqlite info:", cols );
		//console.log( "Sqlite info:", cols2 );
		//process.exit(0)
		for( let col of cols ) {
			const newcol = new Column();
			newcol.name = col.name;
			const t = col.type.split( '(');
			if( t.length > 1 ) {
				newcol.precision = Number(t[1].slice( 0, -1 ));
			}
			newcol.nullable = !col.notnull;
			newcol.default = col.dflt_value;
			newcol.type = t[0];
			newcol.type_ = col.type;

			newcol.primary_key = !!col.pk;

//			if( col.EXTRA ==="auto_incrememnt" )
//				newcol.auto_increment = true

			//console.log( "COL:", newcol );
			this.columns.push( newcol );
			this.cols[newcol.name] = newcol;
			//const 
			//col.COLUMN_NAME
			//col.DATA_TYPE
			//col.COLUMN_TYPE // includes DATA_TYPE + (precision)
		}		
/*
		const keys = db.do( "select * from INFORMATION_SCHEMA.STATISTICS where TABLE_NAME=? and TABLE_SCHEMA=DATABASE()", this.name );
		for( let key of keys ) {
			const index = this.keys[key.INDEX_NAME] || new Index();
			//console.log( "index?", index );
			index.name = key.INDEX_NAME;
			index.unique = !key.NON_UNIQUE;	
			index.columns.push( key.COLUMN_NAME );
			if( !this.keys[key.INDEX_NAME] ) {
				this.keys[key.INDEX_NAME] = index;
				this.indexes.push( index );
			} else {
				
			}
		}
*/
	}	
}

class Index {
	name = null;
	unique = false;
	columns = [];
}

class Column {
	name = null;
	type = null; // SQL expression for type
	type_ = null; // sqlite idea of type
	precision = 0;
	default = null;
	nullable = false;
	auto_increment = false;
	primary_key = false;
	
}

//console.log( "TeST:", tokenAfter( "DEFAULT '0'", "DEFAULT" ) );
//console.log( "TeST:", tokenAfter( "DEFAULT '0' NOT NULL", "DEFAULT" ) );
//console.log( "TeST:", tokenAfter( "DEFAULT 123", "DEFAULT" ) );
//console.log( "TeST:", tokenAfter( "DEFAULT 123 NOT NULL", "DEFAULT" ) );

function tokenAfter( s, token ) {
	let at = s.indexOf( token );
	if( s < 0 ) throw new Error( "This expects you already now the token is in the string." );
	let from = 0;
	at += token.length;
	while( s[at] == ' ' ) at++;
	if( s[at] == "'" ) {
		from = at;
		at++;
		while( s[at] != "'" ){
			if( at >= s.length ) throw new Error( "invalid SQL token after:", token );
			at++;
		}
	} else {
		from = at;
		while( s[at] != " " && at < s.length ) at++;
	}
	return s.substr( from, at-from+1 );
}


class Db {
	db = null;
	MySQL=MySQL;
	Sqlite=Sqlite;
	constructor() {
	}
}


const db = new Db();                         
//console.log( "Why isn't db good?", db );
export default db;
