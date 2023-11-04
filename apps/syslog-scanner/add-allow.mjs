
import {sack} from "sack.vfs"


const lbs = {
        DSN : "maria-firewall",
	db : null,
}


const configProcessor = sack.Config();
configProcessor.add( "DSN=%m", (c)=>lbs.DSN = c );
configProcessor.go( "linux_syslog_scanner.conf" );

lbs.db = sack.DB( lbs.DSN, (db)=>{
	try {
		db.makeTable( "create table banlist ( IP char(48) PRIMARY KEY, last_hit DATETIME default CURRENT_TIMESTAMP, allow int default 0 )" );
	} catch( err ) { console.log( "create failed?", err ); }
} );

lbs.db.do( "update banlist set allow=1 where IP=?", process.argv[2] );

lbs.db.close();
