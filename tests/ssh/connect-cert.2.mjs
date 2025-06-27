import {sack} from "sack.vfs"

const enc = new TextDecoder("utf-8"); // convert arraybuffer to string
const priv = enc.decode( sack.Volume.mapFile( "rsa" ) );
//console.log( "Priv?", priv );

const ssh = sack.SSH();

ssh.connect( { address:"localhost:22", port:22, trace:true
             , user:"d3x0r", password: ""
             , privKey: priv
             , pubKey:null  // pubKey can be gotten from the private key
             , trace: true
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
			channel.readBinary = true;
                	channel.pty().then( async ()=>{
								console.log( "Pty done" );
                        await channel.setenv( "FOO", "bar" ).then( ()=>{
									console.log( "setEnv Done" );

								} ).catch( (err)=>{ console.log( "Environment error:", err ) } );
                        channel.shell().then( ()=>{
									console.log( "Shell started..." );
									channel.send( "cd /\nls\n" );
									//setTimeout( ()=>{ channel.send( "cd /\nls\n" );}, 100 );
                        } );

							} );
                } );
             } ).catch( (err)=>{
			console.log( "Connect error:", err ); } 
		);
             

