
// 
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

const port = sack.Network.UDP( {port:5557
			, address:"0.0.0.0"
			, broadcast: true
			, message } );


class Block {
	number = 0;
	data =null;
	constructor( number, data ) {
		this.number = number;
		this.data = data;
	}
	next = null;
	me = null;
}

class Stream {

	reading = false;
	file = null;
	block = 1;

	fence_block = null; // this is up-to packet stream sent... should maintain a list of what was actually sent for re-transmission
	timeout_timer = 0;
	first_block = null;
	last_block = null;

	addBlock( blk ) {
		if( this.last_block ) {
			blk.me = new ref( this.last_block, "next" );
		} else 
			blk.me = new ref( this, "first_block" );

		this.last_block = ( blk.me['*'] = blk );
	
	}

	constructor( file, mode, rinfo ) {
		switch( mode.toLowerCase() ) {
		default:
			throw new Error( "Bad mode:" + mode );
			console.log( "unhandled mode specified, ban client?", file, mode, rinfo );
			break;
		case "netascii":
			break;
		case "octet":
			break;
		case "mail":
			break;
		}
		this.mode = mode;
		if( !disk.exists( file ) ) {
			throw new Error( "No such file:"+file );
		}
		// sack.Volume.mapFile( file );
		this.file = disk.File( file );
		this.rinfo = rinfo; // send back directed always?
	}


	static  message( msg, rinfo ) {
		console.log( "got message:", msg );
		const op = msg[1] | msg[0] << 8;
		switch( op ) {
		case 1: /* Read request RRQ */
		case 2: /* Write request WRQ */
			const fn = [];
			const md = [];
			let i;
			for( i = 2; i < msg.byteLength; i++ ) {
				const c = msg[i];
				if( c ) {
					fn.push( String.fromCodePoint( c ) );
				} else break;
			}
			i++; // step off the 0.
			if( i < msg.byteLength ) {
				for( ; i < msg.byteLength; i++ ) {
					const c = msg[i];
					if( c ) {
						md.push( String.fromCodePoint( c ) );
					} else break;
				}			
			}
			if( i < msg.byteLength ) {
				try {
					if( op ===1 ) {
						new Stream( fn.join(''), md.join(''), rinfo ).read();
					} else {
						new Stream( fn.join(''), md.join(''), rinfo ).write();
					}
				} catch( err ) {
				}
			}
			break;
		case 3: /* Data DATA */
			const block = msg[3] | (msg[2] << 8);
			const len = msg.byteLength - 4;
			if( len === 512 ) {
				this.more = true;
			} else {
				this.more = false;
			}
			break;
		case 4: /* Acknowledgement ACK */
			const block = (msg[2]<<8) | msg[3];
			// this shouldn't be a block in the future, so it must be a duplicate, ignore it.
			if( this.first_block.number === block ) {
				const blk = (this.first_block.me['*'] = this.first_block.next);
				this.send();	
			}
			break;
		case 5: /* Error (ERROR) */
			break;
		}
	}

	send() {
		let msg = this.first_block;
		do
		{
			port.send( msg.data, this.rinfo );
			if( !this.fence_block )
				this.fence_block = msg.next;

			if( this.timeout_timer )
				clearTimeout( this.timeout_timer );

			this.timeout_timer = setTimeout( ()=>{
					this.timeout_timer = 0;
					this.send();  // re-send packets
				}, TIMEOUT );
			msg = msg.next;
		}
		while( msg != this.fence_block );
	}

	read() {
		this.reading = true;
		for( let n = 0; ; n++ ) {
			const data = this.file.read( 512 );
			const msg = new Uint8Array( data.byteLength + 4 );
			const blk = this.block+n;
			for( let b = 0; b < data.byteLength; b++ ) {
				msg[0] = 0;
				msg[1] = 3;
				msg[2] = (blk) >> 8;
				msg[3] = (blk) & 0xFF;
				msg[4+b] = data[b];
			}
			this.addBlock( new Block( blk, data ) );
		}
		this.send();
	}

	write() {                
		this.writing = true;
	}

}