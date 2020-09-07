"use strict";

var sack;
try {
	sack = require('sack' );
} catch( err ) {
}
if( !sack )
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

require( "./sack-jsox.cjs" )(sack);
require( "./object-storage.cjs" )(sack);
//vfs.
module.exports=exports=sack;


if (process._tickDomainCallback || process._tickCallback)
    sack.Thread(process._tickDomainCallback || process._tickCallback);


