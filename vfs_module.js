"use strict";

var vfs;
try{
vfs = require( "./build/Debug/vfs_module.node" );
} catch(err) { try { vfs = require( "./build/Release/vfs_module.node" ); } catch( err ){  } }

var events = require( 'events');

console.log( "got", vfs );
//console.log( Object.keys( events.EventEmitter.prototype ) );

exports.Sqlite = (...args)=>vfs.Sqlite( ...args );

exports.Volume = ( ...args ) => {
	//console.log( "arguments :", args );
	 let vol = {
        	volume : vfs.Volume( ...args )
                //, Sqlite : (...args)=>vol.volume.Sqlite( ...args )
        	, File: ( name )=>{
                  	let file = vol.volume.File( name );
	       	        Object.assign( file, events.EventEmitter.prototype );
                        //console.log( "File module result: ", Object.getPrototypeOf( file ) );
                         return file;
                        ;
                }
                , 
        }
        return vol;
}

