"use strict";


var sack = require( '..' );


var https = require( 'https' );
//var constants = require('constants');


function doRequire( file ) {

  var evalFinished = false;
    var opts = {  hostname: "192.168.173.2",
                  port : "444",
                  method : "GET",
                  rejectUnauthorized: false,
                  path : "/index.html"
                };
    var now = Date.now();
    https.get( opts,   
      (res) => {
          console.log( "res?")
        const statusCode = res.statusCode;
        const contentType = res.headers['content-type'];
        res.setEncoding('utf8');
        let rawData = '';
        res.on('data', (chunk) => rawData += chunk);
        res.on('end', () => {
            evalFinished = rawData;
            // return here...
            eval( 'console.log( "do someting in eval`d code" )' );
            console.trace( "eval finished.... ", rawData )
            //return thisModule.exports;
            sack.Λ();
        });
      }).on('error', (e) => {
        console.trace(`Got error: ${e.message}`, e);
      });
      while( !evalFinished ) {
      	console.trace( "wait...",Date.now() - now );
      	sack.Δ(  );
        console.log( Date.now() - now );
      }
   return evalFinished;      
}

setTimeout( ()=>{
	console.log( "in a timeout.." )
 doRequire( "https://192.168.173.2:444/index.html" ); 
 console.log( "well... this waited...." );
 }, 100 );

console.log( "And this waited." );