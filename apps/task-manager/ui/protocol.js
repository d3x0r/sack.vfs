

import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"
import {Events} from "/node_modules/sack.vfs/apps/events/events.mjs"
import {System} from "./system.mjs"


export const config = {
	local : { tasks: {},
			refresh : ()=>{},
			reset : ()=>{},
		systems : [],
	},
	//AddTaskList : ()=>{},
	addTaskLog : ()=>{},
	editTask:()=>{},
	insertBackLog : ()=>{},
}


export class MySystem extends System {
	constructor(msg) {
		super( {address:""}, msg.id, msg.port, msg.system, msg.tasks )
	}

	addTask( id, task ) {
		super.addTask( id, task );
	}
	updateTasks( tasks ) {
		console.log( "This system has new tasks?", this, tasks )
	}
}

export class Protocol extends Events {
	static displayRequests = [];
	static taskRequests = [];
	ws = null;

	stopTask( group, task ) {
		if( !task ) return;
		const system = config.local.systems.find( system=>system === group );
		if( system )
			this.ws.send( JSOX.stringify( {op:"stop", system:system.id, id:task.id } ) );
		else			
			this.ws.send( JSOX.stringify( {op:"stop", id:task.id } ) );
	}
	startTask( group, task ) {
		if( !task ) return;
		const system = config.local.systems.find( system=>system === group );
		if( system )
			this.ws.send( JSOX.stringify( {op:"start", system:system.id, id:task.id } ) );
		else			
			this.ws.send( JSOX.stringify( {op:"start", id:task.id } ) );
	}
	editTask( ) {
	}
	restartTask( group, task ) {
		const system = config.local.systems.find( system=>system === group );
		if( system )
			this.ws.send( JSOX.stringify( {op:"restart", system:system.id, id:task.id } ) );
		else			
			this.ws.send( JSOX.stringify( {op:"restart", id:task.id } ) );
	}
	showLog( group, task ) {
		const system = config.local.systems.find( system=>system === group );
		if( system )
			this.ws.send( JSOX.stringify( {op:"log", system:system.id, id:task.id } ) );
		else			
			this.ws.send( JSOX.stringify( {op:"log", id:task.id } ) );
	}

	setTaskMonitor( task, monitor ) {
		this.ws.send( JSOX.stringify( {op:"updateDisplay", id:task.id, monitor:monitor}));
	}
	setTaskDisplay( task, display ) {
		this.ws.send( JSOX.stringify( { op:"updateDisplay", id:task.id, display:display}));
	}

	async getDisplays( ) {
		const p = new Promise( (res,rej)=>{
			Protocol.displayRequests.push( res );
			this.ws.send( "{op:getDisplays}" );
		})
		return p;
	}
	async getTaskInfo( task ) {
		const p = new Promise( (res,rej)=>{
			const system = config.local.systemMap[task];
			Protocol.taskRequests.push( {id:task,res} );
			console.log("We are here in Get Task Info with "+task);
			this.ws.send( JSOX.stringify(  {op:"getTaskInfo", system:system.id, id:task} ));
		});//.then( (obj)=>{/*obj is .task and .title which is task.title*/ /*console.log( "Got Data ?", task );*/  return obj;} );
		return p;
	}

	createTask( task ) {
		// event comes back as an addTask
		this.ws.send( JSOX.stringify( {op:"createTask",task} ) );
	}

	updateTask( id, task ) {
		// event comes back as updateTask
		this.ws.send( JSOX.stringify( {op:"updateTask",id, task} ) );
	}

	deleteTask( id ) {
		// delete comes back as deleteTask
		this.ws.send( `{op:deleteTask,id:"${id}"}` );
	}

 	connect( to ) {
		to = to || location.protocol.replace( "http", "ws" )+"//"+location.host+"/";
		const ws = new WebSocket( to, "tasks");
		config.local.ws = this.ws = ws;
		ws.onopen = function() {
		// Web Socket is connected. You can send data by send() method.
		//ws.send("message to send"); 
		//ws.send( JSON.stringify( { MsgID: "flashboard" } ) );
		};
		ws.onmessage = function (evt) { 
				const received_msg = evt.data; 
			processMessage( JSOX.parse( received_msg ) );
		};
		ws.onclose = function() { 
			console.log( "Got a disconnect... should reconnect" );
			config.local.reset();
			setTimeout( ()=>protocol.connect( to ), 5000 );
			// websocket is closed. 
		};

		function processMessage( msg ) {
			switch( msg.op ) {
			case "tasks":
				  // this would come in as extern.tasks ?
				if( "systems" in msg )
					for( let system of msg.systems ) {
						//console.log( "System:", system );
						system = new MySystem( system );
						protocol.on( "addSystem", system );
					}
				config.local.system = msg.system;
				config.local.taskData = msg.tasks;
				for( let task of msg.tasks ) {
					protocol.on( "addTask",  [task.id,task] );
					config.local.tasks[task.id] = task;
					config.local.systemMap[task.id] = {id:msg.system,tasks:msg.tasks};
				}
				// this should publish an event; but this was tied into the UI.
				protocol.on( "addTaskList", [ config.local.firstPage, config.local,  "taskData" ])

				break;
			case "addTask":
				if( msg.system && msg.system !== config.local.system) {
					for( let system of config.local.systems ) {
						if( system.id === msg.system ) {
							system.addTask( msg.id, msg.task );
							return;
						}
					}
				}
				else {
					config.local.taskData.push( msg.task );
					protocol.on("addTask", [msg.id, msg.task] );
				}
				break;
			case "updateTask":
				protocol.on( "updateTask", [msg.id, msg.task] );
				break;
			case "deleteTask":
				protocol.on( "deleteTask", msg.id );
				break;
			case "extern.tasks":
				protocol.on( "extern.task", msg );
				break;
			case "deleteSystem":
				protocol.on( "deleteSystem", msg.id );
				break;
			case "delete":
				{
					const task = config.local.tasks[msg.id];
					config.local.statusDisplay.deleteRow( task );
					for( let t = 0; t < config.local.taskData.length; t++ ) {
						if( config.local.taskData[t].id === msg.id ) {
							config.local.taskData.splice( t, 1 );
							break;
						}
					}
				}
				break;
			case "backlog":
				{
				const log = config.local.logs[msg.id];
					config.insertBackLog( log, msg.backlog )
				}
				break;
			case "log":
				{
					const log = config.local.logs[msg.id];
					const task = config.local.tasks[msg.id];
					if( task && !log ) {
						config.addTaskLog( task, msg.log );
					} else {
						//log.logFrame.show();
						//debugger;
						if( "at" in msg ) 
							;//insertBackLog( msg )
						else
							log.add( msg.log );
					}
				}
				break;
			case "stopping":
				protocol.on( "stopping", msg.task );			
				break;
			case "stop":
				protocol.on( "stop", msg.task );			
				break;
			case "status":
				
				{
					const task = config.local.tasks[msg.id];
					const system = config.local.systemMap[msg.id];
					if( task && !system )
					{
						if( msg.failed ) task.running = 0;
						else task.running = msg.running;
						task.ended = msg.ended;
						task.started = msg.started;
						config.local.refresh();
						//console.log( "Replacing status?  need to update statuses" );
						return;
					} else {
						{
							let task = null;
							for( task of system.tasks ) {
								if( task.id === msg.id ) {
									if( msg.failed ) task.running = false;
									else task.running = msg.running;
									task.ended = msg.ended;
									task.started = msg.started;
									config.local.refresh();
									break;
								}
							}
							if( task) break;
						}
					}
				}
				console.log( "Status for unknown task" );
				break;
			case "displays":
				{
					const request = Protocol.displayRequests.shift();
					request( msg.displays );
				}
				break;
			case "taskInfo":
				{
					for( let r = 0; r < Protocol.taskRequests.length; r++ ) {
						const request = Protocol.taskRequests[r];
						if( request.id === msg.id ) {
							request.res( {task: msg.task, title: msg.title } );
							Protocol.taskRequests.splice( r, 1 );
							break;
						}
					}
				}
				break;
			}
		}
	}
}

export const protocol = new Protocol();
export default protocol;
