
import {sack} from "sack.vfs"


const lbs = {
	command : "/root/bin/iptables-setup",
	DSN : "maria-firewall",
	exec_timer : 0,
	lastban : null,
	pendingKeys : new Map(), // there's a 2 part ban command...
	pendingFailAuth : new Map(), // there's a 2 part ban command...
	db : null,
	file : null
}


const configProcessor = sack.Config();
configProcessor.add( "command=%m", (c)=>lbs.command = c );
configProcessor.add( "input=%m", (c)=>{
	const words = c.split( " " );
	sack.Task( { bin:words[0], args:words.slice( 1 ).join(" " ), input(buf) { 
			//console.log( "Buf is:", buf );
			processor.write( buf );
		} } );
} );
configProcessor.add( "DSN=%m", (c)=>lbs.DSN = c );
configProcessor.go( "linux_syslog_scanner.conf" );

lbs.db = sack.DB( lbs.DSN, (db)=>{
	try {
		db.makeTable( "create table banlist ( IP char(48) PRIMARY KEY, last_hit DATETIME default CURRENT_TIMESTAMP )" );
	} catch( err ) { console.log( "create failed?", err ); }
} );


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

processor.add( "%m Disconnected from invalid user %w %w port %i [preauth]", failed_pass );

processor.add( "%m Failed password for %w from %w port %i ssh2", failed_pass );
processor.add( "%m Failed password for %w from %w", failed_pass );
processor.add( "%m Invalid user %w from %w", failed_pass );
processor.add( "%m Invalid user %w from %w port %i", failed_pass );

//Sep 25 20:14:29 tower sshd[2659362]: error: kex_exchange_identification: Connection closed by remote host
//Sep 25 20:14:29 tower sshd[2659362]: Connection closed by 156.59.134.94 port 53812
processor.add( "%m sshd[%i]: error: kex_exchange_identification: Connection closed by remote host", failed_key );
processor.add( "%m sshd[%i]: Connection closed by %w port %i", failed_key_close );

processor.add( "%m Disconnected from authenticating user %w %w port %i [preauth]", fail_auth );
processor.add( "%m Connection closed by authenticating user %w %w port %i [preauth]", fail_auth );
processor.add( "%m Unable to negotiate with %w port %i: no matching host key type found. Their offer: %m [preauth]", fail_negotiate );

processor.on( "unhandled", (str)=> console.log( "Unhandled line:", str ) );


function fail_negotiate( leader, ip, port, offer, line ) {
	AddBan( ip, line );
}

function fail_auth( leader, user, ip, port, line ) {
	const oldTries = lbs.pendingFailAuth.get( ip );

	if( !oldTries ) {
		lbs.pendingFailAuth.set( ip, [ Date.now() ] );
		console.log( "Auth rule: first", line );
	} else {
		const now = Date.now();
		console.log( "Auth rule:", oldTries.length, line );
		oldTries.push( now );
		let i;
		for( i = 0; i < oldTries.length; i++ ) {
			if( ( now - oldTries[i] ) < 3600_000 ) break;
		}
		if( i )
			oldTries.splice( 0, i );
		if( oldTries.length > 5 ) {
			AddBan( ip, line );
			lbs.pendingFailAuth.delete( ip );
		}
	}
}

function failed_user_single( leader, ip, line ) {
	AddBan( ip, line );
}

function failed_pass3( leader, ip, port, line ) {
	AddBan( ip, line );
}

function failed_pass2( leader, user, ip, port, line ) {
	AddBan( ip, line );
}

function failed_pass( leader, user, ip, port, line ) {
	AddBan( ip, line );
}

function failed_key( leader, pid, line ) {
	lbs.pendingKeys.set( pid, true );
	console.log( "Handled:", line );
}

function failed_key_close( leader, pid, ip, port, line ) {
	//console.log( "Got pid:", pid );
	if( lbs.pendingKeys.get( pid ) ) {
		lbs.pendingKeys.delete( pid );
		AddBan( ip, line );
		//console.log( "fkc", leader, ip, port );
	}
}



function AddBan( IP, line )
{
	console.log( "Handled:", line );
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
	if( !lbs.lastban || lbs.lastban.localeCompare( IP ) ) {
		sack.Task( {bin:lbs.command, args:[IP] } );
	}
}


function testProcess() {
	console.log( "Test processing..." );
	processor.write( "Test failed line...\n" );
	console.log( "should have been an unknown" );

	processor.write( "Sep 25 22:00:57 tower sshd[2677773]: Did not receive identification string from 1.1.1.1\n" );
	processor.write( "Sep 25 22:00:57 tower sshd[2677773]: Failed password for invalid user admin from 180.165.132.101 port 50554 ssh2\n" );
	processor.write( "Sep 25 22:02:27 tower sshd[2677696]: fatal: Timeout before authentication for 115.205.228.215 port 1767\n" );
	processor.write( `Oct 06 05:19:15 tower systemd-resolved[420]: Using degraded feature set TCP instead of UDP for DNS server 10.173.0.1.
Oct 06 05:19:15 tower systemd-resolved[420]: Using degraded feature set UDP instead of TCP for DNS server 10.173.0.1.
Oct 06 05:24:49 tower systemd-resolved[420]: Using degraded feature set TCP instead of UDP for DNS server 10.173.0.1.
Oct 06 05:24:49 tower systemd-resolved[420]: Using degraded feature set UDP instead of TCP for DNS server 10.173.0.1.
` );
	processor.write( "Sep 25 22:02:27 tower sshd[123]: error: kex_exchange_identification: Connection closed by remote host\n" );
	processor.write( "Sep 25 22:02:27 tower sshd[123]: Connection closed by 1.2.3.4 port 55555\n" );
}
//testProcess();
