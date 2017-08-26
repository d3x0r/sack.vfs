"use strict";

var vfs;
try{
  vfs = require( "./build/Debug/sack_vfs.node" );
} catch(err) { try { /*console.log( err ); */vfs = require( "./build/Release/sack_vfs.node" ); } catch( err ){  console.log( err )} }

vfs.JSON6.stringify = JSON.stringify;
vfs.JSON.stringify = JSON.stringify;

process.on( 'beforeExit', ()=> {
	vfs.Thread() 
});
process.on( 'exit', ()=> {
});
process.on( 'uncaughtException', (err)=> {
	console.log( err )	
});
const thread = vfs.Thread(process._tickDomainCallback);
module.exports =exports= vfs;

