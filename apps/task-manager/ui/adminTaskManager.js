
import {Popup, popups} from "/node_modules/@d3x0r/popups/popups.mjs"

var QueryString = function () {
  // This function is anonymous, is executed immediately and 
  // the return value is assigned to QueryString!
  var query_string = {};
  var query = window.location.search.substring(1);
  var vars = query.split("&");
  for (var i=0;i<vars.length;i++) {
    var pair = vars[i].split("=");
        // If first entry with this name
    if (typeof query_string[pair[0]] === "undefined") {
      query_string[pair[0]] = decodeURIComponent(pair[1]);
        // If second entry with this name
    } else if (typeof query_string[pair[0]] === "string") {
      var arr = [ query_string[pair[0]],decodeURIComponent(pair[1]) ];
      query_string[pair[0]] = arr;
        // If third or later entry with this name
    } else {
      query_string[pair[0]].push(decodeURIComponent(pair[1]));
    }
  } 
  return query_string;
}();


//import {constants,wait} from "/common/system/constants.loader.js"
//import {config as siteConfig} from "./config.js"

//await wait;
let wantTouchButton = false;
let taskServerAddr = location.protocol + "//"+location.hostname+":"+"8088";
const taskServerAddrAlt = location.protocol + "//"+location.hostname+":"+"8088";
const {protocol,config} = await import(  taskServerAddr+"/protocol.js" ).catch( async err=>{
	taskServerAddr = taskServerAddrAlt;
	return import( taskServerAddrAlt + "/protocol.js" ).catch( err=>{
		return {protocol:null,config:null};
	} );
} );

if( !protocol ) {
	const button = window.document.getElementById( "adminTaskManager" );
	console.log( "got button? no?", button );
}

const local = {
	serverTask : null,
	clickTask : null,
	stopped : true,
	starting : false,
	serverStopped : false,
}

//console.log( "protocol?", protocol );

reload.addEventListener( "click", (evt)=>{
		window.close();
 } );

home.addEventListener( "click", (evt)=>{
		window.close();
 } );

if( config ) {
	//config.local.refresh = refresh;
	//config.local.reset = reset;
	config.AddTaskList = addTaskList;
	config.addTaskLog = addTaskLog;
	config.insertBackLog = insertBackLog;
	config.AddSystem = addSystem;
	protocol.on( "addTaskList", addTaskList )
}

class AdminTaskManager {
	signs = [];
	signStatus = [];
	frame = null;
	touchSignButton = null;
	sign1StatusHolder = document.createElement( "div" );
	sign1Status = document.createElement( "span" );
	sign2StatusHolder = document.createElement( "div" );
	sign2Status = document.createElement( "span" );
	sign3StatusHolder = document.createElement( "div" );
	sign3Status = document.createElement( "span" );
	sign4StatusHolder = document.createElement( "div" );
	sign4Status = document.createElement( "span" );
    constructor() {
		//protocol.on( "")
		this.signStatus.push( this.sign1Status);	
		this.signStatus.push( this.sign2Status);	
		this.signStatus.push( this.sign3Status);	
		this.signStatus.push( this.sign4Status);	
	};

    init( frame ) {
			this.frame = frame;
			popups.makeButton( frame, "Stop Signs", AdminTaskManager.stopSigns );		
			popups.makeButton( frame, "Start Signs", AdminTaskManager.startSigns );		

			this.sign1StatusHolder.className = "task-holder";
			this.sign2StatusHolder.className = "task-holder";
			this.sign3StatusHolder.className = "task-holder";
			this.sign4StatusHolder.className = "task-holder";

			const statusBlock = document.createElement( "div" );
			statusBlock.className = "padded-block";
			frame.appendChild( statusBlock );

			statusBlock.appendChild( this.sign1StatusHolder );
			this.sign1StatusHolder.appendChild( this.sign1Status );
			this.sign1Status.textContent = "Sign 1";
			this.sign1Status.className = "yellow";
			this.sign1StatusHolder.style.display = "none";
			this.sign1StatusHolder.addEventListener( "click", (evt)=>{
				this.showTaskConfig( this.signs[0] );
			})

			statusBlock.appendChild( this.sign2StatusHolder );
			this.sign2StatusHolder.appendChild( this.sign2Status );
			this.sign2Status.textContent = "Sign 2";
			this.sign2Status.className = "yellow";
			this.sign2StatusHolder.style.display = "none";
			this.sign2StatusHolder.addEventListener( "click", (evt)=>{
				this.showTaskConfig( this.signs[1] );
			})

			statusBlock.appendChild( this.sign3StatusHolder );
			this.sign3StatusHolder.appendChild( this.sign3Status );
			this.sign3Status.textContent = "Sign 3";
			this.sign3Status.className = "yellow";
			this.sign3StatusHolder.style.display = "none";
			this.sign3StatusHolder.addEventListener( "click", (evt)=>{
				this.showTaskConfig( this.signs[2] );
			})

			{
				statusBlock.appendChild( this.sign4StatusHolder );
				this.sign4StatusHolder.appendChild( this.sign4Status );
				this.sign4Status.textContent = "Sign 4";
				this.sign4Status.className = "yellow";
				this.sign4StatusHolder.style.display = "none";
				this.sign4StatusHolder.addEventListener( "click", (evt)=>{
					this.showTaskConfig( this.signs[3] );
				})
			}

			this.touchSignButton = popups.makeButton( frame, "Fix Board", AdminTaskManager.clickSigns );
			if( !wantTouchButton ) 
				this.touchSignButton.style.display = "none";
			popups.makeButton( frame, "Restart Game Service", AdminTaskManager.restartServer );		
    }

	static resetColors( status ) {
			status.classList.remove( "green" );
			status.classList.remove( "red" );
			status.classList.remove( "orange" );
			status.classList.remove( "yellow" );
	}

	static handleStop( task ) {
		for( let s = 0; s < adminTaskManager.signs.length; s++ ) {
			const sign = adminTaskManager.signs[s];
			if( task.id === sign.id ) {
				AdminTaskManager.resetColors( adminTaskManager.signStatus[s] );
				adminTaskManager.signStatus[s].classList.add( "orange" );
			}
		}
		
	}
	static handleStopping( task ) {
		for(let  s = 0; s < adminTaskManager.signs.length; s++ ) {
			const sign = adminTaskManager.signs[s];
			if( task.id === sign.id ) {
				AdminTaskManager.resetColors( adminTaskManager.signStatus[s] );
				adminTaskManager.signStatus[s].classList.add( "yellow" );
			}
		}
	}	

	static stopSigns() {
		{
			for( let sign of adminTaskManager.signs ){
				if( !sign ) continue;
				protocol.stopTask( null, sign );
			}
		}
		local.starting = false;
		local.stopped = true;
	}
	static startSigns() {
		let running = false;
		if( !local.starting )
			for( let sign of adminTaskManager.signs ) {
				if( !sign ) continue;
				if( sign.running ) {
					running = true;
				}
			}
		if( running && local.stopped ) {
			if( !local.starting )
				setTimeout( AdminTaskManager.startSigns, 100 );
			local.starting = true;
			return;
		}
		//if( !local.starting )
			for( let sign of adminTaskManager.signs )
				if( sign && !sign.running )
					protocol.startTask( null, sign );
		local.starting = true;
		local.stopped = false;
	}
	static clickSigns() {
		if( local.clickTask )
			protocol.startTask( null, local.clickTask );
		
	}
	static restartServer() {
		loadingScreen.style.display = "";
		if( local.serverTask )
			protocol.startTask( null, local.serverTask );
		else {
			loadMessage.textContent = "Restart task not configured...";
			setTimeout( ()=>{ loadingScreen.style.display = "none"; loadMessage.textContent = "Waiting for server" }, 3000 );
		}
	}

	showTaskConfig( task ) {
		if( !this.taskConfiguration ) this.taskConfiguration = new TaskConfiguration( task );
		else this.taskConfiguration.update( task );
	}
}                       

function addSystem( system ){
}
function addTaskList( tasks ) {
	const keys = Object.keys( config.local.tasks );
	for( let key of keys ) {
		const task = config.local.tasks[key];
		if( key === QueryString.key ) {
			adminTaskManager.sign1StatusHolder.style.display = "";
			protocol.getTaskInfo( task.id ).then( info=>{
				task.info = info;
				adminTaskManager.sign1Status.textContent = info.title;
			} )
			if( task.running )
				adminTaskManager.sign1Status.className = "green";
			else
				adminTaskManager.sign1Status.className = "red";
			adminTaskManager.signs[0] = task;
			break;
		}
	}
}

function reset() {
	// no more signs actually exist; task manager disconnected
	adminTaskManager.signs.length = 0;
}

function do_refresh() {
	let s = 0;
	const controls      = [adminTaskManager.sign1Status,adminTaskManager.sign2Status,adminTaskManager.sign3Status,adminTaskManager.sign4Status];
	for( s = 0; s < adminTaskManager.signs.length; s++ ) {
		const sign = adminTaskManager.signs[s];
		if( !sign ) continue;
		const control = controls[s];
		if( sign.running )
			control.className = "green";
		else
			control.className = "red";
	}
	if( local.serverTask ) {
		// restart starts running...
		if( local.serverTask.running ) {
			local.serverStopped = true;
			loadingScreen.style.display = "";
		} else if( local.serverStopped ) { // and then when restart exits, notice that stopped is set...
			setTimeout( testServer, 2000 );
		}	
	}
}

function addTaskLog() {
}

function insertBackLog() {
}

function testServer() {
	console.log( "location?", location );
	const p = fetch( location ).then( (res)=>{
		//console.log( "success?", res );
		if( res.ok ) {
			local.serverStopped = false;
			loadingScreen.style.display = "none";
		}
	} ).catch( (err)=>{
		console.log( "Waiting for server...", err );
		setTimeout( testServer, 50 );
	} );
}

class TaskConfiguration extends Popup {
	taskInfo = {
		name:"Unset",
	}
	taskTitle = "Unset";
	current_task = null;
	current_monitor = null;
	current_select = null;
	displays = null;
	montiorFrame = null;
	monitors = []; // controls for monitors... 
	constructor(task) {
		super( "Task Settings",adminTaskManager.frame, {suffix:"-settings"} );
		this.useMouse = false;
		this.divFrame.clssName = "task-config-frame"
		this.taskInfo = task;
		
		popups.makeTextField( this, this.taskInfo, "name", "Task Name" );
		popups.makeTextField( this, this.taskInfo.info, "title", "Window Title" );

		const monitorFrame = this.monitorFrame = document.createElement( "div");
		monitorFrame.className = "monitors-frame";
		this.appendChild( monitorFrame );
		const button = popups.makeButton( this, "Done", ()=>{

			this.hide();
		} );
		button.button.className = "button monitor-confirm";
		const monitorTitle = document.createElement( "span");
		monitorTitle.className = "monitors-frame-title";
		monitorTitle.textContent = "Select Monitor"
		monitorFrame.appendChild( monitorTitle );
		this.update(task);
	}

	updateDisplays() {
		for( let monitor of this.monitors ) monitor.remove();

		protocol.getDisplays().then( displays=>{
			let top = 0;
			let left = 0;
			let bottom = 0;
			let right = 0;
			for( let monitor of displays.monitor ) {
				if( monitor.x < left ) left = monitor.x;
				if( monitor.y < top ) top = monitor.y;
				if( (monitor.x+monitor.width) > right ) right = (monitor.x+monitor.width);
				if( (monitor.y+monitor.height) > bottom ) bottom = (monitor.y+monitor.height);
			}	

			this.displays = displays;
			let monitor_number = 1;
			for( let monitor of displays.monitor ){
				const monitor_frame = document.createElement( "div" );
				monitor_frame.className = "monitor-frame"
				monitor_frame.style.position = "absolute";
				monitor_frame.style.width = (monitor.width/(right-left)*100)+"%";
				monitor_frame.style.height = (monitor.height/(bottom-top)*100)+"%";
				monitor_frame.style.left = ((monitor.x-left) / (right-left)*100)+"%";
				monitor_frame.style.top = ((monitor.y-top) / (bottom-top)*100)+"%";

				const monitor_child = document.createElement( "div" );
				monitor_child.className = "monitor-child"
				monitor_frame.appendChild( monitor_child );
				if( "moveTo" in this.taskInfo ) {
					if( "monitor" in this.taskInfo.moveTo ) {
						if( this.taskInfo.moveTo.monitor === monitor_number ) {
							this.current_monitor = monitor;
							this.current_select = monitor_child;
							monitor_child.classList.add( "green-border" );
						}
					}
					else if( "display" in this.taskInfo.moveTo ) {
						if( this.taskInfo.moveTo.display === monitor.display ) {
							this.current_monitor = monitor;
							this.current_select = monitor_child;
							monitor_child.classList.add( "green-border" );
						}
					}
					else if( this.taskInfo.moveTo.x === monitor.x 
							&& this.taskInfo.moveTo.y === monitor.y
							&& this.taskInfo.moveTo.width === monitor.width 
							&& this.taskInfo.moveTo.height === monitor.height) {
						this.current_monitor = monitor;
						this.current_select = monitor_child;
						monitor_child.classList.add( "green-border" );
					}
				}

				const monitor_info1 = document.createElement( "span" );
				monitor_info1.className = "monitor-info"
				monitor_child.appendChild( monitor_info1 );
				if( monitor.width != monitor.device.width )
					monitor_info1.textContent = monitor_number +" : " 
							+ monitor.width+"⨯" + monitor.height 
							+ "("+monitor.device.width+"⨯"+monitor.device.height+")";	
				else
					monitor_info1.textContent = monitor_number +" : " + monitor.width+"⨯" + monitor.height;	

				const monitor_info2 = document.createElement( "span" );
				monitor_info2.className = "monitor-status"
				monitor_child.appendChild( monitor_info2 );
				monitor_info2.textContent = monitor.device.monitorName + " " + (monitor.device.primary?"(Primary)":"");
				this.monitorFrame.appendChild( monitor_frame );

				monitor_frame.addEventListener( "click", ((frame,monitor,number)=>((evt)=>{
					if( monitor !== this.current_monitor ) {
						if( this.current_select )
							this.current_select.classList.remove( "green-border" );
						this.current_monitor = monitor;
						this.current_select = frame;
						this.current_select.classList.add( "green-border" );
						protocol.setTaskDisplay( this.current_task, monitor.display );
					}
				}))(monitor_child, monitor,monitor_number));

				monitor_number++;

			}
		});

	}

	async update(task) {
		this.current_task = task;
		protocol.getTaskInfo(task.id).then( info=>{
			const task = info.task;
			const keysDel = Object.keys( this.taskInfo.info.task );
			for( let key of keysDel )  delete this.taskInfo.info.task[key];
			const keys = Object.keys( task );
			for( let key of keys ){
				this.taskInfo.info.task[key] = task[key];
			}
			this.taskTitle = info.title;
			this.updateDisplays();
			this.do_refresh();
			this.show();
		});
	}
}

if( protocol ) {
	protocol.connect( taskServerAddr.replace( "http", "ws" ) );
	protocol.on( "stop", AdminTaskManager.handleStop );
	protocol.on( "stopping", AdminTaskManager.handleStopping );
}


export const adminTaskManager = protocol?new AdminTaskManager():null;
