
import {sack} from "sack.vfs"


const lbs = {
        DSN : "maria-firewall",
	unban : "unban.sh",
	db : null,
}


const configProcessor = sack.Config();
configProcessor.add( "DSN=%m", (c)=>lbs.DSN = c );
configProcessor.add( "unban command=%m", (c)=>lbs.unban = c );
configProcessor.go( "linux_syslog_scanner.conf" );

lbs.db = sack.DB( lbs.DSN, (db)=>{
	try {
		db.makeTable( "create table banlist ( IP char(48) PRIMARY KEY, last_hit DATETIME default CURRENT_TIMESTAMP, allow int default 0 )" );
	} catch( err ) { console.log( "create failed?", err ); }
} );

lbs.db.do( "update banlist set allow=1 where IP=?", process.argv[2] );

console.log( "trying command:", lbs.unban, [process.argv[2]] );
sack.Task( { bin:lbs.unban, args:[process.argv[2]], end() { console.log( "Task Ended..." );}, input:console.log.bind(console), input2:console.log.bind(console) } );

lbs.db.close();
