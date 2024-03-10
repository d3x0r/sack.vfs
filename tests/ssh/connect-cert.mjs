import {sack} from "sack.vfs"

const enc = new TextDecoder("utf-8")
const priv = enc.decode( sack.Volume.mapFile( "rsa" ) );
console.log( "Priv?", priv );
const pub = enc.decode( sack.Volume.mapFile( "rsa.pub" ) );

const ssh = sack.SSH();
const ssh_alt = new sack.SSH();

ssh.connect( { address:"localhost:2222", port:22, trace:true
             , user:"root", password: ""
             , privKey: priv
             , pubKey:null  // pubKey can be gotten from the private key
             , trace: false
             , connect() {
             	// connect callback?
             }
             }
             ).then( (fingerprint)=>{
					console.log( "Connect worked forssh",  fingerprint );
					console.log( "Make channel1:" );
             	ssh.Channel( { type:"session", windowSize: 4096, packetSize: 4096 } ).then( (channel)=>{
						console.log( "First channel result?", channel );
					} );
             	console.log( "Make channel2:" );
             	ssh.Channel( ).then( (channel)=>{
						console.log( "Generic channel result:", channel );
                	channel.read( (buf)=>{
                        	console.log( "Channel data:", buf );
                        } );
                	channel.pty().then( async ()=>{
								console.log( "Pty done" );
                        await channel.setenv( "FOO", "bar" ).then( ()=>{
									console.log( "setEnv Done" );

								} ).catch( (err)=>{ console.log( "Environment error:", err ) } );
                        channel.shell().then( ()=>{
									console.log( "Shell started..." );
                        	channel.send( "ls\n" );
                        } );

							} );
                } );
             } ).catch( (err)=>{
			console.log( "Connect error:", err ); } 
		);
             
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

