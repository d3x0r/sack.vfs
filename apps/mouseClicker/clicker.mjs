
import {sack} from "sack.vfs"

const keys = sack.Keyboard( keyEvents );

const control_keys = {
	alt:false,
	shift:false,
	control:false 
};

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
	//if( event.down && control_keys.shift && control_keys.control && event.char === 'R' )
	//	console.log( "REBOOT?" );
   console.log( "Got Key Event 1:", event, control_keys );
}

let _4down = false;
let clicking = false;
const clickPerSec = 120;
const mouse = sack.Mouse( mouseCallback );
let clickPos = null;

function mouseCallback(event){
	//console.log( "mouse event:", event );
	if( event.buttons & 32 ) {
		if( !_4down ) { 
			clickPos = sack.Mouse.cursor;               	
			_4down = true;
			clicking = !clicking;
			console.log( "click?", clicking );
			if( clicking )
				doClick();
		} else {
		}
	} else {
		_4down = false;
	}
}

function doClick( ) {
	if( !clickPos ) return;
	const oldPos = sack.Mouse.cursor;
	const pos = clickPos;//sack.Mouse.cursor;
        //console.log( "POS:", pos, oldPos );
	sack.Mouse.event( [{ x:pos.x, y:pos.y, buttons:sack.Mouse.buttons.left }
				,{ x:pos.x, y:pos.y, buttons:0 }
			//	,{ x:oldPos.x, y:oldPos.y, buttons:0} 
			] );        
	sack.Mouse.event( oldPos.x, oldPos.y, 0 );        
        if( clicking )
	        setTimeout( doClick, 1000 / clickPerSec );
}
