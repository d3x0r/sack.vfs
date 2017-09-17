
const sack = require( ".." );

console.log( "test addr:", sack.Network.Address( "google.com", 80 ) );
console.log( "test addr:", sack.Network.Address( "google.com:443", 80 ) );

	var os = require("os");
	var interfaces = os.networkInterfaces();
	var addresses = [];
addresses.push( { family:"ipv4", address:"0.0.0.0", netmask:"0.0.0.0", broadcast:"255.255.255.255" } );
	for (var k in interfaces) {
		for (var k2 in interfaces[k]) {
			var address = interfaces[k][k2];
			calculateBroadcast(address);
			addresses.push(address);
			//console.log( "pushed an address from networkInterfaces..." );
		}
	}

const port = 3213;

var Servers = [];
var connecting = 0;

addresses.forEach( (addr)=>{
	if( addr.family == 'IPv6' ) return;
        
	console.log( new Date(), "Bind to:", addr.address );
	var listener = sack.Network.UDP({ address: addr.address
	        , port : port
	        , broadcast:true
		, readStrings : true
	      }
	      ,  (msg,rinfo)=>{
	         console.log( "received message:", msg, "from", rinfo );
	});
	listener.address = sack.Network.Address( addr.broadcast, port );

	console.log( "addr:", listener.address );

	listener.Address = addr;
	connecting++;

	Servers.push( listener );
} );

ping();

var pings = 0;
function ping() {
	pings++;
	if( pings == 4 )
		process.exit(0)
			Servers.forEach((server) => {
				console.log("Send to ", server.Address.broadcast)
				var msg = "Message Data";
				server.send( msg, server.address );
			});
	setTimeout( ping, 1000 );
}



function calculateBroadcast(addr) {
	if (addr.family == "IPv4") {
		var mask = [];
		var addrNum = [];
		var stub = [255, 255, 255, 255];
		var addrPart = [];
		var addrPartNot = [];

		var d = addr.netmask.split('.');
		mask[0] = Number(d[0])
		mask[1] = Number(d[1])
		mask[2] = Number(d[2])
		mask[3] = Number(d[3])

		var d = addr.address.split('.');
		addrNum[0] = Number(d[0])
		addrNum[1] = Number(d[1])
		addrNum[2] = Number(d[2])
		addrNum[3] = Number(d[3])

		addrPart[0] = addrNum[0] & mask[0];
		addrPart[1] = addrNum[1] & mask[1];
		addrPart[2] = addrNum[2] & mask[2];
		addrPart[3] = addrNum[3] & mask[3];

		stub[0] &= ~mask[0];
		stub[1] &= ~mask[1];
		stub[2] &= ~mask[2];
		stub[3] &= ~mask[3];

		addrPart[0] |= stub[0];
		addrPart[1] |= stub[1];
		addrPart[2] |= stub[2];
		addrPart[3] |= stub[3];

		addr.broadcast = `${addrPart[0]}.${addrPart[1]}.${addrPart[2]}.${addrPart[3]}`;
	}
}

