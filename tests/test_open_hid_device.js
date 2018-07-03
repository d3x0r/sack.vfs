

//HID\VID_06A3&PID_8000&REV_0120&MI_00 7&1ffb55b8&0&0000 {4d36e96b-e325-11ce-bfc1-08002be10318}

var sack = require( ".." );
var keyboard = sack.Keyboard();

var keyboards = [];

keyboard.onKey( (event)=>{
	var keybuf;
	if( !( keybuf = keyboards[event.id] ) ) {
		keybuf = keyboards[event.id] = makeKeyBuffer();
	}	
	if( event.char === '\r' )
		keybuf.state = 0;

	switch( keybuf.state ) {
	case 0:
		if( event.char === 'P' ) keybuf.state++;
		break;
	case 1:
	case 2:
		if( event.char >= '0' && event.char <= '9' ) keybuf.state++;
		break;
	case 3:
		if( event.char === 'T' ) keybuf.state++;
		break;
	case 4:
		if( event.char >= '0' && event.char <= '9' ) {
			keybuf.num = Number(event.char);
			keybuf.state++;
		}
		break;
	case 5:
		if( event.char >= '0' && event.char <= '9' ) {
			keybuf.num *= 10;
			keybuf.num += Number(event.char);
			
			keybuf.state++;
		}
		break;
	}
	console.log( "Event:", event );
} );

function makeKeyBuffer() {
	return {
		keybuf : [],
		state : 0,
		num : 0,	
	}
}

