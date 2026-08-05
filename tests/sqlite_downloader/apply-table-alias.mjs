#!/usr/bin/env node
// apply-table-alias.mjs
//
// Applies the sqlite3_column_table_alias() patch to a stock SQLite
// amalgamation (sqlite3.c / sqlite3.h / sqlite3ext.h).
//
//   https://sqlite.org/forum/forumpost/fdb0bb7ad03d6b21
//   https://github.com/sqlite/sqlite/compare/master...d3x0r:sqlite:sqlite3_column_table_alias
//
// Hunks are anchored on stable code landmarks rather than line numbers or
// diff context, so the script survives amalgamation reshuffles between
// releases.  Every hunk must match its expected count exactly; on any
// mismatch nothing is written and the failing hunk is named, so a future
// upstream change surfaces as a loud, specific error instead of a silent
// mis-merge.
//
// Usage:  node apply-table-alias.mjs <amalgamation-dir>
//
// Notes vs the original fork diff (deliberate deviations):
//  - generateColumnTypes' !SQLITE_ENABLE_COLUMN_METADATA branch gets the
//    correct 6-arg columnType() call (the fork diff had 7 args -- a latent
//    compile error in a preprocessor-dead branch).
//  - the redundant early columnType() call the fork diff added to
//    sqlite3SubqueryColumnTypes (rebase artifact) is not reproduced.
//  - src/test1.c and test/capi3f.test hunks are not amalgamation files.

import { readFileSync, writeFileSync, existsSync } from "fs";
import { join } from "path";

const dir = process.argv[2];
if( !dir ) {
	console.error( "Usage: node apply-table-alias.mjs <amalgamation-dir>" );
	process.exit( 2 );
}

// ---- replacement text fragments -------------------------------------------

const HDR_DECLS =
`SQLITE_API const char *sqlite3_column_table_alias(sqlite3_stmt*,int);
SQLITE_API const void *sqlite3_column_table_alias16(sqlite3_stmt*,int);`;

const DOC_OLD = `** the origin_ routines return the column name.`;
const DOC_NEW =
`** the origin_ routines return the column name.  _table_alias_ results
** with the alias(if any) of the table associated with the column;
** _table_ always returns the source table name, even if it has been
** aliased, this returns the original table name if there is no alias.`;

const EXT_STRUCT_MEMBERS =
`  /* Version 3.53.0 and later */
  const char * (*column_table_alias)(sqlite3_stmt*,int);
  const void * (*column_table_alias16)(sqlite3_stmt*,int);
`;

const EXT_MACROS =
`/* Version 3.53.0 and later */
#define sqlite3_column_table_alias     sqlite3_api->column_table_alias
#define sqlite3_column_table_alias16   sqlite3_api->column_table_alias16
`;

const VDBEAPI_FUNCS = `

/*
** Return the alias or, if no alias specified, the name of the table from
** which a result column derives. NULL is returned if the result name is
** an expression or constant or anything else which is not an unambiguous
** reference to a database table.
*/
SQLITE_API const char *sqlite3_column_table_alias(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, 0, COLNAME_TABLE_ALIAS);
}
#ifndef SQLITE_OMIT_UTF16
SQLITE_API const void *sqlite3_column_table_alias16(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, 1, COLNAME_TABLE_ALIAS);
}
#endif /* SQLITE_OMIT_UTF16 */`;

const COLNAME_BLOCK =
`#define COLNAME_NAME        0
#define COLNAME_DECLTYPE    1
#define COLNAME_DATABASE    2
#define COLNAME_TABLE       3
#define COLNAME_COLUMN      4
#define COLNAME_TABLE_ALIAS 5
#ifdef SQLITE_ENABLE_COLUMN_METADATA
# define COLNAME_N          6      /* Number of COLNAME_xxx symbols */`;

// ---- hunk engine -----------------------------------------------------------
//
// find:  string (exact) or RegExp (must be /g)
// count: exact number of required matches
// probe: text whose presence means "already applied" -> hunk is skipped
// apply: (file text) => new text; default is replace-all of `find`

function countOf( s, find ) {
	if( find instanceof RegExp ) return [...s.matchAll( find )].length;
	let n = 0, at = -1;
	while( ( at = s.indexOf( find, at + 1 ) ) >= 0 ) n++;
	return n;
}

const hunks = [];
function hunk( name, files, spec ) { hunks.push( { name, files, ...spec } ); }

// -- header declarations (sqlite3.h + its embedded copy in sqlite3.c)
hunk( "api declarations", [ "sqlite3.h", "sqlite3.c" ], {
	probe: "sqlite3_column_table_alias16(sqlite3_stmt*,int);",
	find: "SQLITE_API const void *sqlite3_column_origin_name16(sqlite3_stmt*,int);",
	count: 1,
	insertAfter: "\n" + HDR_DECLS,
} );

hunk( "api documentation", [ "sqlite3.h", "sqlite3.c" ], {
	probe: "_table_alias_ results",
	find: DOC_OLD,
	count: 1,
	replace: DOC_NEW,
} );

// -- extension api (sqlite3ext.h + its embedded copy in sqlite3.c)
hunk( "sqlite3_api_routines members", [ "sqlite3ext.h", "sqlite3.c" ], {
	probe: "(*column_table_alias)",
	find: /(struct sqlite3_api_routines \{[\s\S]*?)(\n\};)/g,
	count: 1,
	apply: ( s, f ) => s.replace( f.find, ( m, body, close ) => body + "\n" + EXT_STRUCT_MEMBERS.replace( /\n$/, "" ) + close ),
} );

hunk( "extension redirect macros", [ "sqlite3ext.h", "sqlite3.c" ], {
	probe: "#define sqlite3_column_table_alias ",
	find: "#endif /* !defined(SQLITE_CORE) && !defined(SQLITE_OMIT_LOAD_EXTENSION) */",
	count: 1,
	insertBefore: EXT_MACROS,
} );

// -- loadext.c null stubs: first block (no COLUMN_METADATA) gets both,
//    second block (OMIT_UTF16) gets only the utf16 stub.
hunk( "loadext null stubs", [ "sqlite3.c" ], {
	probe: "# define sqlite3_column_table_alias  ",
	find: "# define sqlite3_column_table_name16    0",
	count: 2,
	apply: ( s, f ) => {
		let i = 0;
		return s.replaceAll( f.find, ( m ) => ++i === 1
			? m + "\n# define sqlite3_column_table_alias     0\n# define sqlite3_column_table_alias16   0"
			: m + "\n# define sqlite3_column_table_alias16   0" );
	},
} );

// -- loadext.c sqlite3Apis array: append entries at the very end so the
//    order matches the appended sqlite3_api_routines members.
hunk( "sqlite3Apis entries", [ "sqlite3.c" ], {
	probe: "  sqlite3_column_table_alias,",
	find: /(static const sqlite3_api_routines sqlite3Apis = \{[\s\S]*?)(\n\};)/g,
	count: 1,
	apply: ( s, f ) => s.replace( f.find, ( m, body, close ) => {
		//if( !/,\s*$/.test( body ) ) body += ",";
		return body + "\n  /* Version 3.53.0 and later */\n  ,sqlite3_column_table_alias\n  ,sqlite3_column_table_alias16" + close;
	} ),
} );

// -- vdbe.h COLNAME_* constants
hunk( "COLNAME constants", [ "sqlite3.c" ], {
	probe: "#define COLNAME_TABLE_ALIAS",
	find: /#define COLNAME_NAME\s+0\n#define COLNAME_DECLTYPE\s+1\n#define COLNAME_DATABASE\s+2\n#define COLNAME_TABLE\s+3\n#define COLNAME_COLUMN\s+4\n#ifdef SQLITE_ENABLE_COLUMN_METADATA\n# define COLNAME_N\s+5[^\n]*/g,
	count: 1,
	replace: COLNAME_BLOCK,
} );

// -- select.c columnType()/columnTypeImpl()
hunk( "columnType macro (metadata)", [ "sqlite3.c" ], {
	probe: "columnType(A,B,C,D,E,F) columnTypeImpl(A,B,C,D,E,F)",
	find: "# define columnType(A,B,C,D,E) columnTypeImpl(A,B,C,D,E)",
	count: 1,
	replace: "# define columnType(A,B,C,D,E,F) columnTypeImpl(A,B,C,D,E,F)",
} );

hunk( "columnType macro (no metadata)", [ "sqlite3.c" ], {
	probe: "columnType(A,B,C,D,E,F) columnTypeImpl(A,B)",
	find: "# define columnType(A,B,C,D,E) columnTypeImpl(A,B)",
	count: 1,
	replace: "# define columnType(A,B,C,D,E,F) columnTypeImpl(A,B)",
} );

hunk( "columnTypeImpl signature", [ "sqlite3.c" ], {
	probe: "const char **pzOrigTabAlias",
	find: "  const char **pzOrigCol\n#endif\n){",
	count: 1,
	replace: "  const char **pzOrigCol,\n  const char **pzOrigTabAlias\n#endif\n){",
} );

hunk( "columnTypeImpl locals", [ "sqlite3.c" ], {
	probe: "char const *zOrigTabAlias = 0;",
	find: "  char const *zOrigCol = 0;\n#endif",
	count: 1,
	replace: "  char const *zOrigCol = 0;\n  char const *zOrigTabAlias = 0;\n#endif",
} );

hunk( "capture table alias", [ "sqlite3.c" ], {
	probe: "zOrigTabAlias =\n",
	find: "pS = pTabList->a[j].u4.pSubq->pSelect;\n          }else{\n            pS = 0;\n          }",
	count: 1,
	insertAfter: "\n#ifdef SQLITE_ENABLE_COLUMN_METADATA\n          zOrigTabAlias =\n              pTabList->a[j].zAlias?pTabList->a[j].zAlias:pTabList->a[j].zName;\n#endif",
} );

hunk( "recursive call (subquery)", [ "sqlite3.c" ], {
	probe: "columnType(&sNC, p,&zOrigDb,&zOrigTab,&zOrigCol,&zOrigTabAlias)",
	find: "zType = columnType(&sNC, p,&zOrigDb,&zOrigTab,&zOrigCol);",
	count: 1,
	replace: "zType = columnType(&sNC, p,&zOrigDb,&zOrigTab,&zOrigCol,&zOrigTabAlias);",
} );

hunk( "recursive call (select)", [ "sqlite3.c" ], {
	probe: "      zType = columnType(&sNC, p, &zOrigDb, &zOrigTab, &zOrigCol, &zOrigTabAlias);",
	find: "\n      zType = columnType(&sNC, p, &zOrigDb, &zOrigTab, &zOrigCol);",
	count: 1,
	replace: "\n      zType = columnType(&sNC, p, &zOrigDb, &zOrigTab, &zOrigCol, &zOrigTabAlias);",
} );

hunk( "output parameter store", [ "sqlite3.c" ], {
	probe: "*pzOrigTabAlias = zOrigTabAlias;",
	find: "    *pzOrigCol = zOrigCol;",
	count: 1,
	insertAfter: "\n    *pzOrigTabAlias = zOrigTabAlias;",
} );

hunk( "generateColumnTypes call", [ "sqlite3.c" ], {
	probe: "    const char *zOrigTabAlias = 0;",
	find: "    const char *zOrigCol = 0;\n    zType = columnType(&sNC, p, &zOrigDb, &zOrigTab, &zOrigCol);",
	count: 1,
	replace: "    const char *zOrigCol = 0;\n    const char *zOrigTabAlias = 0;\n    zType = columnType(&sNC, p, &zOrigDb, &zOrigTab, &zOrigCol, &zOrigTabAlias);",
} );

hunk( "generateColumnTypes SetColName", [ "sqlite3.c" ], {
	probe: "COLNAME_TABLE_ALIAS, zOrigTabAlias",
	find: "    sqlite3VdbeSetColName(v, i, COLNAME_COLUMN, zOrigCol, SQLITE_TRANSIENT);",
	count: 1,
	insertAfter: "\n    sqlite3VdbeSetColName(v, i, COLNAME_TABLE_ALIAS, zOrigTabAlias, SQLITE_TRANSIENT);",
} );

// generateColumnTypes' #else branch + sqlite3SubqueryColumnTypes
hunk( "no-metadata columnType calls", [ "sqlite3.c" ], {
	probe: "columnType(&sNC, p, 0, 0, 0, 0);",
	find: "zType = columnType(&sNC, p, 0, 0, 0);",
	count: 2,
	replace: "zType = columnType(&sNC, p, 0, 0, 0, 0);",
} );

// -- vdbeapi.c accessors
hunk( "column_table_alias accessors", [ "sqlite3.c" ], {
	probe: "sqlite3_column_table_alias(sqlite3_stmt *pStmt",
	find: "  return columnName(pStmt, N, 1, COLNAME_COLUMN);\n}\n#endif /* SQLITE_OMIT_UTF16 */",
	count: 1,
	insertAfter: VDBEAPI_FUNCS,
} );

// ---- run -------------------------------------------------------------------

const fileNames = [ "sqlite3.c", "sqlite3.h", "sqlite3ext.h" ];
const files = {};
for( const f of fileNames ) {
	const p = join( dir, f );
	if( !existsSync( p ) ) { console.error( "missing: " + p ); process.exit( 2 ); }
	files[f] = readFileSync( p, "utf8" );
}

const errors = [];
let applied = 0, skipped = 0;

for( const h of hunks ) {
	for( const f of h.files ) {
		let s = files[f];
		if( h.probe && s.includes( h.probe ) ) {
			console.log( `= ${f}: ${h.name} (already applied)` );
			skipped++;
			continue;
		}
		const n = countOf( s, h.find );
		if( n !== h.count ) {
			errors.push( `${f}: ${h.name}: expected ${h.count} anchor match(es), found ${n}` );
			continue;
		}
		if( h.apply )            s = h.apply( s, h );
		else if( h.insertAfter )  s = s.replaceAll( h.find, ( m ) => m + h.insertAfter );
		else if( h.insertBefore ) s = s.replaceAll( h.find, ( m ) => h.insertBefore + m );
		else                      s = s.replaceAll( h.find, h.replace );
		files[f] = s;
		console.log( `+ ${f}: ${h.name}` );
		applied++;
	}
}

if( errors.length ) {
	console.error( "\nFAILED - nothing written:" );
	for( const e of errors ) console.error( "  ! " + e );
	process.exit( 1 );
}

for( const f of fileNames ) writeFileSync( join( dir, f ), files[f] );
console.log( `\nOK: ${applied} hunk(s) applied, ${skipped} skipped, files updated in ${dir}` );
