
import {sack} from "sack.vfs"

const target = "www.bingodemos.com"
const proxies = [
	{port:444, toPort:443},
	{port:8150, toPort:8150},
	{port:8099, toPort:8099},
	{port:8092, toPort:8092},
	{port:8100, toPort:8100},
	{port:8101, toPort:8101},
	{port:8112, toPort:8112},

];


function makeProxy( opts ) {
	const sock1 = sack.Network.TCP( { port:opts.port, connect(pc){
		const pending = [];
		let open = false;
			//console.log( "new socket, right?", pc, target + ":"+opts.toPort );
			const outSock = sack.Network.TCP( { toAddress: target + ":"+opts.toPort, message( buf) {
					pc && pc.send( buf );
				}
				, close() {
					console.log( "Close event on out side" );
					pc.close();
					pc = null;
				}
				, connect(err) {
					//console.log( "relay socket made", err );
					open = true;
					for( let buf of pending ) outSock.send( buf );
					pending.length = 0;
				} } );
			pc.on( "message", (buf)=>{
				if( !open ) pending.push(buf)
				else outSock.send( buf )
			} );
			pc.on( "close", ()=>{ console.log( "Close event on server side" ); outSock.close() } );
		} 
	} );
}

for( let proxy of proxies )
	makeProxy( proxy );
