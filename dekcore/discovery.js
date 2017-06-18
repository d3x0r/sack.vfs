"use strict";

var config = require( "./config.js");
var bits = require( "../vfs/salty_random_generator.js").SaltyRNG();
var dns = require( 'dns');
var localAddresses;
var v6Servers = [];
var Servers=[];
var first_address;
var first_server;
var connection_count;
const port = 3213;

exports.discover = ( options )=>{
            const dgram = require('dgram');
            var os = require("os");
            //console.log( "hostname is ... ", config.run.hostname );
            dns.lookup( config.run.hostname, {
          //family: 4,
          hints: dns.ADDRCONFIG | dns.V4MAPPED,
          all: true
        },(err,a,family)=>{
                a.forEach( (addr)=>{
                    var server;
                        if( addr.family===6) {
                            if( addr.address.startsWith( "fe80::" ) )
                                return;
                            v6Servers.push( server = dgram.createSocket({type:'udp6',reuseAddr:true}) )
                        }
                        else {
                            Servers.push( server = dgram.createSocket({type:'udp4',reuseAddr:true}) )
                            if( addr.address.startsWith( "192") ) {
                                first_server = server;
                                first_address = addr;
                            }
                        }

                        console.log( addr );
                        server.bind({address:addr.address, port:port, exclusive: false,reuseAddr:true});
                        config.run.addresses.push( { address:addr,  } );

                        server.on('error', (err) => {
                          console.log(`server error:\n${err.stack}`);
                          server.close();
                        });

                        server.on('message', (msg, rinfo) => {
                            if( !rinfo.address === addr.address )
                                console.log( msg );
                            console.log( addr.address );
                            console.log(`server got: ${msg} from ${rinfo.address}:${rinfo.port}`);
                        });

                        server.on('listening', () => {
                              var address = server.address();
                              server.setBroadcast(true)
                              if( !connection_count ){
                                  console.log( "setting timeout.")
                                  setTimeout( discoverer.dispatchPing, 1000 );
                                  connection_count = 0;
                              }
                              connection_count++;
                          /*
                         if( addr.family==6 ){

                          //serverv6.bind(3213);
                          var buf = bits.getBuffer( 128-16 );
                          var wordbuf = new Uint16Array( buf );
                          var addr_tail = "";
                          for(var n = 0; n < 5; n++ )
                              addr_tail += ":" + ( wordbuf[n].toString(16)).substring( -4 );
                              console.log( "address is ff08  ", addr_tail )
                        //server.addMembership("ff08" + addr_tail );
                        }
                        */
                            console.log(`server listening ${address.address}:${address.port}`);
                        });


                        //server.setBroadcast(true)
                } )
            } )


    var discoverer = {
        pingMessage : new Buffer( "{'msgop':'whoami','id':"+config.run.Λ+"'}")
        ,dispatchPing : () => {
            console.log( "discover...")
            setTimeout( discoverer.dispatchPing, 30000 );
            if( first_server )
            {
                first_server.send( discoverer.pingMessage, 0, discoverer.pingMessage.length, port,  "192.168.1.255");
                first_server.send( discoverer.pingMessage, 0, discoverer.pingMessage.length, port,  "255.255.255.255");
                console.log( "sent to 255.255.255.255", first_server);
            }
            else {
                    Servers.forEach( ( server )=> {
                        server.send( discoverer.pingMessage, 0, discoverer.pingMessage.length, port,  "255.255.255.255");
                        return;
                    } );

                    v6Servers.forEach(  ( server )=> {
                    //    server.send( pingMessage, 0, pingMessage.length, port,  multicastgroup);
                } );
            }
            /*
            ff02::	Link Local: spans the same topological region as the corresponding unicast scope, i.e. all nodes on the same LAN.
            ff05::	Site local: is intended to span a single site
            ff08::	Organization scope: Intended to span multiple sizes within the same organization
            ff0e::	Global scope, assigned by IANA.
            ff01::	Interface local: Spans only a single interface on a node and is useful only for loopback transmission of multicast.
        */
            //setTimeout( discoverer.dispatchPing, 1000 );
            //discoverer.serverv6.send( pingMessage, 0, pingMessage.length, port,address )
            if( options.ontimeout ) {
                console.log( "can something" + options.timeout );
                setTimeout( options.ontimeout, options.timeout );
                options.ontimeout = null;
            }

        }
    };


    //setTimeout( ()=>{}, 10);
    return discoverer;
};
