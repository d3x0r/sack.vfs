
import {sack} from "sack.vfs"
import {addSettings, subnetOf} from "./subnet.mjs"


const lbs = {
	command : "/root/bin/iptables-setup",
	DSN : "maria-firewall",
	db : null,
}


const configProcessor = sack.Config();
const settings = addSettings( configProcessor, lbs );
configProcessor.add( "output=%m", (c)=>lbs.output = c );
configProcessor.add( "input=%m", (c)=>{} );
configProcessor.add( "DSN=%m", (c)=>lbs.DSN = c );
configProcessor.go( "linux_syslog_scanner.conf" );
settings.resolve();

lbs.db = sack.DB( lbs.DSN, (db)=>{
	try {
		db.makeTable( "create table banlist ( IP char(48) PRIMARY KEY, last_hit DATETIME default CURRENT_TIMESTAMP, allow int default 0 )" );
	} catch( err ) { console.log( "create failed?", err ); }
} );


// Re-apply bans still running (e.g. after a reboot), one per network, each
// with the time left since that network's latest hit.  With firewall=nft,
// run nft-setup.sh first so the sets exist.
const rows = lbs.db.do( `select IP,last_hit from banlist where allow=0 and last_hit>now()-interval ${lbs.banSeconds} second` ) || [];
const nets = new Map(); // cidr -> seconds left
for( const row of rows ) {
	const net = subnetOf( row.IP, lbs.v4prefix, lbs.v6prefix );
	if( !net ) continue;
	const age = ( Date.now() - new Date( row.last_hit ).getTime() ) / 1000;
	let left = Math.ceil( lbs.banSeconds - age );
	if( !( left > 0 ) ) left = lbs.banSeconds; // unparsable date: give it the full time
	nets.set( net.cidr, Math.max( left, nets.get( net.cidr ) || 0 ) );
}
lbs.db.close();

const todo = [ ...nets ];
console.log( "re-banning", todo.length, "networks from", rows.length, "addresses" );
let i = 0;

function ban() {
	if( i >= todo.length ) return;
	const [ cidr, left ] = todo[i++];
	sack.Task( {bin:lbs.command, args:[cidr, left + "s"], noKill:true, end(){ ban()} } );
}
ban();
