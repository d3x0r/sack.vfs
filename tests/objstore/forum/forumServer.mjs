import {sack} from "sack.vfs";
import {ObjectStorage} from "sack.vfs/object-storage";
import {openServer} from "sack.vfs/http-ws";
import {ForumDatabase} from "./forumDb.mjs";
import {ForumProtocol} from "./forumProtocol.mjs";

const moduleFile = decodeURIComponent( import.meta.url.replace(/^file:\/\/\//, "") );
const forumPath = moduleFile.slice( 0, moduleFile.lastIndexOf("/") );
const packagePath = forumPath.slice( 0, forumPath.lastIndexOf("/tests/objstore/forum") );
process.chdir( forumPath );

const storage = new ObjectStorage( process.env.FORUM_STORAGE || "forum.os" );
const database = await new ForumDatabase().hook( storage );
const allowGuests = process.env.FORUM_ALLOW_GUESTS === "1";
let consumeToken = ()=>null;

const protocol = new ForumProtocol( database, {
	allowGuests,
	consumeToken:key=>consumeToken(key),
} );
const port = Number(process.env.PORT) || 8082;
const server = openServer( {
	port,
	resourcePath:forumPath + "/ui",
	npmPath:[forumPath, packagePath],
	allowModules:["katex"],
}, function accept(ws) {
	if( protocol.accept(ws) ) this.accept();
	else this.reject();
}, ws=>protocol.connect(ws) );

if( process.env.FORUM_NO_AUTH !== "1" ) {
	const disk = sack.Volume();
	if( !disk.isDir("fs") ) disk.mkdir("fs");
	const login = await import( "@d3x0r/user-database-remote/enableLogin.mjs" );
	consumeToken = login.getUser;
	login.enableLogin( server, server.app );
} else if( !allowGuests ) {
	console.warn( "FORUM_NO_AUTH is set without FORUM_ALLOW_GUESTS=1; clients cannot connect." );
}

console.log( `Sack Forum listening on http://localhost:${port}/` );
console.log( `storage: ${process.env.FORUM_STORAGE || forumPath + "/forum.os"}` );
console.log( allowGuests ? "local guest access enabled" : "user-database login required" );

process.on( "SIGINT", ()=>{
	storage.flush();
	sack.SaltyRNG.setSigningThreads( 0 );
	process.exit(0);
} );
