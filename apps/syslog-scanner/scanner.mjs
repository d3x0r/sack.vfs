
import {sack} from "sack.vfs"

const disk = sack.Volume();

const lbs = {
	command : "/root/bin/iptables-setup",
	output : "/etc/firewall/banlist",
	DSN : "maria-firewall",
	exec_timer : 0,
	lastban : null
}


const configProcessor = sack.Config();
configProcessor.add( "command=%m", (c)=>lbs.command = c );
configProcessor.add( "output=%m", (c)=>lbs.output = c );
configProcessor.add( "DSN=%m", (c)=>lbs.DSN = c );
configProcessor.go( "linux_syslog_scanner.conf" );




const processor = sack.Config();
processor.add( "%m Did not receive identification string from %w", failed_user_single );

processor.add( "%m Disconnected from %w port %w [preauth]", failed_pass3 );
processor.add( "%m Did not receive identification string from %w port %i", failed_pass3 );
processor.add( "%m Failed password for invalid user %w from %w port %i ssh2", failed_pass2 );
processor.add( "%m Connection closed by %w port %w [preauth]", failed_pass3 );
processor.add( "%m Unable to negotiate with %w port %w: no matching key exchange method found. Their offer: diffie-hellman-group1-sha1", failed_pass3 );
processor.add( "%m Unable to negotiate with %w port %w: no matching key exchange method found. Their offer: diffie-hellman-group1-sha1 [preauth]", failed_pass3 );
processor.add( "%m Bad protocol version identification %m from  %w port %i", failed_pass );
processor.add( "%m fatal: Timeout before authentication for %w port %i", failed_pass3 );

processor.add( "%m Failed password for %w from %w port %i ssh2", failed_pass );
processor.add( "%m Failed password for %w from %w", failed_pass );
processor.add( "%m Invalid user %w from %w", failed_pass );
processor.add( "%m Invalid user %w from %w port %i", failed_pass );

//Sep 25 20:14:29 tower sshd[2659362]: error: kex_exchange_identification: Connection closed by remote host
//Sep 25 20:14:29 tower sshd[2659362]: Connection closed by 156.59.134.94 port 53812
processor.add( "%m sshd[%i]: error: kex_exchange_identification: Connection closed by remote host", failed_key );
processor.add( "%m sshd[%i]: Connection closed by %w port %i", failed_key_close );


processor.on( "unhandled", (str)=> console.log( "Unhandled line:", str ) );

function testProcess() {
	console.log( "Test processing..." );
	processor.write( "Test failed line..." );
	console.log( "should have been an unknown" );
	processor.write( "Sep 25 22:00:57 tower sshd[2677773]: Did not receive identification string from 1.1.1.1" );
	processor.write( "Sep 25 22:00:57 tower sshd[2677773]: Failed password for invalid user admin from 180.165.132.101 port 50554 ssh2" );
	processor.write( "Sep 25 22:02:27 tower sshd[2677696]: fatal: Timeout before authentication for 115.205.228.215 port 1767" );
}
testProcess();

function failed_user_single( leader, ip ) {
	console.log( "fus" );
}

function failed_pass3( leader, ip ) {
	
	console.log( "fus" );
}

function failed_pass2( leader, ip ) {
	console.log( "fus" );
	
}

function failed_pass( leader, ip ) {
	console.log( "fus" );
	
}

function failed_key( leader, ip ) {
	console.log( "fus" );
	
}

function failed_key_close( leader, ip ) {
	
	console.log( "fus" );
}



function ExecFirewall( )
{
	var task1 = sack.Task( {bin:lbs.command} );
	lbs.exec_timer = 0;
}

function AddBan( IP )
{
	if( lbs.db ) {
		const result = lbs.db.do( "select id from banlist where IP=?", IP )
		if( result && result[0] ) {
			console.log( "already banned %s\n", IP );
			lbs.db.do( "update banlist set last_hit=now() where id=?", result[0] );
			return; // no need to include this one.
		} else {
			lbs.db.do( "insert into banlist (IP) values(?)", IP );
		}
	}
	if( !lbs.lastban || lbs.lastban.localeCompare( IP ) ) {
		const file = disk.File( lbs.output );

		console.log( "add", IP );
		{
			file.writeLine( IP );
			if( lbs.exec_timer )
				clearTimeout( lbs.exec_timer );

			lbs.exec_timer = setTimeout( ExecFilewall, 1000 );
		}
		lbs.lastban = IP;
	}
}
