"use strict";

var vfs;
try{
  vfs = require( "./build/RelWithDebInfo/sack_vfs.node" );
} catch(err) { 
  try { 
    /*console.log( err ); */
    vfs = require( "./build/Debug/sack_vfs.node" ); 
  } catch( err ){  
    try { 
      /*console.log( err ); */
      vfs = require( "./build/Release/sack_vfs.node" ); 
    } catch( err ){  
      console.log( err )
    } 
  } 
}

vfs.JSON6.stringify = JSON.stringify;
vfs.JSON.stringify = JSON.stringify;

var disk = sack.Volume();
require.extensions['.json6'] = function (module, filename) {
    var content = disk.read(filename).toString();
    module.exports = sack.JSON6.parse(content);
};


//vfs.Sqlite.op( "SACK/Summoner/Auto register with summoner?", 0 );
//vfs.Sqlite.so( "SACK/Summoner/Auto register with summoner?", 1 );
//console.log( "register:", vfs.Sqlite.op( "SACK/Summoner/Auto register with summoner?", 0 ));
//vfs.loadComplete();
const thread = vfs.Thread(process._tickDomainCallback);
module.exports=exports=vfs;

if( false ) {
	process.on( 'beforeExit', ()=> {
		//console.log( "Before Exit." );
		vfs.Thread() 
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