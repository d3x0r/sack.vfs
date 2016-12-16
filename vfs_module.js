"use strict";

var vfs;
try{
  vfs = require( "./build/Debug/vfs_module.node" );
} catch(err) { try { console.log( err );vfs = require( "./build/Release/vfs_module.node" ); } catch( err ){  console.log( err )} }

exports.Sqlite = vfs.Sqlite;
exports.Volume = vfs.Volume;

const thread = vfs.Thread(process._tickDomainCallback);
exports.Δ = ()=>{thread.Δ();}
