
import {sack} from "sack.vfs"
const JSOX=sack.JSOX
const addr = "ws://localhost:" + (process.env.PORT || 8089);
console.log( "Connect to addr:", addr );
const ws = sack.WebSocket.Client( "ws://localhost:" + (process.env.PORT || 8089), "tasks" );
const task = {
	name: "Temporary Task",
	bin: process.argv[2],
        args : process.argv.slice( 3 ),
        noAutoRun : false,
        temporary : true,
}
let serverTask = null;

ws.onmessage = (msg_)=>{
	const msg = JSOX.parse( msg_ );
	if( msg.op === "addTask" ) {
		ws.send( JSOX.stringify( {op:"log", id:msg.task.id} ) );
		serverTask = msg.task;
	}
	else if( msg.op === "tasks" ) {
	}
	else if( msg.op === "log" ) {
		console.log( msg.log.line );
	}
	else if( msg.op === "status" ) {
		if( msg.running === false && msg.id === serverTask.id )
			process.exit(0);
	}
	console.log( "Got Message:", msg );
}
ws.onopen = ()=>{
	ws.send( JSOX.stringify( {op:"createTask",system:null,task,close:true} ) );
}
ws.onclose = ()=>{
	process.exit();
}

import readline from "node:readline";

const rl = readline.createInterface( {input:process.stdin, output:process.stdout} );
rl.on( "line", (chunk)=>{
	console.log( "Got line:", chunk );
	ws.send( JSOX.stringify( {op:"send", id: serverTask.id, data: chunk +"\n"} ) );
} );
rl.on( "close", ()=>{
	process.exit(0) } );

process.on( "exit", ()=>{
	console.log( "Sending delete?" );
	ws.send( JSOX.stringify( {op:"deleteTask", id:serverTask.id } ) );
	ws.close( 3000, "done" );
} );