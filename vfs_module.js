"use strict";

var vfs;
try{
  vfs = require( "./build/Debug/vfs_module.node" );
} catch(err) { try { /*console.log( err ); */vfs = require( "./build/Release/vfs_module.node" ); } catch( err ){  console.log( err )} }

const thread = vfs.Thread(process._tickDomainCallback);
module.exports = vfs;
exports.Sqlite = vfs.Sqlite;
exports.Volume = vfs.Volume;

exports.Δ = vfs.Δ;
exports.Λ = vfs.Λ;
