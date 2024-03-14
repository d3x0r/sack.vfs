import {sack} from "sack.vfs"

const enc = new TextDecoder("utf-8"); // convert arraybuffer to string
const priv = enc.decode( sack.Volume.mapFile( "rsa" ) );

const ssh = sack.SSH();
const ssh_alt = new sack.SSH();

ssh.connect( { address:"[::1]:2222", port:22, trace:true
             , user:"root", password: ""
             , privKey: priv
             , pubKey:null  // pubKey can be gotten from the private key
             , trace: false
             }
           ).then( (fingerprint)=>{
					console.log( "Connect worked forssh",  fingerprint );
					ssh.reverse( "::0", 8022 ).then( (listener)=>{
						console.log( "Reverse connection listener:", listener );
						const ws = sack.WebSocket.ssh2ws( listener );
						console.log( "New websocket server:", ws );
						ws.onrequest = (request,response)=>{
							console.log( "Request happened:", request, response );
							response.writeHead( 404 );
							response.end( "<HTML><HEAD><title>404</title></HEAD><BODY>404</BODY></HTML>");

						};
						ws.onaccept = accept;
						ws.onconnect = (ws)=>{
								console.log( "Connected ws?", ws );
							ws.onmessage = (msg)=>{
								console.log( "Received message:", msg );						
								ws.send( msg );
							}
							ws.onclose = (code, reason ) =>console.log( "websocket close:", code, reason );
							ws.send( "Say Something?" );
						}
						
					} );
             } ).catch( (err)=>{
                      	console.log( "Connect error:", err ); 
						process.exit(); // maybe try a different connection?
				} 
                      );
             

function accept(ws){ console.log( "Accept ws?", ws ); this.accept( "protocol" ); return true}