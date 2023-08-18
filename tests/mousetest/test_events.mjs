import {sack} from "sack.vfs"

try {
	sack.Mouse.event( 10, 10 );
        console.log( "This should have thrown an error, and is an error if you see this." );
} catch( err) {
	console.log( "Expected Error:", err );
}

function g( i ) {
	if( i > 1000 ) return;
	if( !i )
	        sack.Mouse.event( 1, 1, 0 );
	else
	        sack.Mouse.event( 10, 10, sack.Mouse.buttons.relative );
	setTimeout( ()=>g(i+10), 50 );
}

function f( i ) {
	if( i > 1000 ) return g(0);
        sack.Mouse.event( i, i, 0 );
	setTimeout( ()=>f(i+10), 50 );
}

f( 0 );

