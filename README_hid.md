
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

There is a function on the resulting keyboard object `lock(true/false)` 
which can lock the keyboard and block any input.

the keyboard object can also use `send( key, code, isDown, isExtended)` or 
`send( [{key:,code:,down:false,extended:false}[, ...] )` to send key events.
down is a boolean indicating if the key is being pressed or released.  extended
is used to generate an extended keycode on windows.  (grey keys and some extra 
function keys use this).



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
of mouse button flags.  These definitions are stored on the `Mouse` class as a `button` object; for example `Mouse.buttons.left === 1`.

| button | value |
|----|-----|
|left| 1 |
|right| 2 |
|middle | 0x10 | 
|x1| 0x20 |
|x2| 0x40 |
|relative| 0x1000000 |

x1 and x2 are buttons 4 and 5.   The missing bits are usually MK_SHIFT(0x4), MK_CONTROL(0x8).

The flag `relative` is not a mouse button flag, but controls whether the `x, y` position specified for a `event()` (below) is a relative or absolute position.  Default is to move to the absolute position specified.

There is an accessor on `sack.Mouse` that can set/get the cursor position.

``` js
const curPos = sack.Mouse.cursor;  // get the mouse

sack.Mouse.cursor = {x:123, y:456}; // set the cursor position
```

the mouse event handler can be closed with the `close()` method on the mouse result.

### Mouse System Utility Methods

The `Mouse()` class also has some utility functions.

|method|args|description|
|---|---|---|
|clickAt| (x,y) | Generates a left button click at position x,y. |
|event| (x,y,b,s1,s2) | Generates events for the information supplied.  `x` and `y` set the position of the mouse; `b` sets the state of the buttons, the value of this are the `buttons` flags mentioned above; `s1` generates up-down scroll wheel events, and is a number, a negative number goes up; `s2` generates right-left scroll events, and is a floating point number, a negative number goes left(?).  Default scroll size is `1`, but may be a fraction of a scroll tick |

In order to generate a full click, two `event()` calls are needed, one to click the button and another to release any buttons.
Mouse position is only sent when the current system cursor position is different from the position specified.


