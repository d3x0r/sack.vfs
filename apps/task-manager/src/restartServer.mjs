import {sack} from "sack.vfs"

const ws = sack.WebSocket.Client( "ws://localhost:" + (process.env.PORT || 8089), "tasks" );
ws.onopen = ()=>{
	ws.send( "{op:shutdown, stop:true}" );
}
ws.onclose = (code,reason)=>{
	console.log( "Closed at:", new Date(), code, reason );
	process.exit(0);
}