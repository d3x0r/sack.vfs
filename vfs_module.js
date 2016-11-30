"use strict";

var vfs;
try{
  vfs = require( "./build/Debug/vfs_module.node" );
} catch(err) { try { vfs = require( "./build/Release/vfs_module.node" ); } catch( err ){  } }

exports.Sqlite = vfs.Sqlite;
exports.Volume = vfs.Volume;
