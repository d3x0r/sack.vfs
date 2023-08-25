import {sack} from "sack.vfs"


        sack.Mouse.event( 960, 540, 0 );
	console.log( "Mouse Moved  to:", sack.Mouse.cursor );

/*
try {
	sack.Mouse.event( 10, 10 );
        console.log( "This should have thrown an error, and is an error if you see this." );
} catch( err) {
	console.log( "Expected Error:", err );
}
*/
function g( i ) {
	if( i > 1000 ) return;
	if( !i )
	        sack.Mouse.event( 1, 1, 0 );
	else {
	        sack.Mouse.event( 10, 10, sack.Mouse.buttons.relative );
	}
	setTimeout( ()=>g(i+10), 50 );
}

function f( i ) {
	if( i > 1000 ) return ;//g(0); 
	console.log( "Mouse starts at:", sack.Mouse.cursor );
        sack.Mouse.event( i*4, i*2, 0 );
	console.log( "Mouse Moved  to:", sack.Mouse.cursor );
	setTimeout( ()=>f(i+10), 50 );
}

f( 0 );

