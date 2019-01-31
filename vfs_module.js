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

require( "./sack-jsox.js" )(sack);
require( "./object-storage.js" )(sack);
//vfs.
module.exports=exports=sack;

