
var sack = require( "../.." );//sack-gui" );

sack.Systray.set( "sideplayR.ico", ()=>{
	console.log( "Launch browser(double click)" );
        sack.Task( {
        	bin:"http://localhost:8123/index.html"
        } );
} );

