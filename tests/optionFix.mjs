import {sack} from "sack.vfs";

sack.DB.so( "TLS", "Root Path", "/etc/ssl/certs" );
//#const opts = sack.DB( "~/.Freedom Collective/option.db" );
const com19 = sack.DB.op( "COM PORTS", "com19", "9600,something", "COMPORTS.INI" );
console.log( "Com19:", com19 );
const opt = sack.DB.op( "TLS", "Root Path","/etc/ssl/certs" );
const opt1 = sack.DB.op( "TLS/Root Path","/etc/ssl/certs" );

console.log( "op?", opt, opt1 );


const opt2 = sack.DB.op( "TLS", "Root Path","/etc/ssl/certs", "COMPORTS.INI" );

//#opts.


let level = 1;
sack.DB.eo( dump );

function dump (a,b){
	if( b === '.' ) return;
	try {
		console.log( "layer", level, ":", a, b );
		level++;
	a.eo( dump );
	level--;
	}catch(err){ console.log( err ) }
}
