
var sack = require( "../.." );//sack-gui" );

sack.Systray.set( "pdown.ico", ()=>{
	console.log( "Launch browser" );
        sack.Task( {
        	bin:"http://localhost:8123/index.html"
        } );
} );

