
import {sack} from "sack.vfs";

const mouse = sack.Mouse( mouseEvent );
const keyboard = sack.Keyboard( keyEvents );
const timeout = 5000;
let now = Date.now();
let timer = 0;
let hidden = false;
let oldMouse = null;
const displays = sack.Task.getDisplays();
let maxX = -Infinity;
let midPrimary = {x:0, y:0};

for( let monitor of displays.monitor ) {
	for( let device of displays.device ) {
		if( device.primary && device.display === monitor.display ) {
			midPrimary.x = monitor.x + monitor.width/2;
			midPrimary.y = monitor.y + monitor.height/2;
			break;
		}
	}
    if( (monitor.width+monitor.x) > maxX ){
    	maxX = (monitor.width+monitor.x);
    }
}

//console.log( "width: ", maxX );
function hideMouse() {
	oldMouse = sack.Mouse.cursor;
	sack.Mouse.cursor = {x:maxX, y:0};
}


function tick() {
	const newNow = Date.now();
	if( (newNow-now) >= timeout ) {
		hidden = true;
		hideMouse();
		timer = 0;
	} else {
		timer = setTimeout( tick, timeout - (newNow-now) );
	}

}
	timer = setTimeout( tick, timeout );

function mouseEvent( event ) {
	now = Date.now();
	if( timer ) clearTimeout( timer );
	timer = setTimeout( tick, timeout );
	if( hidden ) {
		const newMouse = sack.Mouse.cursor;
		oldMouse.x += newMouse.x - maxX;
		oldMouse.y += newMouse.y - 0;
		sack.Mouse.cursor = oldMouse;
		hidden = false;
	}
   //console.log( "Got Mouse Event 1:", event );
}

export async function go() {
	console.log( "Hide Mouse Enabled: delay=", timeout );
}

const control_keys = {
	alt:false,
	shift:false,
	control:false 
};

let usedMouse = false;

function keyEvents( event ) {
	if( event.down ) {
		if( event.scan == 56 ) control_keys.alt=true;
		if( event.scan == 42 ) control_keys.shift=true;
		if( event.scan == 54 ) control_keys.shift=true;
		if( event.scan == 29 ) control_keys.control = true;
	} else {
		if( event.scan == 56 ) control_keys.alt=false;
		if( event.scan == 42 ) control_keys.shift=false;
		if( event.scan == 54 ) control_keys.shift=false;
		if( event.scan == 29 ) control_keys.control = false;
	}
	//console.log( "Keys:", control_keys, event );
	//if( event.down && !control_keys.alt && control_keys.shift && control_keys.control && event.char === 'M' ) {
	//}
		// 'M' ctrl-shift-M triggers account menu or something in chrome
/*
	if( event.down && !control_keys.alt && control_keys.shift && control_keys.control && event.char === 'Z' ) {
		//console.log( "Key Event Triggered to find mouse." );
		if( usedMouse ) {
			return true;
		}
		sack.Mouse.cursor = midPrimary;
		oldMouse = sack.Mouse.cursor;
		now = Date.now();
		hidden = false;
		if( timer ) clearTimeout( timer );
		timer = setTimeout( tick, timeout );
		usedMouse = true;
		return true;
	}
	if( !event.down && !control_keys.alt && control_keys.shift && control_keys.control && event.char === 'Z' ) {
		//console.log( "Key Event Triggered to find mouse." );
		if( usedMouse ) {
			usedMouse = false;
			return true;
		}
	}
*/

	if( event.down && !control_keys.alt && control_keys.shift && control_keys.control && event.char === 'M' ) {
		//console.log( "Key Event Triggered to find mouse." );
		if( usedMouse ) {
			return true;
		}
		sack.Mouse.cursor = midPrimary;
		oldMouse = sack.Mouse.cursor;
		now = Date.now();
		hidden = false;
		if( timer ) clearTimeout( timer );
		timer = setTimeout( tick, timeout );
		usedMouse = true;
		return true;
	}
	if( !event.down && !control_keys.alt && control_keys.shift && control_keys.control && event.char === 'M' ) {
		//console.log( "Key Event Triggered to find mouse." );
		if( usedMouse ) {
			usedMouse = false;
			return true;
		}
	}

	return false;
	//console.log( "Got Key Event 1:", event, control_keys );
}
