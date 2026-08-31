

const myPath = import.meta.url.split(/\/|\\/g);
const tmpPath = myPath.slice();
tmpPath.splice( 0, 3 );
tmpPath.splice( tmpPath.length-1, 1 );
const appRoot = (process.platform==="win32"?"":'/')+tmpPath.slice(0,-1).join( '/' );
const parentRoot = (process.platform==="win32"?"":'/')+tmpPath.slice(0,-2).join( '/' );

import {System} from "../ui/system.mjs"
import {local} from "./local.mjs"
import {isTopLevel} from "sack.vfs/isTopLevel" 

import os from "os";
import {sack} from "sack.vfs"
const disk = sack.Volume();
export const pwdBare = process.cwd();
let firstLoad = true;

import {openServer} from "../../http-ws/server.mjs"
import {setupRest} from "./main.rest.mjs"

import {config as taskConfig, Task, closeAllTasks} from "./task.mjs"
taskConfig.pwdBare = pwdBare;
taskConfig.send = send;
taskConfig.config = config;
taskConfig.local = local;
local.addTask = addTask;

const JSOX = sack.JSOX;
import {config} from "./config.mjs"

// Plugins.  An ordered list of { name, function, options }, run in sequence:
// with a `function` the next one waits for it, without one the module is just
// imported for its side effects and the list moves straight on.
if( config.extraModules ) {	
	await new Promise( (res,rej)=>{
		loadModules( 0 );
	
		function loadModules( n ) {
			if( n >= config.extraModules.length ) {
				return res();
			}
			const plugin = config.extraModules[n];
			const next = ()=>loadModules( n+1 );
			return import( "file://"+pwdBare+"/"+plugin.name ).then( (module)=>{
				// no function named: importing it was the whole point
				if( !plugin.function ) return next();
				const entry = module[plugin.function];
				if( "function" !== typeof entry ) {
					console.log( "Plugin has no such export:", plugin.name, plugin.function );
					return next();
				}
				// a plugin that returns nothing is not an error - only wait when
				// there is something to wait on.  Calling .then() on whatever it
				// returned used to throw straight into the "Error loading" catch.
				return Promise.resolve( entry( plugin.options ) ).then( next, (err)=>{
					console.log( "Error running:", plugin.name, plugin.function, err );
					return next();
				} );
			} ).catch( (err)=>{
					console.log( "Error loading:", plugin.name, err );
					return next();
				} );
		}
	} );
}


config.tasks.forEach( loadTask );



const serverOpts = {resourcePath:process.env.RESOURCE_PATH || (appRoot+"/ui")
	, npmPath:process.env.NPM_PATH || (parentRoot+"/..")
	, port:Number(process.env.PORT) || config.port || 8080};
// start server...
console.log( "Serve on port:", serverOpts.port );
export const server = openServer( serverOpts, accept, connect );

setupRest( server );

server.addHandler( (req,res)=>{
	if( req.url.startsWith( "/events")){
		req.url = "/../.." + req.url;
	}
	return false;
})

let authCb = null;
export function onLogin( loginCb ) {
	authCb = loginCb;
	
}

// optional expect handler, otherwise use getUser on the key....
// default expect handler (null for last callback)
// does this same operation.


class Connection {
	ws = null;
	logStreams = [];
	remote = null;
	address = null;
	system = null;
	authed = false;
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
			// `task` is what was just found to be missing - reading its .id threw
			// out of the message handler, so the request never got any reply and
			// the editor sat waiting on the promise forever.
			ws.send( JSOX.stringify( {op:"taskInfo", id:msg.id, task:null }));
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
	ws.onopen = ()=>{
		//console.log( "Sending initial tasks", local.tasks );
		ws.send( JSOX.stringify( {op:"extern.tasks", tasks:local.tasks
		      , system:config.hostname || os.hostname()
		      , id : local.id
		      , port:serverOpts.port
		      // tell upstream not to offer controls it will not be allowed to use
		      , disallowUpstreamTaskManagment: !!config.disallowUpstreamTaskManagment
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


export function beginScheduler() {
	//console.log( "Loading tasks?", config.tasks, local.tasks );
	config.tasks.forEach( loadTask );

	startTasks();

}

if( isTopLevel(import.meta.url) ) beginScheduler();

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

function onStopAll( n ) {
	if( !config.onStopAll || n >= config.onStopAll.length )
		return;
	// this used to chain to loadModules(), which is local to the extraModules
	// block above and not in scope here - so the second hook onward never ran,
	// and the ReferenceError went nowhere.
	const hook = config.onStopAll[n];
	const next = ()=>onStopAll( n+1 );
	return import( hook.name ).then( (module)=>{
		if( !hook.function ) return next();
		const entry = module[hook.function];
		if( "function" !== typeof entry ) {
			console.log( "Stop hook has no such export:", hook.name, hook.function );
			return next();
		}
		return Promise.resolve( entry( hook.options ) ).then( next, (err)=>{
			console.log( "Error running stop hook:", hook.name, hook.function, err );
			return next();
		} );
	} ).catch( (err)=>{
			console.log( "Error loading stop hook:", hook.name, err );
			return next();
		} );
}



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
	if( protocol === "task-proxy" ) {
		//console.log( "Remote system connection..." );
		// handleMessage() guards itself; this one did not, so a throw mid-case
		// skipped the upstream/downstream forwarding at the end of the case.
		ws.onmessage = (msg_)=>{
			try {
				handleProxyMessage( msg_ );
			} catch( err ) {
				console.log( "Error handling proxy message:", err );
			}
		};
	} else if( protocol === "tasks" ) { // client UI
		ws.onmessage = (msg)=>handleMessage(ws,msg);
		console.log( "Adding task info connection...");
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
		case "login":
			/* this just dispatches an event? */
			if( authCb ) {
				if( authCb( msg.uid ) ) {
					ws.authed = true;
				}
			}
			break;
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
		// These three relay a remote system's own change on to this system's
		// clients.  They all used to drop `id` on the way out, which is what
		// every one of those clients keys the task off; and send() already
		// relays upstream, so the extra upstreamWS.send was a duplicate.
		case "addTask": {
			// received from creating a remote task
			connection.system.addTask( msg.id, msg.task );
			send( {op:msg.op, system:connection.system.id, id:msg.id, task:msg.task});
		}
		break;
		case "updateTask": {
			// received from updating a remote task
			connection.system.updateTask( msg.id, msg.task );
			send( {op:msg.op, system:connection.system.id, id:msg.id, task:msg.task});
		}
		break;
		case "deleteTask": {
			// received from deleting a remote task
			connection.system.deleteTask( msg.id );
			send( {op:msg.op, system:connection.system.id, id:msg.id});
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
				// local.systems indexes every known system at any depth, so it is
				// the authoritative lookup by id - the old per-connection scans
				// missed reconnects and read a `connection.systems` that Connection
				// has never had.
				for( n = 0; n < local.systems.length; n++ ) {
					const testSystem = local.systems[n];
					if( testSystem.id === msg.id ){
						system = testSystem;
						break;
					}
				}
				if( system ){
					// re-report: a reconnect, or the peer's task list changed.
					// Refresh in place so the id keeps resolving to one object.
					//console.log( "Found existing system to replace tasks." );
					system.connection = connection;
					system.tasks = msg.tasks;
					system.disallowUpstreamTaskManagment = !!msg.disallowUpstreamTaskManagment;
					// the create path below sets this; reconnects need it too or
					// later addTask/updateTask deref a null connection.system.
					if( !connection.system ) connection.system = system;
				}
				if( !system ){
					//console.log( "Make a new system", msg.tasks );
					// this is the connection that the system can be reached on...
					system = new System( connection, msg.id, msg.port, msg.system, msg.tasks);
					system.disallowUpstreamTaskManagment = !!msg.disallowUpstreamTaskManagment;
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


	// send() already relays to local.upstreamWS, and it stringifies first; the
	// extra send of the raw object each of these used to do reached upstream as
	// "Unhandled message format", so an upstream master never saw these at all.
	function addTask( id, task ) {
		send( {op:"addTask", system:local.id, id, task } );
	}
	function updateTask( id, task ) {
		send( {op:"updateTask", system:local.id, id, task } );
	}
	function deleteTask( id ) {
		send( {op:"deleteTask", system:local.id, id } );
	}

function handleMessage( ws, msg_ ) {
	try {
		const msg = JSOX.parse( msg_ );
		const connection = local.connections.find( (c)=>c.ws===ws );
		// `disallowUpstreamTaskManagment` in the config makes this system refuse
		// to have its task list edited by whoever it reports to.  Upstream is
		// told about the setting when we connect, so its UI hides the controls;
		// this is the enforcement behind that, for anything that gets sent
		// anyway.  It deliberately covers only the task edits - shutdown and
		// stopAll/startAll are still honoured.
		if( config.disallowUpstreamTaskManagment
		 && local.upstreamWS && ws === local.upstreamWS
		 && ( msg.op === "createTask" || msg.op === "updateTask" || msg.op === "deleteTask" ) ) {
			console.log( "Refused upstream task management:", msg.op, msg.id || (msg.task && msg.task.name) );
			return;
		}
		switch( msg.op ) {
		case "shutdown": {
			if( msg.system && msg.system !== local.id ) {
				// aimed at one of the systems reporting to this one
				const remote = local.systems.find( system=>system.id === msg.system );
				if( remote ) remote.connection.ws.send( msg_ );
				else console.log( "Told to shut down a system I can't reach:", msg.system );
				break;
			}
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
			// same as the delete case: the local system's identity is local.id,
			// so a create addressed to this system by name fell through to the
			// remote lookup and was dropped.
			if( !msg.system || msg.system === local.id ) {
				const task = loadTask( msg.task );
				if( !msg.task.temporary )
					saveRunConfig();
				addTask( task.id, task ); // sends new task
				if( !task.noAutoRun ) task.start();
			} else {
				// forward to the system that owns it, the same way
				// handleStart/handleStop do.  (System.createTask() sends through
				// the Connection itself, which has no send().)
				const remote = local.systems.find( system=>system.id === msg.system );
				if( remote ) remote.connection.ws.send( msg_ );
				else console.log( "Told to create a task on a system I can't reach:", msg );
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
				// `connection` is the browser asking for the change, and its
				// .system is null - the update has to go to the system that
				// owns the task.
				const remote = local.systems.find( system=>system.id === msg.system );
				if( remote ) remote.connection.ws.send( msg_ );
				else console.log( "Told to update a task I don't know, and can't reach?", msg );
			}
		}
			break;
		case "deleteTask": {
			// `local.system` is not a field - the local system is `local.id`,
			// so a delete that named the local system never matched here.
			if( !msg.system || msg.system === local.id ) {
				const task = local.taskMap[msg.id];
				if( task ) {
					const taskInfo = task.task;
					// all of this used to be conditional on finding the task in
					// config.tasks, so a task that wasn't in the saved config
					// stayed in local.tasks and came back on the next task list.
					const cfgIdx = config.tasks.indexOf( taskInfo );
					if( cfgIdx >= 0 ) config.tasks.splice( cfgIdx, 1 );
					const taskIdx = local.tasks.indexOf( task );
					if( taskIdx >= 0 ) local.tasks.splice( taskIdx, 1 );
					delete local.taskMap[msg.id];
					if( task.running )
						task.stop();
					if( !taskInfo.temporary )
						saveRunConfig();
					deleteTask( msg.id );
				}
			} else {
				// forward to the system that owns it, the same way
				// handleStart/handleStop do.
				const remote = local.systems.find( system=>system.id === msg.system );
				if( remote ) remote.connection.ws.send( msg_ );
				else ws.send( JSOX.stringify( {op:"delete", id: msg.id } ) );
			}
			}
			break;
		case "getPlugins": {
			if( msg.system && msg.system !== local.id ) {
				const remote = local.systems.find( system=>system.id === msg.system );
				if( remote ) remote.connection.ws.send( msg_ );
				else console.log( "Told to list plugins on a system I can't reach:", msg.system );
				break;
			}
			ws.send( JSOX.stringify( { op:"plugins", system:local.id
			                         , plugins: config.extraModules || [] } ) );
			break;
		}
		case "setPlugins": {
			if( msg.system && msg.system !== local.id ) {
				const remote = local.systems.find( system=>system.id === msg.system );
				if( remote ) remote.connection.ws.send( msg_ );
				else console.log( "Told to set plugins on a system I can't reach:", msg.system );
				break;
			}
			// only entries with a module name are worth keeping; `function` is
			// optional - such a plugin is imported and not waited on.
			config.extraModules = ( msg.plugins || [] ).filter( plugin=>plugin && plugin.name );
			saveRunConfig();
			console.log( "Plugin list updated;", config.extraModules.length
			           , "plugin(s) - they load on the next start of this service manager." );
			send( { op:"plugins", system:local.id, plugins: config.extraModules } );
			break;
		}
		case "getDisplays": {
				const displays = sack.Task.getDisplays();
				for( let device of displays.device ) {
					for( let monitor of displays.monitor ) {
						if( monitor.display === device.display ) {
							device.monitorName = device.monitor;
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
	disk.write( process.env.TASK_MANAGER_RUN_CONFIG||"config.run.jsox", output );
}

if( "enableExitSignal" in sack.system ) {
	sack.system.enableExitSignal( ()=>{
		//console.log( "Got exit signal... so generate exit?" );
		closeAllTasks().then( ()=>{
			//console.log( "Took some time to shut down tasks?" );
			//process.emit( "SIGINT" );
			process.exit(0);
		});
	} );
}

//process.on( "SIGINT", ()=>{ process.stdout.write( "SIGINT\n" ); closeAllTasks().then( ()=>{ console.log( "sigint terminate finished" ); } ) } );

//process.on( "uncaughtException", (err)=>{ process.stdout.write( "uncaught Exception" + err ); } );

