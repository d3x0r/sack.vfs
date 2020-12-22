"use strict";
const util = require('util');

var sack, errN = [];
try {
	sack = require('sack' );
} catch( err ) {
	errN.push(err);
}

if( !sack ){
  if( process.platform === "win32" ) {
    try {
      if( process.config.target_defaults.default_configuration === 'Debug' )
        sack = require( "./build/Debug/sack_gui.node" );
    } catch( err ){
      errN.push(err);
    }	

    if( process.config.target_defaults.default_configuration === 'Release' ) {
      try {
          sack = require( "./build/RelWithDebInfo/sack_gui.node" );
      } catch( err ){
        errN.push(err);
      }
      try {
          sack = require( "./build/Release/sack_gui.node" );
      } catch( err ){
        errN.push(err);
      }
    }
  }
} else {
  if( !sack )
    try {
        sack = require( "./build/RelWithDebInfo/sack_gui.node" );
    } catch( err ){
      errN.push(err);
    }
  if( !sack )
    try {
      sack = require( "./build/Debug/sack_gui.node" );
    } catch( err ){
      errN.push(err);
    }
  if( !sack )
    try {
        sack = require( "./build/Release/sack_gui.node" );
    } catch( err ){
      errN.push(err);
    }
}

if( !sack )
  throw new Error( util.format( "Failed to match configuration:", process.config.target_defaults.default_configuration, "\n", errN.join(',') ) );

require( "./sack-jsox.cjs" )(sack);
require( "./object-storage.cjs" )(sack);

module.exports=exports=sack;

// needed to dispatch promise resolutions that have been created in callbacks.
if (process._tickDomainCallback || process._tickCallback)
    sack.Thread(process._tickDomainCallback || process._tickCallback);


