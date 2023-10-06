
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


const processor = sack.Config();
processor.add( '%m "enp3s0" ip saddr %m/%i counter packets %i bytes %i drop', (leader,ip,mask,packet,bytes)=>{
	AddBan( ip );	
} );
//processor.on( "unhandled", (str)=> console.log( "Unhandled line:", str ) );

console.log( "read old bans?" );
processor.go( "/tmp/oldbans" );
console.log( "did read old bans?" );

lbs.db.close();


function AddBan( IP )
{
	console.log( "ban IP:", IP );
	if( lbs.db ) {
		const result = lbs.db.do( "select 1 from banlist where IP=?", IP )
		if( result && result[0] ) {
			console.log( "already banned? %s\n", IP );
			lbs.db.do( "update banlist set last_hit=now() where IP=?", IP );
			return; // no need to include this one.
		} else {
			console.log( "insert IP:", IP );
			lbs.db.do( "insert into banlist (IP) values(?)", IP );
		}
	}
}


