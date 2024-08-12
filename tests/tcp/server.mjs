import {sack} from "sack.vfs"


const server = sack.Network.TCP( { readStrings:true, port:1234, address:"[::0]", connect( pc ) {
	console.log( "connected:", pc );
		pc.on( "message", (buf)=>{
			console.log( "(message)I mean this should be associated with pc, not server...", buf );
			pc.readStrings = !pc.readStrings;
		} )
		pc.on( "close", ()=>{
			console.log( "(close)I mean this should be associated with pc, not server..." );
		} )
	
}, 
message( buf ) {
	console.log( "Got buffer:" , buf );
	
},
close() {
	console.log( "Socket closed..." );
}


} );


