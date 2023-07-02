
# Human Interface Device Interfaces

## Keyboard

Adds ability to read keyboard input from the system.

``` js
import {sack} from "sack.vfs"
const keys = sack.Keyboard( eventCallback );

function eventCallback( event ) {
	// gets key events.
	// return true if the event is consumed.
	return false;
}
```

Keys are dispached to every `sack.Keyboard` object created.

There is a callback on the resulting keyboard object `lock(true/false)` which can lock the keyboard and block any input.

the keyboard event handler can be closed with the `close()` method on the mouse result.

## Mouse

Adds ability to read mouse input from the system.

``` js
import {sack} from "sack.vfs"
const mouse = sack.Mouse( eventCallback );

function eventCallback( event ) {
	// gets mouse events.
}
```

Mouse events are dispached to every `sack.Keyboard` object created.

Mouse Events look like `{op:<event type>, wheel:<number> buttons:<number>, x:<number>, y:<number>}`.  `op` is 
the type of mouse event forwarded from the hook.  `wheel` is the delta on the mouse wheel.  `buttons` is a composite
of mouse button flags.  

| button | value |
|----|-----|
|left| 1 |
|right| 2 |
|middle | 0x10 | 
|x1| 0x20 |
|x2| 0x40 |

x1 and x2 are buttons 4 and 5.   The missing bits are usually MK_SHIFT(0x4), MK_CONTROL(0x8).

There is an accessor on `sack.Mouse` that can set/get the cursor position.

``` js
const curPos = sack.Mouse.cursor;  // get the mouse

sack.Mouse.cursor = {x:123, y:456}; // set the cursor position
```

the mouse event handler can be closed with the `close()` method on the mouse result.

