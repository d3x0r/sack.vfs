
import {sack} from "sack.vfs"
import {addSettings, subnetOf, recentBans} from "./subnet.mjs"

const lbs = {
	command : "/root/bin/iptables-setup",
	output : "/etc/firewall/banlist",
	DSN : "maria-firewall",
	exec_timer : 0,
	pendingKeys : new Map(), // there's a 2 part ban command...
	db : null,
	file : null
}


const configProcessor = sack.Config();
const settings = addSettings( configProcessor, lbs );
configProcessor.add( "output=%m", (c)=>lbs.output = c );
configProcessor.add( "input=%m", (c)=>{
	const words = c.split( " " );
	sack.Task( { bin:words[0], args:words.slice( 1 ).join(" " ), input(buf) {
			//console.log( "Buf is:", buf );
			processor.write( buf );
		} } );
} );
configProcessor.add( "DSN=%m", (c)=>lbs.DSN = c );
configProcessor.go( "linux_syslog_scanner.conf" );
settings.resolve();
const banned = recentBans( lbs.banSeconds * 1000 );

lbs.db = sack.DB( lbs.DSN, (db)=>{
	try {
		db.makeTable( "create table banlist ( IP char(48) PRIMARY KEY, last_hit DATETIME default CURRENT_TIMESTAMP, allow int default 0 )" );
	} catch( err ) { console.log( "create failed?", err ); }
} );


const processor = sack.Config();

	processor.add( "%w - - [%m] \"GET /CherryWeb %m", failed_http );
	processor.add( "%w - - [%m] \"GET //a2billing/customer/templates/default/footer.tpl %m", failed_http );
	processor.add( "%w - - [%m] \"GET /assets/jnkp.php  %m", failed_http );
	processor.add( "%w - - [%m] \"GET /recordings%m", failed_http );
	// bot check-ins to an API this server doesn't have
	processor.add( "%w - - [%m] \"POST /api/heartbeat %m", failed_http );
	processor.add( "%w - - [%m] \"POST /api/sysinfo %m", failed_http );

processor.on( "unhandled", (str)=> console.log( "Unhandled line:", str ) );


// args follow the pattern: %w (IP), %m (date), %m (rest of request), then the whole line
function failed_http( ip, date, rest, line ) {
	AddBan( ip );
}

function ExecFirewall( )
{
	lbs.exec_timer = 0;
}

function AddBan( IP )
{
	const net = subnetOf( IP, lbs.v4prefix, lbs.v6prefix );
	if( !net ) { console.log( "not an address, ignoring:", IP ); return; }
	if( lbs.db ) {
		const result = lbs.db.do( "select allow from banlist where IP=?", IP )
		if( result && result[0] ) {
			if( Number( result[0].allow ) ) {
				console.log( "allow-listed, not banning:", IP );
				return;
			}
			// already known; re-banning below restarts its timeout
			lbs.db.do( "update banlist set last_hit=now() where IP=?", IP );
		} else {
			console.log( "insert IP:", IP );
			lbs.db.do( "insert into banlist (IP) values(?)", IP );
		}
	}
	// the firewall already drops this network; lines still arriving were in flight
	if( banned.check( net.cidr ) ) return;
	console.log( "ban:", net.cidr, "for", IP );
	sack.Task( {bin:lbs.command, args:[net.cidr, lbs.banSeconds + "s"] } );
}


