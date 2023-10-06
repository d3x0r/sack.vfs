
import {sack} from "sack.vfs"

const disk = sack.Volume();

const lbs = {
	command : "/root/bin/iptables-setup",
	output : "/etc/firewall/banlist",
	DSN : "maria-firewall",
	exec_timer : 0,
	lastban : null,
	pendingKeys : new Map(), // there's a 2 part ban command...
	db : null,
	file : null
}


const configProcessor = sack.Config();
configProcessor.add( "command=%m", (c)=>lbs.command = c );
configProcessor.add( "output=%m", (c)=>lbs.output = c );
configProcessor.add( "input=%m", (c)=>{} );
configProcessor.add( "DSN=%m", (c)=>lbs.DSN = c );
configProcessor.go( "linux_syslog_scanner.conf" );

lbs.db = sack.DB( lbs.DSN, (db)=>{
	try {
		db.makeTable( "create table banlist ( IP char(48) PRIMARY KEY, last_hit DATETIME default CURRENT_TIMESTAMP )" );
	} catch( err ) { console.log( "create failed?", err ); }
} );


const ips = lbs.db.do( "select IP from banlist" );
const file = disk.File( lbs.output );
for( let ip of ips ) {
	file.writeLine( ip.IP );
	console.log( "rewrite:", ip.IP );
}

sack.Task( {bin:lbs.command, args:[lbs.output], noKill:true } );

lbs.db.close();

