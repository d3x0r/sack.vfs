

import {sack} from "sack.vfs"

const opts = {  hostname:  'localhost',
       				  port : 8084,
					  method : "GET",
					  path : "/math/index.html",
                                          onReply( res ) {
                                          		//console.log( "async response:", res );
                                          }
					};


function req(resolve ) {
	opts.onReply = (response)=>{
		resolve( response )
	};
	sack.HTTP.get( opts )
}


const p = [];
for( let n = 0; n < 1000; n++ ) {
	p.push( new Promise( (res,rej)=>req(res) ) );
}

console.log( p );
Promise.all(p).then( ()=>{
	console.log( "got all." )
} );