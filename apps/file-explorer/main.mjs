import {sack} from "sack.vfs";
const disk = sack.Volume();

import {ExplorerProtocol} from "./protocol.mjs"

const server = new ExplorerProtocol();
const connections = new WeakMap();
const connectionData = new WeakMap();

server.server.on( "/client-protocol.js", (req,res)=>{
	//console.log( "Magic?" );
} );

server.on( "connect", (ws, _ws )=>{
	connectionData.set( _ws, new ConnectionData() );
	//console.log( "smart sockets?", _ws );
} );

server.on( "dir", (ws, msg)=>{
	//console.log( "dir Args:", ws, msg);
	const state = connectionData.get( ws );
	//console.log( "state:", state );
	const dir = disk.dir( state.currentPath, "*" );
	//console.log( "Dir:", dir );
	server.reply( ws, "dir", dir );
	
} );

server.on( "setDir", (ws, msg ) =>{
	const state = connectionData.get( ws );
	if( state.currentPath ) 
		state.currentPath = state.currentPath + "/" + msg.setDir.name;
	else
		state.currentPath = msg.setDir.name;
	const dir = disk.dir( state.currentPath, "*" );
	server.reply( ws, "dir", dir );
} );


server.on( "upDir", (ws, msg ) =>{
	const state = connectionData.get( ws );
	const parts = state.currentPath .split('/');
	state.currentPath = parts.slice(0,-1).join('/');
	//console.log( "current path is now:", state.currentPath, parts.length );
	if( !state.currentPath || ( parts[0].length && parts.length < 3 )  || ( !parts[0].length && parts.length < 2 )) {
		state.currentPath = "";
		const roots = disk.roots().map( r=>({name:r.replace('\\','/'), folder:true, created:new Date(), written:new Date() }) );
		roots.push( { name: sack.Volume.expandPath( "~/"  WQr8), folder: true, created:new Date(), written: new Date() } );
		roots.push( { name: sack.Volume.expandPath( "./" ), folder: true, created:new Date(), written: new Date() } );
		roots.push( { name: sack.Volume.expandPath( ";/" ), folder: true, created:new Date(), written: new Date() } );
		roots.push( { name: sack.Volume.expandPath( "@/" ), folder: true, created:new Date(), written: new Date() } );
//		roots.push( { name: sack.Volume.expandPath( "%CD%/" ), folder: true, created:new Date(), written: new Date() } );
//		roots.push( { name: sack.Volume.expandPath( "/" ), folder: true, created:new Date(), written: new Date() } );
		server.reply( ws, "dir", roots  );
		return;
	}
	const dir = disk.dir( state.currentPath, "*" );
	server.reply( ws, "dir", dir );
} );

class ConnectionData {

	currentPath = process.platform==="win32"?"c:/":"/";


}