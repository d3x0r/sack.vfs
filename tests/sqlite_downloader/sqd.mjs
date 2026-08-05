import {sack} from "sack.vfs"

sack.HTTPS.get( { hostname:"sqlite.org", path:"download.html", onReply(res) {
		console.log( "good?", res );
	} } );


//setTimeout( ()=>{}, 5000 );