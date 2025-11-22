
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"
import {Popup,popups} from "/node_modules/@d3x0r/popups/popups.mjs"
import {config as protocolConfig, protocol, MySystem} from "./protocol.js"

// <link rel="stylesheet" href="../styles.css">
const style = document.createElement( "link" );
style.rel = "stylesheet";
//style.href = "/node_modules/@d3x0r/popups/styles.css";
style.href = "/node_modules/@d3x0r/popups/dark-styles.css";
document.head.insertBefore( style, document.head.childNodes[0] || null );


import {local} from "./local.js"

import {TaskInfoEditor} from "./taskInfoForm.js"

protocolConfig.local = local;
protocolConfig.addTaskLog = addTaskLog;
protocolConfig.insertBackLog = insertBackLog;
protocol.on( "addTaskList", AddTaskList )
protocol.on( "addSystem", AddSystem );
protocol.on( "addTask", addTask );
protocol.on( "deleteTask", deleteTask );
protocol.on( "extern.task", addNewSystem );
protocol.on( "deleteSystem", deleteSystem );

protocol.connect();
		

class Display extends Popup {
	constructor() {
		super("Service Manager", document.body, { suffix: "-service-manager" });


		if( local.login )
			popups.makeButton( this.divCaption, "Add Task", ()=>{
				new TaskInfoEditor( null, null );
			}, {suffix:"add-task"} );
		
		local.pageFrame = new popups.PagedFrame( this, {suffix:"-system"} );
		local.pageFrame.on( "activate", (page)=>{ local.activePage = page; } );
		local.firstPage = local.pageFrame.addPage( "Master");
		local.pageFrame.activate( local.firstPage );
	}
}


function delTime(date) {
	const len = date.getTime();
	//console.log( "len:", len );
	if( len > 1000 ) {
		if( len > 60000 ) {
			if( len > 3600000 ) {
				if( len > 3600000 * 24 ) {
					return (Math.floor(len/(24*3600000))).toString() + "day(s) " 
							+ (Math.floor(len/3600000)%24).toString().padStart( 2, "0" ) + ":"
							+ (Math.floor(len/60000)%60).toString().padStart( 2, "0" ) + ":"
							+ (Math.floor(len/1000)%60).toString().padStart( 2, "0" ) + "."
							+ (len%1000).toString().padStart( 3, "0" );
				} else
					return (Math.floor(len/3600000)).toString().padStart( 2, "0" ) + ":"
							+ (Math.floor(len/60000)%60).toString().padStart( 2, "0" ) + ":"
							+ (Math.floor(len/1000)%60).toString().padStart( 2, "0" ) + "."
							+ Math.floor(len%1000).toString().padStart( 3, "0" );
			} else {
				return (Math.floor(len/60000)%60).toString().padStart( 2, "0" ) + ":"
						+ (Math.floor(len/1000)%60).toString().padStart( 2, "0" ) + "."
						+ (len%1000).toString().padStart( 3, "0" );
			}
		} else {
			return Math.floor(len/1000).toString() + "."+ (len%1000).toString().padStart( 3, "0" );
		}

	} else {
		return "0."+ len.toString().padStart( 3, "0" );
	}
}

function showLogClick(taskList,task) {
	if( local.logs[task.id] && local.logs[task.id].logFrame ){
		local.logs[task.id].logFrame.show();
	}
	else
		protocol.showLog( taskList, task )
}

function deleteTask( taskId ) {
	console.log( "Deleting task..." );
	const task = local.tasks[taskId];
	if( task ) {
		delete local.tasks[taskId];
		delete local.systemMap[taskId];
		for( let t = 0; t < local.taskData.length; t++ ) {
			if( local.taskData[t].id === taskId ) {
				local.taskData.splice( t, 1 );
				break;
			}
		}

		if( local.statusDisplay ){
			local.statusDisplay.reinit();
			local.statusDisplay.fill();
		}
	}
}


function addTask( id, task ) {
	if( local.statusDisplay ){
		local.statusDisplay.reinit();
		local.statusDisplay.fill();
	}
}

// object is 'local' field is 'tasks'
// so the datagrid takes its data from "local.tasks" which is an array of tasks.
// each task has a name, running, started, ended, and id.
function AddTaskList(display, object, field) {
	const editing = {

	}
	const columns = [ {field:"name", name:"Name", className: "name", type:{edit:false} }
		, { field: "running", name:"Status"  , className: "status"
				, type:{edit:false
						,options:[ { text:"Running", value:true,className:"task-running" }
								, {text:"Stopped", value:false,className:"task-stopped"} 
								, {text:"Failed", value:0,className:"task-failed"}] } }
		, { name:"Changed" , className: "started", type:{ toString(row) { 
					if( row.running ) 
						return row.started.toLocaleDateString() +" " + row.started.toLocaleTimeString() 
					else if( row.ended )
						return row.ended.toLocaleDateString() +" " + row.ended.toLocaleTimeString() 
					else return "unknown time";
			} } }
		, { field: null, name:"Run Time", className: "runtime", type:{ toString(row) {
				if( row.running ) {
             	return delTime( new Date( Date.now() - row.started.getTime() ) );
				} else
             	return delTime( new Date( Date.now() - row.ended.getTime() ) );
		    } } }
		, { name:"Display", className: "-display", type:{suffix:" blue", click:(gridRow)=>showTaskAdmin(object,gridRow.rowData/*task*/), text: "CONFIG ✎"} }
		, { name:"Show Log", className: "-log", type:{suffix:" blue", click:(gridRow)=>showLogClick(object,gridRow.rowData/*task*/), text: "LOG 🗎"} }
		, { name:"Stop"    , className: "-stop", type:{suffix:" red", click(gridRow){protocol.stopTask(object,gridRow.rowData)}, text: "STOP ▢"} }
		, { name:"Start"   , className: "-start", type:{suffix:" green", click(gridRow){protocol.startTask(object,gridRow.rowData)}, text: "PLAY ▷"} }
		, { name:"Restart" , className: "-restart", type:{suffix:" pumpkin", click(gridRow){protocol.restartTask(object,gridRow.rowData)}, text: "RESTART ↻"} }
		//, { name:"Edit"    , className: "edit", type:{click:protocol.editTask.bind( protocol,object), text: "Edit ✎"} }
	];
	if( local.login )
		columns.push( { name:"Edit"    , className: "-edit", type:{suffix:" purple", click: async function(gridRow) {
			const task = gridRow.rowData;
      // Define the new action or function here
			if( editing[task.id] ) return;

			const taskInfo = await protocol.getTaskInfo(task.id)
			editing[task.id] = true;
			const editor = new TaskInfoEditor( task.id, taskInfo.task );
			editor.on( "close", ()=>{ delete editing[task.id] } );

    }, text: "Edit ✎"} } );


	const dataGrid = new popups.DataGrid( display, object, field, {//suffix:'-browse'
		edit:false,
      columns } );

	let visible = false;
	if( object === local ) {
		local.statusDisplay = dataGrid;

		local.refresh = refresh;
	}

	//const el = document.getElementById("your-target-element");
	const observer = new IntersectionObserver((entries) => {
   	 if(entries[0].isIntersecting){
				
				visible = true;
				refresh();
      	   // el is visible
	    } else {
				visible = false;
   	      // el is not visible
	    }
	});

	observer.observe(dataGrid.el); // Asynchronous call

	function refresh() {
		dataGrid.refresh();
		for( let system of local.systems ){
			system.dataGrid.refresh();
		}
		if( visible ) {
			if( local.statusTimer ) clearTimeout( local.statusTimer );
			local.statusTimer = setTimeout( ()=>{
				local.statusTimer = 0;
				refresh();
			}, 1000 );
		} else 
			local.statusTimer = 0;
	}
	return dataGrid;
}

function deleteSystem( system ) {
	let oldsystem = local.systems.findIndex( testsystem=>testsystem.id=system );
	if( oldsystem>=0 ) {
		if( local.systems[oldsystem].page === local.activePage )
			local.pageFrame.activate( local.firstPage );
		local.systems[oldsystem].page.remove();
		local.systems.splice( oldsystem, 1 );
	}
}

function addNewSystem( system ) {
	AddSystem( system );
}

function AddSystem( system ) {

	let oldsystem = local.systems.find( testsystem=>testsystem.id===system.id );
	if( oldsystem ){
		oldsystem.updateTasks( system.tasks );
	} else {
		system = new MySystem( system );
		local.systems.push( system );

		const div = document.createElement( "div" );
		div.className = "System-Container";
		const page = local.pageFrame.addPage( system.system );
		page.appendChild( div );
		const label = document.createElement( "span" );
		label.className = "span-label-system-name";
		label.textContent = system.system;
		div.appendChild( label );
		system.page = page;
		for( let task of system.tasks ) {
			local.tasks[task.id] = task;
			local.systemMap[task.id] = system;
		}

		//system.pageFrame = div;
		system.dataGrid = AddTaskList( div, system, "tasks" );
		//console.log( "system datagrid became:", system.dataGrid );
	}
}

function showTaskAdmin( object, task ) {
	window.open( "adminTaskControl.html?key="+task.id, "_blank", { width: 640, height: 480 } );
}

function addTaskLog( task, log ) {
	const logFrame = new Popup( task.name, document.body, { enableClose:true} );

	const opts = {
		follow: true, 
	}
	const follow = popups.makeCheckbox( logFrame,  opts, "follow", "Follow Log" );

	const logList = document.createElement( "div" );
	const logEnd = document.createElement( "span" );
	logList.className = "task-log-listbox";
	logFrame.appendChild( logList );
	logList.appendChild( logEnd );
	logEnd.textContent = "-Load More-";

	const loadObserver = new IntersectionObserver((entries) => {
   		if(entries[0].isIntersecting){
			if( log.at ) {
			console.log( "Needs load more");
			local.ws.send( JSOX.stringify( {op:"log", id:task.id, at:log.at } ) );
			} else {
				console.log( "already have the full log should just remove this...");
			}
			//visible = true;
	    } else {
			console.log( "Load Log Hidden");
			//visible = false;
   	      // el is not visible
	    }
	});

	loadObserver.observe(logEnd); // Asynchronous call

	
	local.logs[task.id] = { logFrame, logList, logEnd, task, log, add };
	for( let lineIdx = 0; lineIdx < log.log.length; lineIdx++ ){
		const line = log.log[lineIdx];
		add( line );
	}
	console.log( "Should be setting at end of scroll...");
	logList.scrollTop = logList.scrollHeight;		


	function add( line, after ) {
		const newspan = document.createElement( "div" );
		newspan.className = "outputSpan";
		newspan.textContent = line.line;
		if( after ) 
			logList.insertBefore( newspan, logEnd );
		else
			logList.appendChild( newspan );
		//this.output.insertBefore( newspan, this.inputPrompt );
		//if( prompt ) 
		//	this.inputPrompt = newspan;
		if( follow.value ) {
			logList.scrollTop = logList.scrollHeight;		
		}
		return newspan;
		
	}

}


function insertBackLog( log, msg ) {
		console.log( "backlog insert...")
		let firstAdd = null;
		for( let lineIdx = 0; lineIdx < msg.log.length; lineIdx++ ){
			const line = msg.log[lineIdx];
			 const newline = log.add( line, log.logEnd );
			 if( !firstAdd ) firstAdd = newline;
		}
		const scrollat = log.logEnd.getBoundingClientRect();
		log.logEnd.remove();
		if( msg.at )
			log.logList.insertBefore( log.logEnd, firstAdd );
		log.log.at = msg.at;
		log.logList.scrollTop = scrollat.top;
}

local.display = new Display();
