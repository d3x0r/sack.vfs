
// UDP ports 137 (Name Service) and 138 (Datagram Service), and TCP port 139 (Session Service)

// 67

// 68 if DHCP authorization is required on the server

// 69

// 4011

// Random ports from 64001 through 65000*, to establish a session with the server for TFTP and multicasting

// 389


/*

135 for RPC

5040* for RPC

137–139 for SMB

389 for LDAP
*/


const TIMEOUT = 2000;

import {sack} from "sack.vfs"
import {ref} from "sack.vfs/ref"

import {addresses} from "sack.vfs/net.broadcast"

const sendto = [];
for( let a of addresses ) {
	if( a.broadcast ) {
		// ipv6 doesn't resolve broadcast
		const sendTo = sack.Network.Address( a.broadcast, 5557 );
		sendto.push( sendTo );
	}
}
console.log( "use Addresses:", addresses );

const disk = sack.Volume();

import {protocol} from "./tftp-server.mjs"
import {message as message67} from "./bootp-server.mjs"


	static  message68( msg, rinfo ) {
		console.log( "68 got message:", msg );
	}
	static  message69( msg, rinfo ) {
		console.log( "69 got message:", msg );
	}
	static  message4011( msg, rinfo ) {
		console.log( "4011 got message:", msg );
	}





const port68 = sack.Network.UDP( {port:68
			, address:"0.0.0.0"
			, broadcast: true
			, message: Stream.message68 } );

const port69 = sack.Network.UDP( {port:69
			, address:"0.0.0.0"
			, broadcast: true
			, message: Stream.message69 } );


const port67 = sack.Network.UDP( {port:67
			, address:"0.0.0.0"
			, broadcast: true
			, message: Stream.message67 } );

const port4011 = sack.Network.UDP( {port:4011
			, address:"0.0.0.0"
			, broadcast: true
			, message: Stream.message4011 } );

