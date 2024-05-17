import {sack} from "sack.vfs"

const addr = "ws://localhost:" + (process.env.PORT || 8089);
console.log( "Connect to addr:", addr );
const ws = sack.WebSocket.Client( "ws://localhost:" + (process.env.PORT || 8089), "tasks" );
ws.onopen = ()=>{
	ws.send( "{op:stopAll,close:true}" );
}
ws.onclose = ()=>{
	process.exit();
}