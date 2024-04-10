import {sack} from "sack.vfs"

let done = null;

function checkServer() {
	sack.HTTP.get( {method:"GET", hostname: '127.0.0.1', port : 8082, path : "/", onReply(result){
		if( result.statusCode === 200 ) {
			done( true );
			console.log( "found server" );
		} else {
			console.log( "waiting for server" );
			setTimeout( checkServer, 500 );
		}
	} } );

			      
}

export async function go() {
	return new Promise( (res,rej)=>{
		done = res;
		checkServer();
	} );
}
