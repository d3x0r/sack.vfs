
import {sack} from "sack.vfs"

const oldCursor = sack.Mouse.cursor;
console.log( "Cursor at:", oldCursor )

sack.Mouse.cursor = {x:100, y:100 };
console.log( "Cursor now at:", sack.Mouse.cursor )

const displays = sack.Task.getDisplays();

let maxX = -Infinity;
for( let monitor of displays.monitor ) {
    if( (monitor.width+monitor.x) > maxX ){
    	maxX = (monitor.width+monitor.x);
    }
}

console.log( "width: ", maxX );
sack.Mouse.cursor = {x:maxX, y:0};
//    setTimeout( ()=>{ sack.Mouse.cursor = oldCursor }, 500 );
