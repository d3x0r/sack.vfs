import {Events} from "./events.mjs"

class MyEvent extends Events {
}

class MyEvent2 extends Events {
}

MyEvent.log= true;
Events.on( "event", (data)=>console.log( "event R:", data ) );
MyEvent.on( "event", (data)=>console.log( "event 1:", data ) );
MyEvent2.on( "event", (data)=>console.log( "event 2:", data ) );

MyEvent.on( "event", "apple" );
MyEvent2.on( "event", "orange" );
