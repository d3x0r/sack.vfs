

const myPath = import.meta.url.split(/\/|\\/g);
const tmpPath = myPath.slice();
tmpPath.splice( 0, 3 );
tmpPath.splice( tmpPath.length-1, 1 );
const programRoot = (process.platform==="win32"?"":'/')+tmpPath.join( '/' );

//const appPath = process.argv[1].split(/\/|\\/g);
//console.log( "myRoot:", programRoot );

const local = {
	tasks : [],
	taskMap : {},
	connections: [],
}

import {sack} from "sack.vfs"

const JSOX = sack.JSOX;

//import config from "../config.jsox"

import {openServer} from "sack.vfs/apps/http-ws"
import {Task} from "./task.mjs"

export let config ;
const disk = sack.Volume();
disk.readJSOX( "config.tasks.jsox", (c)=>{
	config=c ;
	config.tasks.forEach( (task)=>{
		const newTask = Task.load( task );
		local.tasks.push( newTask );
		local.taskMap[newTask.id] = newTask;
	} );
	openServer( {resourcePath:programRoot+"/../ui", npmPath:config.npmPath, port:Number(process.env.PORT) || config.port || 8080}, accept, connect );
	local.tasks.forEach( task=>task.start() );
});

if( !config ) throw new Error( "Configuration has an error, or doesn't exist" );

export function send( msg_ ) {
	const msg = JSOX.stringify( msg_ );
	local.connections.forEach( ws=>ws.send( msg ) );
}

function accept( ws ) {
	//console.log( "Accept socket? Why am I stalling? shared ports?" );
	this.accept();
}

function connect( ws ) {
	ws.onmessage = handleMessage;
	ws.onclose = handleClose;
	local.connections.push( ws );

	sendTasks();

	function handleMessage( msg_ ) {
		const msg = JSOX.parse( msg_ );
		switch( msg.op ) {
		case "task_logging":
			for( let task of local.tasks ) {
				if( task.id === msg.id ) {
					task.ws = ws;
					break;
				}
			}
			break;
		case "start":
			{ 
				const task = local.taskMap[msg.id];
				if( !task ) {
					ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
				} else {
					//task.resume = true;
					task.start();
				}
			}
			break;
		case "stop":
			{ 
				const task = local.taskMap[msg.id];
				if( !task ) {
					ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
				} else {
					task.resume = false;
					task.stop();
					local.connections.forEach( ()=>
						ws.send( JSOX.stringify( {op:"stop", task } ) ));
				}
			}
			break;
		case "restart":
			{ 
				const task = local.taskMap[msg.id];
				if( !task ) {
					ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
				} else {
					task.resume = true;
					task.stop();
				}
			}
			break;
		case "log":
			{ 
				const task = local.taskMap[msg.id];
				if( !task ) {
					ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
				} else if( !msg.at ) {
					// this adds the socket, and sends the initial log for the task..
					task.ws = ws;
				} else {
					const backlog = task.getLog( msg.at );
					const backLogMsg = { op:"backlog", id:task.id, backlog };
					//console.log( "Sending backlog", backLogMsg );
					ws.send( JSOX.stringify( backLogMsg ))
				}
			}
			break;
		case "tasks":
			//local.tasks
			break;
		}

	}

	function sendTasks() {
		const msg = {op:"tasks", tasks: local.tasks };
		const msg_ = JSOX.stringify( msg );
		ws.send( msg_ );
	}

	function handleClose( code, reason ) {
		//console.log( "Client disconnect:", code, reason );
		const id = local.connections.findIndex( conn=>conn===ws );
		if( id >=0 ) local.connections.splice( id, 1 );
	}

}


