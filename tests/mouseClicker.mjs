import {sack} from "sack.vfs"

let _4down = false;
let clicking = false;
const clickPerSec = 60;
const mouse = sack.Mouse( mouseCallback );

function mouseCallback(event){
	//console.log( "Constructor event:", event );
        if( event.buttons & 32 ) {
		if( !_4down ) {                	
                	_4down = true;
	        	clicking = !clicking;
        	        if( clicking )
	        	        doClick();
                } else {
                }
        } else {
        	_4down = false;
        }
}

function doClick( ) {
	const pos = sack.Mouse.cursor;
        //console.log( "POS:", pos );
	sack.Mouse.event( pos.x, pos.y, sack.Mouse.buttons.left );
	sack.Mouse.event( pos.x, pos.y, 0 );        
        if( clicking )
	        setTimeout( doClick, 1000 / clickPerSec );
}