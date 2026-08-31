
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"
import {config as protocolConfig, protocol, MySystem} from "./protocol.js"

import {Popup,popups} from "/node_modules/@d3x0r/popups2/popups.js"
import "/node_modules/@d3x0r/popups2/controls/button.js"
import {PagedFrame} from "/node_modules/@d3x0r/popups2/controls/paged-frame.js"
import {DataGrid} from "/node_modules/@d3x0r/popups2/controls/data-grid.js"
import {Checkbox} from "/node_modules/@d3x0r/popups2/controls/checkbox.js"

import {TaskConfiguration} from "./taskConfiguration.js"

// <link rel="stylesheet" href="../styles.css">
const style = document.createElement( "link" );
style.rel = "stylesheet";
//style.href = "/node_modules/@d3x0r/popups/styles.css";
style.href = "/node_modules/@d3x0r/popups2/dark-styles.css";
document.head.insertBefore( style, document.head.childNodes[0] || null );


import {local} from "./local.js"

import {TaskInfoEditor} from "./taskInfoForm.js"
import {SimpleNotice} from "/node_modules/@d3x0r/popups2/forms/simple-notice.js"
import {PluginsEditor} from "./pluginsForm.js"

protocolConfig.local = local;
protocol.on( "insertBackLog", insertBackLog );
protocol.on( "addTaskList", AddTaskList )
protocol.on( "addSystem", AddSystem );
protocol.on( "addTask", addTask );
protocol.on( "addTaskLog", addTaskLog );
protocol.on( "updateTask", updateTask );
protocol.on( "deleteTask", deleteTask );
protocol.on( "extern.task", addNewSystem );
protocol.on( "deleteSystem", deleteSystem );
protocol.on( "login", showForm );
protocol.connect();

function showForm() {
	if( !local.display )
		local.display = new Display();
	else
		local.display.show();

	// systems are dispatched before the master task list, so replay them in
	// that order; both queues exist because the Display is not built until the
	// login resolves, which can land after the first `tasks` message.
	if( local.pendingSystems.length ) {
		local.pendingSystems.splice( 0 ).forEach( AddSystem );
	}

	if( local.pendingShowTasks.length ) {
		local.pendingShowTasks.forEach( task => {
			AddTaskList(local.firstPage, task.object, task.field ); 
		} );
	}
}		

class Display extends Popup {
	constructor() {
		super("Service Manager", document.body, { suffix: "-service-manager" });


		// "Add Task" now lives on each page (see AddTaskList) so that it can
		// target the system whose tab you are looking at; from the caption it
		// could only ever add to this service manager.
		local.pageFrame = new PagedFrame( this, {suffix:"-system"} );
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

// the server echoes the stored task definition, which has none of the live
// status fields (running/started/ended/id) - only merge what the list shows,
// or the row loses its status until the next status message.
function updateTask( taskId, task ) {
	const row = local.tasks[taskId];
	if( !row || !task ) return;
	if( "name" in task ) row.name = task.name;
	if( local.refresh ) local.refresh();
	else if( local.statusDisplay ) local.statusDisplay.refresh();
}

// Okay/Cancel before a delete: it stops the task and drops it out of the
// saved configuration, and there is no undo.
function confirmDeleteTask( group, task ) {
	const notice = new SimpleNotice( "Delete Task"
		, "Delete \"" + ( task.name || "" ) + "\"?  It is stopped if running, and removed from the saved configuration."
		, ()=>{ protocol.deleteTask( group, task ); notice.remove(); }
		// SimpleNotice runs its cancel callback for any dismissal, Okay
		// included, so this only tidies up - it must not undo anything.
		, ()=>{ notice.remove(); } );
	notice.show();
}

async function showPlugins( group ) {
	const plugins = await protocol.getPlugins( group );
	new PluginsEditor( group, plugins );
}

// Stopping a launcher takes down every task it owns and exits it - from here
// there is then no way to start it again, so it asks first.
function confirmShutdown( group ) {
	// A System's `system` is its hostname, but local.system is this manager's
	// own id - an opaque string, not something to show anyone.
	const remote = local.systems.indexOf( group ) >= 0;
	const notice = new SimpleNotice( "Stop Launcher"
		, ( remote ? "Stop all tasks on \"" + group.system + "\" and exit its service manager?"
		           : "Stop all tasks and exit this service manager?" )
		  + "  It cannot be restarted from here."
		, ()=>{ protocol.shutdownLauncher( group ); notice.remove(); }
		, ()=>{ notice.remove(); } );
	notice.show();
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
	if( !display ) display = local.firstPage;
	if( !display ) {
		local.pendingShowTasks.push( { object, field } );
		return;
	}
	// a system can refuse to be managed from here; then it gets no task editing
	// controls at all, rather than ones whose changes it would drop.
	const managed = !object.disallowUpstreamTaskManagment;
	if( local.login ) {
		// a reconnect refills this same page, so drop the button the previous
		// connection left on it rather than stacking another one.  `display` is
		// a PagedFrame page as often as an element, so hang it off the page
		// object rather than querying the DOM for it.
		if( display.taskListAdd ) display.taskListAdd.remove();
		const addFrame = document.createElement( "div" );
		addFrame.className = "task-list-add";
		display.appendChild( addFrame );
		display.taskListAdd = addFrame;
		if( managed )
			popups.makeButton( addFrame, "Add Task", ()=>{
				// `object` is this page's group: `local` for the master list, or
				// the System for an upstreamed one.
				new TaskInfoEditor( null, null, object );
			}, {suffix:"add-task"} );
		const stop = popups.makeButton( addFrame, "Stop Launcher", ()=>{
			confirmShutdown( object );
		}, {suffix:"stop-launcher"} );
		stop.tooltip = "Stop every task on this system and exit its service manager";
		if( managed ) {
			const plugins = popups.makeButton( addFrame, "Plugins", ()=>{
				showPlugins( object );
			}, {suffix:"plugins"} );
			plugins.tooltip = "Modules this service manager loads at start-up";
		}
	}
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
		, { name:"Display", className: "-display", type:{suffix:" plum", click:(gridRow)=>showTaskAdmin(object,gridRow.rowData/*task*/), text: "✎"} }
		, { name:"Show Log", className: "-log", type:{suffix:" blue", click:(gridRow)=>showLogClick(object,gridRow.rowData/*task*/), text: "🗎"} }
		, { name:"Stop"    , className: "-stop", type:{suffix:" red", click(gridRow){protocol.stopTask(object,gridRow.rowData)}, text: "▢"} }
		, { name:"Start"   , className: "-start", type:{suffix:" green", click(gridRow){protocol.startTask(object,gridRow.rowData)}, text: "▷"} }
		, { name:"Restart" , className: "-restart", type:{suffix:" pumpkin", click(gridRow){protocol.restartTask(object,gridRow.rowData)}, text: "↻"} }
		//, { name:"Edit"    , className: "edit", type:{click:protocol.editTask.bind( protocol,object), text: "Edit ✎"} }
	];
	if( local.login && managed )
		columns.push( { name:"Edit"    , className: "-edit", type:{suffix:" purple", click: async function(gridRow) {
			const task = gridRow.rowData;
      // Define the new action or function here
			if( editing[task.id] ) return;

			const taskInfo = await protocol.getTaskInfo(task.id)
			editing[task.id] = true;
			const editor = new TaskInfoEditor( task.id, taskInfo.task, object );
			editor.on( "close", ()=>{ delete editing[task.id] } );

    }, text: "✎"} } );
	if( local.login && managed )
		columns.push( { name:"Delete"  , className: "-delete", type:{suffix:" red", click(gridRow){
			confirmDeleteTask( object, gridRow.rowData );
		}, text: "🗑"} } );


	const dataGrid = new DataGrid( display, object, field, {//suffix:'-browse'
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
	// `=` here assigned the id to every system it walked, and matched the first
	let oldsystem = local.systems.findIndex( testsystem=>testsystem.id===system );
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
	if( !local.pageFrame ) {
		// the Display owns the page frame and is not built until the login
		// resolves; the first `tasks` message can carry systems before that.
		local.pendingSystems.push( system );
		return;
	}

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
	const dialog = new TaskConfiguration( local.display, task );
	// "Done" only hides it, so without this every open leaves another dialog in
	// the document; registering it also lets a dropped connection close the one
	// that is showing, along with the task editors.
	local.dialogs.add( dialog );
	dialog.on( "hide", ()=>{ local.dialogs.delete( dialog ); dialog.remove(); } );
}

function formatLogTimestamp( time ) {
	const date = time instanceof Date ? time : new Date( time );
	if( Number.isNaN( date.getTime() ) )
		return "";
	return date.getFullYear().toString().padStart( 4, "0" ) + "-"
		+ (date.getMonth()+1).toString().padStart( 2, "0" ) + "-"
		+ date.getDate().toString().padStart( 2, "0" ) + " "
		+ date.getHours().toString().padStart( 2, "0" ) + ":"
		+ date.getMinutes().toString().padStart( 2, "0" ) + ":"
		+ date.getSeconds().toString().padStart( 2, "0" ) + "."
		+ date.getMilliseconds().toString().padStart( 3, "0" );
}

function getLogLineText( line, showTime ) {
	const isLogObject = line && typeof line === "object";
	const text = isLogObject && "line" in line ? line.line : line;
	if( showTime && line && line.time ) {
		const timestamp = formatLogTimestamp( line.time );
		if( timestamp )
			return "[" + timestamp + "] " + text;
	}
	return text;
}

function renderLogLine( span, line, showTime ) {
	span.textContent = getLogLineText( line, showTime );
}

function addTaskLog( task, log ) {
	const logFrame = new Popup( task.name, document.body, { enableClose:true} );

	const opts = {
		follow: true, 
		showTime: false,
	}
	const follow = popups.makeCheckbox( logFrame,  opts, "follow", "Follow Log" );
	const showTime = popups.makeCheckbox( logFrame, opts, "showTime", "Show Time Stamps" );
	// add() chases the tail when following; backlog inserts have to opt out of
	// that or every inserted line snaps the view to the bottom.
	const state = { suspendFollow:false };

	const logList = document.createElement( "div" );
	const logEnd = document.createElement( "span" );
	logList.className = "task-log-listbox";
	logFrame.appendChild( logList );
	logList.appendChild( logEnd );
	logEnd.textContent = "-Load More-";
	showTime.on( "change", ()=>{
		for( const lineEl of logList.querySelectorAll( ".outputSpan" ) )
			renderLogLine( lineEl, lineEl.logLine, showTime.value );
	} );

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

	
	local.logs[task.id] = { logFrame, logList, logEnd, task, log, add, loadObserver, state };
	for( let lineIdx = 0; lineIdx < log.log.length; lineIdx++ ){
		const line = log.log[lineIdx];
		add( line );
	}
	console.log( "Should be setting at end of scroll...");
	logList.scrollTop = logList.scrollHeight;		


	function add( line, after ) {
		const newspan = document.createElement( "div" );
		newspan.className = "outputSpan";
		newspan.logLine = line;
		renderLogLine( newspan, line, showTime.value );
		if( after ) 
			logList.insertBefore( newspan, logEnd );
		else
			logList.appendChild( newspan );
		//this.output.insertBefore( newspan, this.inputPrompt );
		//if( prompt ) 
		//	this.inputPrompt = newspan;
		if( follow.value && !state.suspendFollow ) {
			logList.scrollTop = logList.scrollHeight;
		}
		return newspan;
		
	}

}


function insertBackLog( log, msg ) {
		// Anchor on the line that was directly under the sentinel, not on scroll
		// offsets: a live line arriving between the request and this reply calls
		// add(), which snaps to the bottom while following, so any scrollTop
		// captured here may already be somewhere else entirely.
		const anchorEl = log.logEnd.nextElementSibling;
		let firstAdd = null;
		log.state.suspendFollow = true;
		for( let lineIdx = 0; lineIdx < msg.log.length; lineIdx++ ){
			const line = msg.log[lineIdx];
			 const newline = log.add( line, log.logEnd );
			 if( !firstAdd ) firstAdd = newline;
		}
		log.state.suspendFollow = false;
		log.log.at = msg.at;
		log.logEnd.remove();
		if( msg.at && firstAdd ) {
			log.logList.insertBefore( log.logEnd, firstAdd );
			// IntersectionObserver only reports *transitions*, and it reports
			// asynchronously; the remove/insert above happens in one synchronous
			// block so it collapses to "no change" and the sentinel never reports
			// again.  Re-observing forces a fresh report - which also chains the
			// next page when the one just added didn't fill the view.
			log.loadObserver.unobserve( log.logEnd );
			log.loadObserver.observe( log.logEnd );
		}
		if( anchorEl ) {
			// leave the line we were reading one line down from the top, so the
			// last line loaded shows above it as the join.
			// clientTop skips the listbox border - scrollTop is measured from the
			// padding box, getBoundingClientRect() from the border box.
			const listTop = log.logList.getBoundingClientRect().top + log.logList.clientTop;
			const elTop   = anchorEl.getBoundingClientRect().top;
			log.logList.scrollTop += (elTop - listTop) - (anchorEl.offsetHeight||0);
		}
}

new Display();