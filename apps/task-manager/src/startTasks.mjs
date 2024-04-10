import {sack} from "sack.vfs"

const ws = sack.WebSocket.Client( "ws://localhost:" + (process.env.PORT || 8089), "tasks" );
ws.onopen = ()=>{
	ws.send( "{op:startAll,close:true}" );
}
ws.onclose = ()=>{
	process.exit();
}