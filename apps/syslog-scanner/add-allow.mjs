
import {sack} from "sack.vfs"
import {addSettings, subnetOf} from "./subnet.mjs"

// usage: node add-allow.mjs <ip>
// Marks the address as allowed and lifts the ban on its whole network
// (bans are per network, so that's the only way to let it through).

const lbs = {
	DSN : "maria-firewall",
	db : null,
}


const configProcessor = sack.Config();
const settings = addSettings( configProcessor, lbs );
configProcessor.add( "DSN=%m", (c)=>lbs.DSN = c );
configProcessor.go( "linux_syslog_scanner.conf" );
settings.resolve();

const IP = process.argv[2];
const net = subnetOf( IP, lbs.v4prefix, lbs.v6prefix );
if( !net ) {
	console.log( "not an address:", IP );
	process.exit( 1 );
}

lbs.db = sack.DB( lbs.DSN, (db)=>{
	try {
		db.makeTable( "create table banlist ( IP char(48) PRIMARY KEY, last_hit DATETIME default CURRENT_TIMESTAMP, allow int default 0 )" );
	} catch( err ) { console.log( "create failed?", err ); }
} );

lbs.db.do( "update banlist set allow=1 where IP=?", IP );

console.log( "trying command:", lbs.unban, [net.cidr] );
sack.Task( { bin:lbs.unban, args:[net.cidr], end() { console.log( "Task Ended..." );}, input:console.log.bind(console), input2:console.log.bind(console) } );

lbs.db.close();
