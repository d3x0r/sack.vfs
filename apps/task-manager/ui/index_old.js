
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs"
import {Popup,popups} from "/node_modules/@d3x0r/popups/popups.mjs"

// <link rel="stylesheet" href="../styles.css">
const style = document.createElement( "link" );
style.rel = "stylesheet";
//style.href = "/node_modules/@d3x0r/popups/styles.css";
style.href = "/node_modules/@d3x0r/popups/dark-styles.css";
document.head.insertBefore( style, document.head.childNodes[0] || null );



const local = {
	tasks : {},
	ws : null,
	logs : {},
	taskData : null,
	display : null,
	statusDisplay : null,
	statusTimer: 0,
};

function connect() {
  	const ws = new WebSocket(location.protocol.replace( "http", "ws" )+"//"+location.host+"/", "tasks");
	local.ws = ws;
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
		local.statusDisplay.remove();
		for( let taskid in local.logs ) {
			const log = local.logs[taskid];
			log.logFrame.remove();
			log.logList.remove();
			delete local.logs[taskid];
		}
		setTimeout( ()=>connect(), 5000 );
		// websocket is closed. 
	};

	function processMessage( msg ) {
		switch( msg.op ) {
		case "tasks":
			local.taskData = msg.tasks;
			for( let task of msg.tasks )
				local.tasks[task.id] = task;
			AddTaskList()
			break;
		case "delete":
			{
				const task = local.tasks[msg.id];
				local.statusDisplay.deleteRow( task );
				for( let t = 0; t < local.taskData.length; t++ ) {
					if( local.taskData[t].id === msg.id ) {
						local.taskData.splice( t, 1 );
						break;
					}
				}
			}
			break;
		case "backlog":
			{
			const log = local.logs[msg.id];
			insertBackLog( log, msg.backlog )
			}
			break;
		case "log":
			{
				const log = local.logs[msg.id];
				const task = local.tasks[msg.id];
				if( task && !log ) {
					addTaskLog( local.tasks[msg.id], msg.log );
				} else {
					log.logFrame.show();
					//debugger;
					if( "at" in msg ) 
						;//insertBackLog( msg )
					else
						log.add( msg.log );
				}
			}
			break;
		case "status":
			
			{
				const task = local.tasks[msg.id];
				{
					task.running = msg.running;
					task.ended = msg.ended;
					task.started = msg.started;
					local.refresh();
					//console.log( "Replacing status?  need to update statuses" );
					return;
				}			
			}
			console.log( "Status for unknown task" );
			break;
		}
	}


}

class Display extends Popup {
	constructor() {
		super("Service Manager", document.body);
		
	}
}

local.display = new Display();
connect();

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


function AddTaskList() {
	const dataGrid = new popups.DataGrid( local.display, local, "taskData", {//suffix:'-browse'
		edit:false,
      columns:[ {field:"name", name:"Name", className: "name", type:{edit:false} }
		, { field: "running", name:"Status"  , className: "status"
				, type:{edit:false
						,options:[ { name:"Running", value:true,className:"task-running" }, {name:"Stopped", value:false,className:"task-stopped"} ] } }
		, { name:"Changed" , className: "started", type:{ toString(row) { 
					if( row.running ) 
						return row.started.toLocaleDateString() +" " + row.started.toLocaleTimeString() 
					else 
						return row.ended.toLocaleDateString() +" " + row.ended.toLocaleTimeString() 
			} } }
		, { field: null, name:"Run Time", className: "runtime", type:{ toString(row) {
				if( row.running ) {
             	return delTime( new Date( Date.now() - row.started.getTime() ) );
				} else
             	return delTime( new Date( Date.now() - row.ended.getTime() ) );
		    } } }
		, { name:"Show Log", className: "log", type:{click:showLog, text: "LOG ✎"} }
		, { name:"Stop"    , className: "stop", type:{click:stopTask, text: "STOP ▢"} }
		, { name:"Start"   , className: "start", type:{click:startTask, text: "PLAY ▷"} }
		, { name:"Restart" , className: "restart", type:{click:restartTask, text: "RESTART ↻"} }
		]
	} );
	
	local.statusDisplay = dataGrid;

	let visible = false;
	local.refresh = refresh;

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
		if( visible ) {
			if( local.statusTimer ) clearTimeout( local.statusTimer );
			local.statusTimer = setTimeout( ()=>{
				local.statusTimer = 0;
				refresh();
			}, 1000 );
		} else 
			local.statusTimer = 0;
	}

	function stopTask( task ) {
		local.ws.send( JSOX.stringify( {op:"stop", id:task.id } ) );
		console.log( "Should have task data?", task );
	}
	function startTask( task ) {
		local.ws.send( JSOX.stringify( {op:"start", id:task.id } ) );
		console.log( "Should have task data?", task );
	}
	function restartTask( task ) {
		local.ws.send( JSOX.stringify( {op:"restart", id:task.id } ) );
		console.log( "Should have task data?", task );
	}
	function showLog( task ) {
		local.ws.send( JSOX.stringify( {op:"log", id:task.id } ) );
		console.log( "Should have task data?", task );
	}

}

function addTaskLog( task, log ) {
	const logFrame = new Popup( task.name, local.display, { enableClose:true} );

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
