"use strict";
const util = require('util');

var sack, errN = [];
try {
	//sack = require('sack' );
	//console.log( "Got sack loaded?" );
} catch( err ) {
	errN.push(err);
}

if( !sack )
  if( process.platform === 'browser' ) { // electron
      try {
          sack = require( "./build/Release/sack_vfs.node" );
			console.log( "Release loaded Yes.", sack );
      } catch( err ){
        errN.push(err);
      }
		if( !sack )
          try {
              sack = require( "./build/Debug/sack_vfs.node" );
				console.log( "Debug loaded Yes.", sack );
          } catch( err ){
            errN.push(err);
          }
		if( !sack )
          try {
              sack = require( "./build/RelWithDebInfo/sack_vfs.node" );
          } catch( err ){
            errN.push(err);
          }
  } else if( process.platform === 'win32' ) {
    try {
      if( process.config.target_defaults.default_configuration === 'Debug' 
		|| ( Number(process.version.split('.')[0].split('v')[1]) >= 16 ) 
		|| ( Number(process.version.split('.')[0].split('v')[1]) < 12 ) )
        sack = require( "./build/Debug/sack_vfs.node" );
    } catch( err ){
      errN.push(err);
    }	

    if( process.config.target_defaults.default_configuration === 'Release' ) {
      try {
          sack = require( "./build/RelWithDebInfo/sack_vfs.node" );
      } catch( err ){
        errN.push(err);
      }
		if( !sack )
      try {
          sack = require( "./build/Release/sack_vfs.node" );
      } catch( err ){
        errN.push(err);
      }
    }
  } else {
    if( !sack )
      try {
          sack = require( "./build/RelWithDebInfo/sack_vfs.node" );
      } catch( err ){
        errN.push(err);
      }
    if( !sack )
      try {
        sack = require( "./build/Debug/sack_vfs.node" );
      } catch( err ){
        errN.push(err);
      }
    if( !sack )
      try {
          sack = require( "./build/Release/sack_vfs.node" );
      } catch( err ){
        errN.push(err);
      }
  }
if( !sack )
  throw new Error( util.format( "Failed to match configuration:", process && process.config && process.config.target_defaults && process.config.target_defaults.default_configuration, "\n", errN.join(',') ) );

require( "./sack-jsox.cjs" )(sack);
require( "./object-storage.cjs" )(sack);
require( "./object-storage-cb.cjs" )(sack);

module.exports=exports=sack;


// needed to dispatch promise resolutions that have been created in callbacks.
if (process._tickDomainCallback || process._tickCallback)
    sack.Thread(process._tickDomainCallback || process._tickCallback);


