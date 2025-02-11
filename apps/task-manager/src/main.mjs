

const myPath = import.meta.url.split(/\/|\\/g);
const tmpPath = myPath.slice();
tmpPath.splice( 0, 3 );
tmpPath.splice( tmpPath.length-1, 1 );
const appRoot = (process.platform==="win32"?"":'/')+tmpPath.slice(0,-1).join( '/' );
const parentRoot = (process.platform==="win32"?"":'/')+tmpPath.slice(0,-2).join( '/' );

import {System} from "../ui/system.mjs"
import {local} from "./local.mjs"

import os from "os";
import {sack} from "sack.vfs"
const disk = sack.Volume();
export const pwdBare = process.cwd();
let firstLoad = true;
let config = await reloadConfig();

import {openServer} from "../../http-ws/server.mjs"

import {config as taskConfig, Task, closeAllTasks} from "./task.mjs"
taskConfig.pwdBare = pwdBare;
taskConfig.send = send;
taskConfig.config = config;
taskConfig.local = local;
local.addTask = addTask;

const JSOX = sack.JSOX;

async function reloadConfig() {
	const config_run = (await import( (process.platform==="win32"?"file://":"")+pwdBare+"/config.run.jsox" ).catch( err=>(console.log( "parsing error:", err),{default:null}) )).default;
	const config_tasks = (await import( (process.platform==="win32"?"file://":"")+pwdBare+"/config.tasks.jsox" ).catch( err=>(console.log( "parsing error:", err),{default:null}) )).default;
	const config = config_run
			|| config_tasks
	      || (await import( (process.platform==="win32"?"file://":"")+pwdBare+"/config.jsox" ).catch( err=>({default:null}) )).default 
	      || { extraModules:[]
	         , hostname:""
	         , useUpstream: false
	         , upstreamServer: ""
	         , port:0
	         , tasks:[] 
	         };

	if( !firstLoad )
		config.tasks.forEach( loadTask );
	firstLoad = false;
	return config;
}

const serverOpts = {resourcePath:appRoot+"/ui"
	, npmPath:parentRoot+"/.."
	, port:Number(process.env.PORT) || config.port || 8080};
// start server...
console.log( "Serve on port:", serverOpts.port );
const server = openServer( serverOpts, accept, connect );
server.addHandler( (req,res)=>{
	if( req.url.startsWith( "/events")){
		req.url = "/../.." + req.url;
	}
	return false;
})

class Connection {
	ws = null;
	logStreams = [];
	remote = null;
	address = null;
	system = null;
	constructor( ws ) {
		this.ws = ws;
		this.address = ws.connection.remoteAddress;
	}
}

function handleStart( ws, msg, msg_ ) {
	if( (!("system" in msg ) ) || msg.system === local.id ){
		const task = local.taskMap[msg.id];
		if( !task ) {
			ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
		} else {
			//task.restart = true;
			task.start();
		}
	} else {
		const remote = local.systems.find( system=>system.id === msg.system );
		console.log( "Got remote:", remote, remote.connection );
		if( remote ) remote.connection.ws.send( msg_ );
		else ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
	}

}

function handleRestart( ws, msg, msg_ ) {
	if( (!("system" in msg ) ) || msg.system === local.id ){
		const task = local.taskMap[msg.id];
		if( !task ) {
			ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
		} else {
			console.log( "Set task restart:", task );
			task.restart = true;
			console.log( "Task is running alrady?", task.running );
		}
	} else {
		const remote = local.systems.find( system=>system.id === msg.system );
		console.log( "Got remote:", remote, remote.connection );
		if( remote ) remote.connection.ws.send( msg_ );
		else ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
	}

}


function handleStop( ws, msg, msg_ ) {
	if( (!("system" in msg ) ) || msg.system === local.id ){
		const task = local.taskMap[msg.id];
		if( !task ) {
			ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
		} else {
			task.restart = false;
			task.stop();
		}
	} else {
		const remote = local.systems.find( system=>system.id === msg.system );
		if( remote ) remote.connection.ws.send( msg_ );
		else {
			ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
			ws.send( JSOX.stringify( {op:"deleteSystem", id: msg.system } ) );
		}
	}
}


function handleInput( ws, msg, msg_ ) 
{ 
	if( (!("system" in msg ) ) || msg.system === local.id ){
		const task = local.taskMap[msg.id];
		//console.log( "Log request:", msg_ );
		if( !task ) {
			ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
		} else {
			task.run.write( msg.data );
		}
	}
	else {
		const remote = local.systems.find( system=>system.id === msg.system );
		//console.log( "found remote system:", remote, msg_ );
		if( remote ) {
				remote.connection.ws.send( msg_ );
		}
		else {
			ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
			ws.send( JSOX.stringify( {op:"deleteSystem", id: msg.system } ) );
		}
	}
}


function handleLog( ws, msg, msg_ ) 
{ 
	if( (!("system" in msg ) ) || msg.system === local.id ){
		const task = local.taskMap[msg.id];
		//console.log( "Log request:", msg_ );
		if( !task ) {
			ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
		} else if( !msg.at ) {
			// this adds the socket, and sends the initial log for the task..
			task.ws = ws;
			// this will generate 'log' as new messages happen.
		} else {
			const backlog = task.getLog( msg.at );
			const backLogMsg = { op:"backlog", system: local.id, id:task.id, backlog };
			//console.log( "Sending backlog", backLogMsg );
			ws.send( JSOX.stringify( backLogMsg ))
		}
	}
	else {
		const remote = local.systems.find( system=>system.id === msg.system );
		//console.log( "found remote system:", remote, msg_ );
		if( remote ) {
				remote.connection.ws.send( msg_ );
				// this task can wants to be on this connection.
				if( local.taskMap[msg.id] )
					local.taskMap[msg.id].push( ws );
				else
					local.taskMap[msg.id] = [ ws ];
			}
		else {
			ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
			ws.send( JSOX.stringify( {op:"deleteSystem", id: msg.system } ) );
		}
	}
}


function handleTaskInfo( ws, msg, msg_ ) {
	//console.log( "handleTaskInfo:", msg_ );
	if( msg.system === local.id ) {
		const task = local.taskMap[msg.id];
		if( task ){
			ws.send( JSOX.stringify( {op:"taskInfo", id:task.id, task:task.task, title:task.title }));
		}else
			ws.send( JSOX.stringify( {op:"taskInfo", id:task.id, task:null }));
	} else {
		const remote = local.systems.find( system=>system.id === msg.system );
		if( remote ) {
			if( local.replyMap[msg.id] ) {
				local.replyMap[msg.id].push( ws );
			} else local.replyMap[msg.id] = [ws];
			remote.connection.ws.send( msg_ );
		}
	}

}

function connectToCore() {
	console.log( "Connecting upstream...");
	const ws = sack.WebSocket.Client( "ws://"+(config.upstreamServer|| "localhost:8089"), "task-proxy");
	//console.log( "ws?", ws );
	ws.onopen = ()=>{
		console.log( "Sending initial tasks" );
		ws.send( JSOX.stringify( {op:"extern.tasks", tasks:local.tasks
		      , system:config.hostname || os.hostname()
		      , id : local.id
		      , port:serverOpts.port
		      }
		))
		local.upstreamWS = ws;
	}
	ws.onmessage = (msg)=>handleMessage(ws,msg);
	ws.onclose = (code,reason)=>{
		console.log( "Disconnected from upstream" );
		local.upstreamWS = null;
		setTimeout( connectToCore, 5000 );
	}
}

//console.log( "Upstream?", config.useUpstream, config.upstreamServer )
if( config.useUpstream )
	connectToCore();



config.tasks.forEach( loadTask );

function loadTask( task ) {
	const oldTask = local.tasks.find( oldTask=>oldTask.name === task.name );
	if( !oldTask ) {
		const newTask = new Task( task );
		if( !config.tasks.find( oldTask=>oldTask.name === task.name )){
			config.tasks.push( task );
		}
		local.tasks.push( newTask );
		local.taskMap[newTask.id] = newTask;
		return newTask;
	}else {
		oldTask.update( task );
		return oldTask;
	}
}

function loadModules( n ) {
	if( n >= config.extraModules.length )
		return startTasks();
	import( config.extraModules[n].name ).then( (module)=>{
		module[config.extraModules[n].function](config.extraModules[n].options).then( ()=>{
			loadModules( n+1 );
		} ).catch( (err)=>{
			console.log( "Error loading:", config.extraModules[n].name, config.extraModules[n].function );
			loadModules( n+1 );
		} )
	} ).catch( (err)=>{
			console.log( "Error loading:", config.extraModules[n].name, config.extraModules[n].function );
			loadModules( n+1 );
		} );
}

function onStopAll( n ) {
	if( !config.onStopAll || n >= config.onStopAll.length )
		return;
	return import( config.onStopAll[n].name ).then( (module)=>{
		module[config.onStopAll[n].function](config.onStopAll[n].options).then( ()=>{
			return loadModules( n+1 );
		} ).catch( (err)=>{
			console.log( "Error loading:", config.onStopAll[n].name, config.onStopAll[n].function );
			return loadModules( n+1 );
		} )
	} ).catch( (err)=>{
			console.log( "Error loading:", config.onStopAll[n].name, config.onStopAll[n].function );
			return loadModules( n+1 );
		} );
}


if( config.extraModules ) {
	loadModules( 0 );
}
else startTasks();

function startTasks() {
	local.tasks.forEach( task=>{
		if (!task.running 
                   && !task.hasDepends 
		   && !task.noAutoRun){
			task.start() 
		}} );
}
// start client interface to server.
//sack.Task( { work:programRoot+"/../ui", bin:"cmd", args:["/C", "start", "http://localhost:8080/index.html" ] } );

//setTimeout( ()=>{console.log( "Timeout closing" )}, 5000 );

export function send( msg_ ) {
	// should append my system id
	if( "string" === typeof msg_ ) {
		if( local.upstreamWS && local.upstreamWS.readyState === 1 ) local.upstreamWS.send( msg_ );
		local.connections.forEach( conn=>(conn.ws.readyState == 1) &&conn.ws.send( msg_ ) );
	} else {
		const msg = JSOX.stringify( msg_ );
		if( local.upstreamWS && local.upstreamWS.readyState === 1 ) local.upstreamWS.send( msg );
		local.connections.forEach( conn=>(conn.ws.readyState == 1) &&conn.ws.send( msg ) );
	}
}

function accept( ws ) {
	this.accept();
}



function connect( ws ) {
	//console.log( "Connect ws:", ws.headers );
	const connection = new Connection( ws );
	const protocol = ws.headers["Sec-WebSocket-Protocol"];
	if( protocol === "task-proxy" )
		ws.onmessage = handleProxyMessage;
	else if( protocol === "tasks" ) { // client UI
		ws.onmessage = (msg)=>handleMessage(ws,msg);
		console.log( "Adding connection...");
		local.connections.push( connection );
		sendTasks();
	
	} else {
		ws.close( 1020, "Bad Protocol" );
		return;
	}

	ws.onclose = handleClose;

	// this is from a peer connecting to upstream
	function handleProxyMessage( msg_ ) {
		const msg = JSOX.parse( msg_ );
		//console.log( "Received (from proxy):", msg );
		switch( msg.op ) {
		case "taskInfo":
			const replyTo = local.replyMap[msg.id];
			//console.log( "Info reply should rely to:", replyTo, local.replyMap );
			if( replyTo ) {
				replyTo.forEach( ws=>(ws.readyState===1) && ws.send( msg_ ) );
				delete local.replyMap[msg.id];
			} else {
				//ws.send( JSOX.stringify( {op:"remote disappeared?"}))
			}
			break;
		case "log": {
			// relay log forward... someone asked for logging
			// to which connections?
			const sendTo = local.taskMap[msg.id];
			if( sendTo ) {
				sendTo.forEach( ws=>(ws.readyState===1) && ws.send( msg_ ) );
			} else {
				//ws.send( JSOX.stringify( {op:"remote disappeared?"}))
			}

		}
		break;
		case "backlog": {
			const sendTo = local.taskMap[msg.id];
			if( sendTo ) {
				sendTo.forEach( ws=>(ws.readyState===1) && ws.send( msg_ ) );
			}else {
				//ws.send( JSOX.stringify( {op:"remote disappeared?"}))
			}
		}break;
		case "addTask": {
			// received from creating a remote task
			connection.system.addTask( msg.id, msg.task );
			if( local.upstreamWS ) local.upstreamWS.send( JSOX.stringify( {op:msg.op, system:connection.system.id, task:msg.task } ) );
			send( {op:msg.op, system:connection.system.id, task:msg.task});
		}
		break;
		case "updateTask": {
			// received from updating a remote task
			connection.system.updateTask( msg.task );
			if( local.upstreamWS ) local.upstreamWS.send( JSOX.stringify( {op:msg.op, system:connection.system.id, task:msg.task } ) );
			send( {op:msg.op, system:connection.system.id, task:msg.task});
		}
		break;
		case "deleteTask": {
			// received from deleting a remote task
			connection.system.deleteTask( msg.task );
			if( local.upstreamWS ) local.upstreamWS.send( msg_ );
			send( {op:msg.op, system:connection.system.id, task:msg.task});
		}
		break;
		case "status": {
			// update internal version of statuses.
			for( let n = 0; n < local.systems.length; n++ ) {
				const system = local.systems[n];
				if( system.id === msg.system ) {
					for( let task of system.tasks ) {
						if( task.id === msg.id ) {
							task.running = msg.running;
						}
					}
				}
			}
			// send to all other connections a status update...
			// msg.system should be a remote system id, but I don't hae to insert it based on
			// connection.system.id
			if( local.upstreamWS ) local.upstreamWS.send( msg_ );
			send( msg_ );
			break;
		}
		case "extern.tasks": {
				let n;
				let system = null;
				//console.log( "Got external tasks...", msg );
				//console.log( "looking at systems:", local.systems );
				//console.log( "Connection system?", connection.system );
				if( connection.system ){
					if( connection.system.id === msg.id ){
						console.log( "This shouldn't happen.. we should have already been connected to this system...")
						system = connection.system;
					}else {
						for( n = 0; n < connection.systems.length; n++ ) {
							const testSystem = connection.systems[n];
							if( testSystem.id === msg.id ){
								connection.system = testSystem;  // this really shouldn't happen...
								console.log( "Found existing system to replace tasks. (this probably shouldn't happen with IDs)" );
								system = testSystem;
								system.connection =  connection;						
								system.tasks = msg.tasks;
								break;
							}
						}
					}
				} else
					for( n = 0; n < local.systems.length; n++ ) {
						const testSystem = local.systems[n];
						if( testSystem.id === msg.id ){
							system = testSystem;
							console.log( "Found existing system to replace tasks." );
							system.connection =  connection;						
							system.tasks = msg.tasks;
							break;
						}
					}
				if( !system ){
					console.log( "Make a new system" );
					// this is the connection that the system can be reached on...
					system = new System( connection, msg.id, msg.port, msg.system, msg.tasks);
					// if this already heard tasks, this is probably a chlid system of the remote
					// which will go under that system's systems.
					if( connection.system ){
						system.upstream = connection.system;
						connection.system.systems.push( system );
					}
					else {
						connection.system = system;
						// upstream is self. (null) 
						// another level above me would have this upstream as me...
					}
					// every remote sys in local.systems.
					local.systems.push( system );
				}
				if( local.upstreamWS ) {
					local.upstreamWS.send( msg_ );
				}
				send( msg_ );
			}
			break;
		}
	}


	function sendTasks() {
		const msg = {op:"tasks", system:local.id, tasks: local.tasks, systems: local.systems };
		const msg_ = JSOX.stringify( msg );
		ws.send( msg_ );
	}


	function handleClose( code, reason ) {
		if( protocol === "task-proxy"){
			// need to forget this system.
			const systemindex = local.systems.findIndex( system=>system.connection === connection );
			console.log( "did we find proxy connection?", systemindex);
			if( systemindex >= 0 ) {
				local.systems.splice( systemindex, 1 );
				console.log( "connection too:", connection.system );
				send( {op:"deleteSystem", id: connection.system.id});
			}
		}
		console.log( "Client disconnect:", code, reason );
		const id = local.connections.findIndex( conn=>conn.ws===ws );
		//console.log( "Did we find the connection?", id );
		if( id >=0 ) local.connections.splice( id, 1 );
	}

}


	function addTask( id, task ) {
		if( local.upstreamWS ) local.upstreamWS.send( {op:"addTask", system:local.id, id, task } );
		send( {op:"addTask", system:local.id, id, task } );
	}
	function updateTask( id, task ) {
		if( local.upstreamWS ) local.upstreamWS.send( {op:"updateTask", system:local.id, id, task } );
		send( {op:"updateTask", system:local.id, id, task } );
	}
	function deleteTask( id ) {
		if( local.upstreamWS ) local.upstreamWS.send( {op:"deleteTask", system:local.id, id } );
		send( {op:"deleteTask", system:local.id, id } );
	}

function handleMessage( ws, msg_ ) {
	try {
		const msg = JSOX.parse( msg_ );
		switch( msg.op ) {
		case "shutdown": {
			console.log( "received shutdown request" );
			closeAllTasks(msg.close?ws:null).then( ()=>{
				console.log( "Close resulted, and we're exiting now." );
				setTimeout( ()=>{process.exit(msg.stop?1:0);}, 1000 );
			} );
			break;
		}
		case "stopAll": {
			console.log( "Stopping all tasks", msg.close );
			if( msg.close )
				closeAllTasks( ws ).then( onStopAll );
			else 
				closeAllTasks().then( onStopAll );
			break;
		}
		case "startAll": {
			console.log( "Start all tasks" );
			startTasks();
			if( msg.close )
				ws.close( 1000, "Starting Tasks" );
			break;
		}
		case "task_logging":
			for( let task of local.tasks ) {
				if( task.id === msg.id ) {
					task.ws = ws;
					break;
				}
			}
			break;
		case "start":
			handleStart( ws, msg, msg_ );
			break;
		case "stop":
			handleStop( ws, msg, msg_ );
			break;
		case "restart":
			handleRestart( ws, msg, msg_ );
			break;
		case "log":
			handleLog( ws, msg, msg_ );
			break;
		case "send":
			handleInput( ws, msg, msg_ );
			break;
		case "createTask": {
			if( local.system === msg.system || !msg.system ) {
				const task = loadTask( msg.task );
				if( !msg.task.temporary )
					saveRunConfig();
				addTask( task.id, task ); // sends new task
				if( !task.noAutoRun ) task.start();
			} else if( msg.system && msg.system != local.system ) {
				local.systems.find( system=>{
					if( system.id === msg.system ) {
						system.createTask( msg_ );
						return true;
					} 
					return false;
				});
			}else {
				const task = loadTask( msg.task );
				if( !msg.task.temporary )
					saveRunConfig();
				addTask( task.id, task );
				if( !task.noAutoRun ) task.start();
			}
			}
			break;
		case "updateTask": {
			if( !msg.system || msg.system === local.id ) {
				const task = local.taskMap[msg.id];
				if( task )
				{
					const taskInfo = task.task;
					task.update( msg.task );
					for( let t = 0; t < config.tasks; t++ ) {
						if( config.task[t] === taskInfo ) {
							const keys = Object.keys( msg.task );
							for( let key of keys ) {
								taskInfo[key] = msg.task[key];
							}
						}
						msg.task = taskInfo;
						msg_ = JSOX.stringify( msg );
					}
					saveRunConfig();
					updateTask( msg.id, task.task );
				}
			}else {
				if( connection.system )
					connection.system.updateTask( msg.id, msg.task );
				else
					console.log( "Told to update a task I don't know, and can't reach?", msg );
			}
		}
			break;
		case "deleteTask": {
			if( !msg.system || msg.system ===local.system ) {
				const task = local.taskMap[msg.id];
				if( task ) {
					const taskInfo = task.task;
					for( let t = 0; t < config.tasks.length; t++ ) {
						if( config.tasks[t] === taskInfo ) {
							config.tasks.splice( t, 1 );
							for( let t2 = 0; t2 < local.tasks.length; t2++ ) {
								if( local.tasks[t2] === task ) {
									local.tasks.splice( t2, 1 );
									break;
								}
							}
							delete local.taskMap[msg.id];
							if( task.running )
								task.stop();
							if( !taskInfo.temporary )
								saveRunConfig();
							break;
						}
					}
					send( msg_ );
				}
			} else {
				// send to remote system...
			}
			}
			break;
		case "getDisplays": {
				const displays = sack.Task.getDisplays();
				for( let device of displays.device ) {
					for( let monitor of displays.monitor ) {
						if( monitor.display === device.display ) {
							device.monitor = monitor;
							monitor.device = device;
							break;
						}
					}
				}
				ws.send( JSOX.stringify( { op:"displays", displays: displays } ) );
			}
			break;
		case "getTaskInfo":
			handleTaskInfo( ws, msg, msg_ );
			break;
		case "updateDisplay": {
				const task = local.taskMap[msg.id];
				if( !task ) {
					// task on a remote system?
				} else {
					const taskInfo = task.task;
					if( "moveTo" in taskInfo ) {
						if( "monitor" in msg ) {
							delete taskInfo.moveTo.display;
							taskInfo.moveTo.monitor = msg.monitor;
						} else {
							delete taskInfo.moveTo.monitor;
							taskInfo.moveTo.display = msg.display;
						}
					} else {
						taskInfo.moveTo = {
							display: msg.display,
							monitor: msg.monitor,
							timeout: 2500,
						};
					}
					task.move();
					saveRunConfig();
				}
			}
			break;
		}
	} catch( err ) {
		console.log( "Exception?", err );
	}
}


function saveRunConfig() {
	const c = Object.assign( {}, config );
        c.tasks = c.tasks.reduce( (acc,task)=>{if( !task.temporary ) acc.push( task ); return acc;}, [] );
	const output = JSOX.stringify( config, null, "\t" );
	disk.write( "config.run.jsox", output );
}

if( "enableExitSignal" in sack.system ) {
	sack.system.enableExitSignal( ()=>{
		console.log( "Got exit signal... so generate exit?" );
		closeAllTasks().then( ()=>{
			console.log( "Took some time to shut down tasks?" );
			//process.emit( "SIGINT" );
			process.exit(0);
		});
	} );
}

//process.on( "SIGINT", ()=>{ process.stdout.write( "SIGINT\n" ); closeAllTasks().then( ()=>{ console.log( "sigint terminate finished" ); } ) } );

//process.on( "uncaughtException", (err)=>{ process.stdout.write( "uncaught Exception" + err ); } );

