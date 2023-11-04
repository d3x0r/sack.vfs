
import {sack} from "sack.vfs"


const lbs = {
	command : "/root/bin/iptables-setup",
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
		db.makeTable( "create table banlist ( IP char(48) PRIMARY KEY, last_hit DATETIME default CURRENT_TIMESTAMP, allow int default 0 )" );
	} catch( err ) { console.log( "create failed?", err ); }
} );


const ips = lbs.db.do( "select IP from banlist where allow=0" );
let i = 0;

function ban() {
	if( i >= ips.length ) return;
	
	sack.Task( {bin:lbs.command, args:[ips[i].IP], noKill:true, end(){ ban()} } );
	i++;
}
ban();

lbs.db.close();

