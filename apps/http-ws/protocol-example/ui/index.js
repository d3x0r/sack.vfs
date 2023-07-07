
import {Popup,popups} from "/node_modules/@d3x0r/popups/popups.mjs"
import {protocol} from "./protocol.js"

protocol.on( "open", ()=>{	
	protocol.load();
} )

protocol.on( "close", (code, reason)=>{
	
	setTimeout( protocol.connect, 5000 );
} )

protocol.on( "dir", (config)=>{
	console.log( "config?", config );
} );
