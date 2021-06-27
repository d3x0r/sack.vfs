
import {sack} from "sack.vfs";

const AsyncFunction = Object.getPrototypeOf( async function() {} ).constructor;

//open( { protocol: "userDatabaseClient"
//      , server : "ws://localhost:8089/"
//    } );

function open( opts ) {
	const protocol = opts?.protocol || "protocol";
	const server = opts.server;
	console.log( "connect with is:", server, protocol );
	var client = sack.WebSocket.Client( server, protocol, { perMessageDeflate: false } );
        
	client.on("open", function (ws)  {
		console.log( "Connected (service identification in process; consult config .jsox files)" );
		//console.log( "ws: ", this ); //  ws is also this
		this.onmessage = ( msg_ )=> {
			const msg = sack.JSOX.parse( msg_ );
			if( msg.op === "addMethod" ) {
				try {
					var f = new AsyncFunction( "Import", msg.code );
					const p = f.call( this, (m)=>import(m) );
				} catch( err ) {
					console.log( "Function compilation error:", err,"\n", msg.code );
				}
			}
			else {
				if( this.processMessage && !this.processMessage( msg )  ){
					if( msg.op === "authorize" ) {
						// expect a connection from a user.
						opts.authorize( msg.user );
					}
					else {
						console.log( "unknown message Received:", msg );
					}
				}
			}
       	};
		this.on( "close", function( msg ) {
        		console.log( "opened connection closed" );
        	        //setTimeout( ()=> {console.log( "waited" )}, 3000 )
	        } );
		//client.send( "Connected!" );
		//client.send( msg );
	       	//client.send( msgtext );
                //client.send( "." );
	} );

	client.on( "close", function( msg ) {
      		console.log( "unopened connection closed" );
	} );

} 




function handleMessage( ws, msg ) {
	if( msg.op === "addMethod" ) {
		
	}
}

export const UserDbRemote = {
    open(opts) {
    	const realOpts = Object.assign( {}, opts );
        realOpts.protocol= "userDatabaseClient";
 	realOpts.server = opts.server || "ws://localhost:8089/";	
	return open(realOpts);
    }
}

// return
// return UserDbRemote;//"this usually isn't legal?";
