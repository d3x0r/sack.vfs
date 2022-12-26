# Events
This is a simple event class.

## Methods
|Name| Arguments | Description |
|---|---|---|---|
|on | (string, function) | Register for an event notification.  The callback will receive a optional parameter passed to `on()` |
| on | (string, data)| If the second parameter is an object, string, number (non function), then this event is triggered passing the argument to each subscription|
|off | (string,function) | Remove a subscription by name and function |

## Example

``` js
import {Events} from "sack.vfs/Events"

const events = new Events();

events.on( "tick", tick );

function tick( arg ) {
   console.log( "Tick?", arg );
   events.off( "tick", tick ); // maybe make it behave like 'once' ?
}

events.on( "tick", 0 );
```
