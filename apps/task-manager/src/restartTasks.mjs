import {sack} from "sack.vfs"

function stop() {
	const ws = sack.WebSocket.Client( "ws://localhost:" + (process.env.PORT || 8089), "tasks" );
	ws.onopen = ()=>{
		ws.send( "{op:stopAll, close:true}" );
	}
	ws.onclose = ()=>{
		start();
	}
}

function start() {
	const ws = sack.WebSocket.Client( "ws://localhost:" + (process.env.PORT || 8089), "tasks" );
	ws.onopen = ()=>{
		ws.send( "{op:startAll, close:true}" );
	}
	ws.onclose = ()=>{
		process.exit(0);
	}
}
