"use strict";

var fs = require( 'fs');
const numCPUs = require('os').cpus().length;
const crypto = require( 'crypto');
const tls = require( 'tls');

var id_generator;
var config = require( './config.js');
var keyManager = require( "./id_manager.js" );
// server listening 0.0.0.0:41234

exports.Cluster = ()=>{
    return {
        ID : config._
        , isMaster : true
        , start: startCluster
    }
};

function startCluster() {
        if (cluster.isMaster) {
      // Fork workers.
            console.log( "here");

              for (var i = 0; i < numCPUs; i++) {
                cluster.fork();
              }

              cluster.on('exit', (worker, code, signal) => {
                  if( exports.Cluster.isMaster )
                    cluster.fork();
                else

                console.log(`worker ${worker.process.pid} died`);
              });
        } else {
            scriptServer();
        }
}


function scriptServer() {
    console.log( "Started Script Services");
    var privateKey = fs.readFileSync('ca-key.pem').toString();
    var certificate = fs.readFileSync('ca-cert.pem').toString();
    var option = {key: privateKey, cert: certificate};
    var credentials = tls.createSecureContext ();
    tls.createServer( option,

  // Workers can share any TCP connection
  // In this case it is an HTTP server
//  var server = http.createServer(
        (req, res) => {
      //req.connection.Socket.

      console.log( "got request" + req.url );
      if( req.url=== '/' )   {
            res.writeHead(200);
            res.end('<HTML><head><script src="hello.js"></script></head></HTML>');
        } else /*if( req.url === '' )*/ {
            let relpath;
            if( fs.access( relpath = req.url.substring(1,req.url.length), (err)=>{
                if( !err ) {
                    console.log( "Existed as a file...");
                }
            }))

            console.log( "and truth?");
            //console.log( req );
            console.log( req.url);
            res.writeHead(404);
            res.end('<HTML><head><script src="core/no such thing.js"></script></head></HTML>');

        }
    });
}

function GetFrom( path ) {
    res.pipe(file);
    let path1=path.lastIndexOf( "/");
    //let path2=path.lastIndexOf( "\")
    var file = fs.createWriteStream(path);
    var request = http.get("http://" + authority + "//" + path, function(response) {
        response.pipe( file );
    });
}
