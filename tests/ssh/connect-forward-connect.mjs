import {sack} from "sack.vfs"

const ssh = sack.SSH();
const ssh_alt = new sack.SSH();

ssh.connect( { address:"localhost:2222", port:22
             , user:"root", password: "password"
             , privKey: null
             , pubKey:null  // pubKey can be gotten from the private key
             , trace: false
             }
           ).then( (fingerprint)=>{
                console.log( "Connect worked forssh",  fingerprint );
                console.log( "Make channel1:" );
             	ssh.Channel( { type:"session", message: "pre-message", windowSize: 4096, packetSize: 4096 } ).then( (channel)=>{
                   console.log( "First channel result?", channel );
                } );


					const host = "localhost";
					const hostport = 4321;
					const originator = "here";
					const origport = 0;

					const encoder = new TextEncoder();
					const hostbytes = encoder.encode(host);
					const origbytes = encoder.encode(originator);

					const buffer = new ArrayBuffer( hostbytes.length + origbytes.length + 8 + 8 );
					const bytes = new Uint8Array( buffer );
					const dv = new DataView( buffer );
					dv.setUint32( 0                                       , hostbytes.length );
					dv.setUint32( 4 + hostbytes.length                    , hostport );
					dv.setUint32( 8 + hostbytes.length                    , origbytes.length );
					dv.setUint32( 12 + hostbytes.length + origbytes.length, origport );

             	ssh.Channel( { type:"direct-tcpip", message: buffer, windowSize: 4096, packetSize: 4096 } ).then( (channel)=>{
                   console.log( "First channel result?", channel );
                } );

             	console.log( "Make channel2:" );
             	ssh.Channel( ).then( (channel)=>{
                   console.log( "Generic channel result:", channel );
                   channel.read( (buf)=>{
                      console.log( "Channel data:", buf );
                   } );
                   channel.pty();
                   channel.setenv( "FOO", "bar" );
                   channel.shell().then( ()=>{
                      channel.send( "ls\n" );
                   } );
                } );
            } );
             
if( false ) {
	// test various address connection errors to be thrown.
	ssh_alt.connect( { address: "10.173.0.2" } ).catch( (err)=>{
		console.log( "Addr error:", err );
      ssh_alt.connect( {address:"10.173.0.1:23" } ).catch( (err)=>{
      	console.log( "port error:", err );
              ssh_alt.connect( {address:"1000.1.2.3"} ).catch( (err)=>{	                	
	      	console.log( "number error:", err );
	      } )
      } );
	} );
}

