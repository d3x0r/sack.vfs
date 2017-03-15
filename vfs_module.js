"use strict";

var vfs;
try{
  vfs = require( "./build/Debug/sack_vfs.node" );
} catch(err) { try { /*console.log( err ); */vfs = require( "./build/Release/sack_vfs.node" ); } catch( err ){  console.log( err )} }
process.on( 'beforeExit', ()=>vfs.Thread() );
//process.on( 'exit', ()=>vfs.Thread() );
const thread = vfs.Thread(process._tickDomainCallback);
module.exports = vfs;
exports.Sqlite = vfs.Sqlite;
exports.Volume = vfs.Volume;

exports.Δ = vfs.Δ;
exports.Λ = vfs.Λ;
