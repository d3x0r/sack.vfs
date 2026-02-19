import {sack} from "sack.vfs"

const ssh = sack.SSH();
const ssh_alt = new sack.SSH();

ssh.connect( { address:"10.173.0.1", port:22
             , user:"root", password: "password"
             , privKey: null
             , pubKey:null  // pubKey can be gotten from the private key
             , trace: false
             }
           ).then( (fingerprint)=>{
                console.log( "Connect worked forssh",  fingerprint, [...(new Uint8Array( fingerprint))].map(x=>x.toString(16).padStart(2,'0')).join('') );
                console.log( "Make channel1:" );
             	ssh.Channel( { type:"session", message: "pre-message", windowSize: 4096, packetSize: 4096 } ).then( (channel)=>{
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
            } ).catch( (err)=>console.log( "Failed:", err ) );
             
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

