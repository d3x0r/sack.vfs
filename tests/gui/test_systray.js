
var sack = require( "../.." );

function init() {
sack.Systray.set( "pdown.ico", ()=>{
	console.log( "double 1 click event!" );
	sack.Systray.set( "pri.ico", ()=>{
		console.log( "double 2 click event!" );
		init();
	} );
} );
}
init();
sack.Systray.on( "New Menu Option", ( )=>{
	console.log( "Custom option" );
} );
