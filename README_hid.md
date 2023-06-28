
# Human Interface Device Interfaces

## Keyboard

Adds ability to read keyboard input from the system.

``` js
import {sack} from "sack.vfs"
const keys = sack.Keyboard( eventCallback );

function eventCallback( event ) {
	// gets key events.
}
```

There is a callback on the resulting keyboard object `lock(true/false)` which can lock the keyboard and block any input.

the keyboard event handler can be closed with the `close()` method on the mouse result.

## Mouse

Adds ability to read mouse input from the system.

``` js
import {sack} from "sack.vfs"
const mouse = sack.Mouse( eventCallback );

function eventCallback( event ) {
	// gets key events.
}
```

There is an accessor on `sack.Mouse` that can set/get the cursor position.

``` js
const curPos = sack.Mouse.cursor;  // get the mouse

sack.Mouse.cursor = {x:123, y:456}; // set the cursor position
```

the mouse event handler can be closed with the `close()` method on the mouse result.
