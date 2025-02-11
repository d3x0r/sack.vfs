


if( "undefined" === typeof process ) 
	var process = {
			platform : "ivm",
		}

var sack, errN = [];
try {
	//sack = require('sack' );
	//console.log( "Got sack loaded?" );
} catch( err ) {
	errN.push(err);
}

let basePath = ( process.platform === "win32" )?process.env.PATH:null;
let isGUI = false;
function includeNative( gui ) {
isGUI = gui;
const name = gui?"sack_gui.node":"sack_vfs.node";
if( !sack )
  if( process.platform === 'browser' ) { // electron
      try {
			if( gui && basePath )
				process.env.PATH = basePath
					+";"+__dirname+"\\build\\Release\\bin"
					+";"+__dirname+"\\build\\Release\\share\\SACK\\plugins"
          sack = require( "./build/Release/" + name );
			console.log( "Release loaded Yes.", sack );
      } catch( err ){
        errN.push(err);
      }
		if( !sack )
          try {
				if( gui && basePath )
					process.env.PATH = basePath
						+";"+__dirname+"\\build\\Debug\\bin"
						+";"+__dirname+"\\build\\Debug\\share\\SACK\\plugins"
              sack = require( "./build/Debug/" + name );
				console.log( "Debug loaded Yes.", sack );
          } catch( err ){
            errN.push(err);
          }
		if( !sack )
			try {
				if( gui && basePath )
					process.env.PATH = basePath
						+";"+__dirname+"\\build\\RelWithDebInfo\\bin"
						+";"+__dirname+"\\build\\RelWithDebInfo\\share\\SACK\\plugins"
				sack = require( "./build/RelWithDebInfo/" + name );
			} catch( err ){
            errN.push(err);
          }
  } else if( process.platform === 'win32' ) {
    try {
      if( process.config.target_defaults.default_configuration === 'Debug' 
		   || ( Number(process.version.split('.')[0].split('v')[1]) >= 16 ) 
		   || ( Number(process.version.split('.')[0].split('v')[1]) < 12 ) ) {
			if( gui && basePath )
				process.env.PATH = basePath
					+";"+__dirname+"\\build\\Debug\\bin"
					+";"+__dirname+"\\build\\Debug\\share\\SACK\\plugins"
			sack = require( "./build/Debug/" + name );
		}
    } catch( err ){
      errN.push(err);
    }	

	if( process.config.target_defaults.default_configuration === 'Release' ) {
		try {
			if( gui && basePath )
				process.env.PATH = basePath
					+";"+__dirname+"\\build\\RelWithDebInfo\\bin"
					+";"+__dirname+"\\build\\RelWithDebInfo\\share\\SACK\\plugins"
          sack = require( "./build/RelWithDebInfo/" + name );
      } catch( err ){
        errN.push(err);
      }
		if( !sack )
      try {
			if( gui && basePath )
				process.env.PATH = basePath
					+";"+__dirname+"\\build\\Release\\bin"
					+";"+__dirname+"\\build\\Release\\share\\SACK\\plugins"
          sack = require( "./build/Release/" + name );
      } catch( err ){
        errN.push(err);
      }
    }
  } else {
    if( !sack )
      try {
			if( gui && basePath )
				process.env.PATH = basePath
					+";"+__dirname+"\\build\\RelWithDebInfo\\bin"
					+";"+__dirname+"\\build\\RelWithDebInfo\\share\\SACK\\plugins"
          sack = require( "./build/RelWithDebInfo/" + name );
      } catch( err ){
        errN.push(err);
      }
    if( !sack )
      try {
			if( gui && basePath )
				process.env.PATH = basePath
					+";"+__dirname+"\\build\\Debug\\bin"
					+";"+__dirname+"\\build\\Debug\\share\\SACK\\plugins"
        sack = require( "./build/Debug/" + name );
      } catch( err ){
        errN.push(err);
      }
    if( !sack )
      try {
			if( gui && basePath )
				process.env.PATH = basePath
					+";"+__dirname+"\\build\\Release\\bin"
					+";"+__dirname+"\\build\\Release\\share\\SACK\\plugins"
			sack = require( "./build/Release/" + name );
      } catch( err ){
        errN.push(err);
      }
  }
	if( !sack && !gui ) includeNative( true );
}
includeNative( false );
if( !sack ) {
	const util = require('util');
	throw new Error( util.format( "Failed to match configuration:", process && process.config && process.config.target_defaults && process.config.target_defaults.default_configuration, "\n", errN.join(',') ) );
}

export default sack;
export {sack};

import jsoxHook from  "./sack-jsox.mjs";
jsoxHook(sack);
import objStoreHook from "./object-storage.mjs" 
objStoreHook(sack);
import objStoreCbHook from "./object-storage-cb.mjs"
objStoreCbHook( sack );
//if( process.platform === "win32" )
//	require( "./service-object.cjs" )(sack);


/*
process.on('exit', ()=>{
	console.log( "exit in sack.vfs addon" );
} );
process.on('beforeExit', ()=>{
	console.log( "beforeExit in sack.vfs addon" );
} );
*/

// this allowed ctrl-C to be a graceful exit...
// not sure why it wasn't without this though... 
if(isGUI)
process.on('SIGINT', ()=>{
	//console.log( "sigint (early?) in sack.vfs addon" );
	sack.sigint();
	//process.emit( "exit" );
	//process.exit(0);
} )

// needed to dispatch promise resolutions that have been created in callbacks.
if (process._tickDomainCallback || process._tickCallback)
    sack.Thread(process._tickDomainCallback || process._tickCallback);


