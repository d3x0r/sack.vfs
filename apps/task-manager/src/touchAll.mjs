
import {sack} from "sack.vfs"

const displays = sack.Task.getDisplays();
//console.log( "Which to use?", displays.monitor );

function f(i) {
	if( i >= displays.monitor.length ) return;

	const monitor = displays.monitor[i];
	const x = monitor.x + Math.ceil(monitor.width/2);
	const y = monitor.y + Math.ceil(monitor.height/2);
	sack.Mouse.clickAt( x, y );	
	//sack.Mouse.event( x, y, 0 );	
	//console.log( "Clicked at?", sack.Mouse.cursor );
	setTimeout( ()=>f(i+1), 1000 );
}

f(0);