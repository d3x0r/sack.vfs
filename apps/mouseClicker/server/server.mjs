import {openServer} from "../../http-ws/server.mjs"
import {sack} from "sack.vfs";

sack.Systray.set( "mouseClicker.ico", ()=>{
	console.log( "Launch browser(double click)" );
        sack.Task( {
        	bin:"http://localhost:8123/index.html"
        } );
} );


const server = openServer( {port:8000, resourcePath:"../ui", npmPath:"../../.."} ); 

//const app = uExpress();
server.addHandler( extraHandler );

function extraHandler( req, res ) {
	if( req.url.startsWith( "/http-ws" ) ) {
	console.log( "Handle:", req );
		req.url = "/../.." + req.url;
	}
	if( req.url.startsWith( "/events" ) ) {
	console.log( "Handle:", req );
		req.url = "/../.." + req.url;
	}
	
}

//app.get( /.*\.jsox/, (req,res)=>{
