import {sack} from "sack.vfs"

const ws = sack.WebSocket.Client( "ws://localhost:" + (process.env.PORT || 8089), "tasks" );
ws.onopen = ()=>{
	console.log( "opened, send shutdown command..." );
	ws.send( "{op:shutdown,stop:true}" );
}
ws.onclose = ()=>{
	console.log( "Socket closed, exit." );
	process.exit();
}
