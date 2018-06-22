"use strict";

var sack;
try{
  sack = require( "./build/RelWithDebInfo/sack_gui.node" );
} catch(err1) {
  try {
    //console.log( err1 );
    sack = require( "./build/Debug/sack_gui.node" );
  } catch( err2 ){
    try {
      //console.log( err2 );
      sack = require( "./build/Release/sack_gui.node" );
    } catch( err3 ){
      console.log( err1 )
      console.log( err2 )
      console.log( err3 )
    }
  }
}

sack.JSON6.stringify = JSON.stringify;
sack.JSON.stringify = JSON.stringify;
//vfs.
var disk = sack.Volume();
require.extensions['.json6'] = function (module, filename) {
    var content = disk.read(filename).toString();
    module.exports = sack.JSON6.parse(content);
};


//sack.Sqlite.op( "SACK/Summoner/Auto register with summoner?", 0 );
//sack.Sqlite.so( "SACK/Summoner/Auto register with summoner?", 1 );
//console.log( "register:", sack.Sqlite.op( "SACK/Summoner/Auto register with summoner?", 0 ));
//sack.loadComplete();
if (process._tickDomainCallback || process._tickCallback)
    sack.Thread(process._tickDomainCallback || process._tickCallback);

module.exports=exports=sack;

if( false ) {
	process.on( 'beforeExit', ()=> {
		//console.log( "Before Exit." );
		sack.Thread()
	});
	process.on( 'exit', ()=> {
		//console.log( "Process Exit." );
	});
	process.on( 'uncaughtException', (err)=> {
		console.log( err )
	});
	process.on('SIGUSR2',function(){
        	console.log('SIGUSR2 recieved')
	        //socketcluster.killWorkers();
        	//socketcluster.killBrokers();
	        //process.exit(1)
	})
}