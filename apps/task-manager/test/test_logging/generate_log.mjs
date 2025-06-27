
for( var i = 0; i < 100; i++ ) {
	console.log( "L:", i );
   console.error( "E:", i );
}


function dontExit() {
	setTimeout( dontExit, 10000 );
}

dontExit();
