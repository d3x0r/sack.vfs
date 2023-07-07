# Events

This is a simple event class.  It offers `on` to define and dispatch events; and `off` to remove events.



## Methods
|Name| Arguments | Description |
|---|---|---|
|on  | (string, function [,priority]) | Register for an event notification.  The callback will receive a optional parameter passed to `on()`; results with an Event object. |
|on  | (string, data)| If the second parameter is an object, string, number (non function), then this event is triggered passing the argument to each subscription.|
|on  | (string, array)| If the second parameter is an array, then the callback is called with `.apply(array)`.|
|on  | (string)| If the second parameter is undefined then the result is true if at least one event handler has been registered.|
|off | (string,function) | Remove a subscription by name and function |

## Events Accessors

|Name| Arguments | Description |
|---|---|---|
|log | true | enable debug logging, once enabled, cannot be disabled |
|enablePriority | bool | The default behavior is to use an extra argument after function; this can be used to disable or re-enable this feature. |

## Events Accessors

|Name| Arguments | Description |
|---|---|---|
|enableArrayArgs | bool | The default behavior is to pass data that is an array as an argumnet list to the callback; this can be used to disable or re-enable this feature.


`on()` events which emit events (with a second data parameter), the result is an array of the result of all of the callbacks registered.

If the second parameter to `on(string,data)` is an array, and the option `eventObject.enableCallbackApply=true` has been enabled, then
the callback is called with the arguments spread; as in `(...data)`.

The callback is called with 1 extra paraemter, `cb(data, result_array )` which is the array of previous results. allowing event handlers to check the results of other
callbacks.  If the data passed was an array, and `enableArrayArgs` is true, then the arguments are still passed at the end `cb(...data,arr)`.

The `priority` argument after an event notification handler is specified, higher numbers are run earlier.  Priority 0 is the default.  The last
default handler registered at a prioirty level is the last to run.  Negative priorities may be used to run very last.

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
