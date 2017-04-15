"use strict";

var vfs;
try{
  vfs = require( "./build/Debug/sack_vfs.node" );
} catch(err) { try { /*console.log( err ); */vfs = require( "./build/Release/sack_vfs.node" ); } catch( err ){  console.log( err )} }
//console.log( "vfs:", vfs );

process.on( 'beforeExit', ()=>vfs.Thread() );
const thread = vfs.Thread(process._tickDomainCallback);
module.exports =exports= vfs;

