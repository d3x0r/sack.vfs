"use strict";

var sack;
try{
  sack = require( "./build/RelWithDebInfo/sacK_gui.node" );
} catch(err1) {
  try {
    //console.log( err1 );
    sack = require( "./build/Debug/sacK_gui.node" );
  } catch( err2 ){
    try {
      //console.log( err2 );
      sack = require( "./build/Release/sacK_gui.node" );
    } catch( err3 ){
      console.log( err1 )
      console.log( err2 )
      console.log( err3 )
    }
  }
}

require( "./sack-jsox.js" )(sack);
require( "./object-storage.js" )(sack);
//vfs.
module.exports=exports=sack;


if (process._tickDomainCallback || process._tickCallback)
    sack.Thread(process._tickDomainCallback || process._tickCallback);


