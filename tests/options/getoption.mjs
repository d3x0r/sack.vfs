import {sack} from "sack.vfs"
console.log( "says static exists:", sack.DB, sack.DB.eo, sack.DB.op, sack.DB.so );


console.log( sack.DB.op( "SACK/Network/Connect Timeout" ) );
console.log( sack.DB.op( "/DEFAULT/node/SACK/Network/Connect Timeout" ) );
console.log( sack.DB.op( "/comports.ini/node/.COM12/port timeout" ) );

// only starts enumerating the root - might be nice to just list sub-options
sack.DB.eo( "SACK", (opt)=>{
	console.log( "sack eo got:", opt.name, opt );
} )
sack.DB.eo( (opt)=>{
	console.log( "eo got:", opt.name, opt );
	if( opt.name === "DEFAULT" ) {
		opt.eo( ( opt)=>{
			if( opt.name === "node" ) {
				opt.eo( (opt)=>{ 
					if( opt.name==="SACK" ) opt.eo( (opt)=>console.log( "Sack opt:", opt ) ) } );
			}
			//console.log( opt.name );
		} );
	}
} )


sack.DB.eo( "SACK", (opt)=>{
	console.log( "sack(2) eo got:", opt.name, opt );
} )

function lo(root) {
	sack.DB.eo( root, (opt)=>{
		console.log( "sack(2) eo got:", opt.name, opt );
	} )


}

sack.DB.so( "SACK/Network/Connect Timeout", "10051" );
sack.DB.so( "/comports.ini/node/.COM12/port timeout", Number(sack.DB.op( "/comports.ini/node/.COM12/port timeout" ))+1 );

console.log( "After set?", sack.DB.op( "SACK/Network/Connect Timeout" ) );
