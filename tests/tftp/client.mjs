import {sack} from "sack.vfs"
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

const port = sack.Network.UDP( {port:5558, address:"0.0.0.0", message:(msg,rinfo)=>{ console.log( "got message:", msg ) } } );

const sendTo = sack.Network.Address( "255.255.255.255:5557" );

console.log( "Address?", sendTo );
port.send( "Ping?", sendTo );

for( let s of sendto ) {

	port.send( "Ping?", s );

}
