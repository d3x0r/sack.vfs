"use strict";

var vfs;
try{
  vfs = require( "./build/Debug/sack_vfs.node" );
} catch(err) { try { /*console.log( err ); */vfs = require( "./build/Release/sack_vfs.node" ); } catch( err ){  console.log( err )} }
console.log( "vfs:", vfs );
vfs.InitFS();
require( "./Δ" );
process.on( 'beforeExit', ()=>vfs.Thread() );
//process.on( 'exit', ()=>vfs.Thread() );
const thread = vfs.Thread(process._tickDomainCallback);
module.exports =exports= vfs;
exports.Sqlite = vfs.Sqlite;
exports.Volume = vfs.Volume;

exports.Δ = vfs.Δ;
exports.Λ = vfs.Λ;
