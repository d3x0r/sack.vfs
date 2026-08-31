
import {local} from "./local.mjs"
import {sack} from "sack.vfs"
import path from "path"
const JSOX = sack.JSOX;
const disk = sack.Volume();

export const config = {
	pwdBare:null, config:null, send:null, local : null
}
//import {pwdBare, config,send} from "./main.mjs";

let pendingDepends = [];

// lines of output retained per task; a long lived service would otherwise grow
// #log without bound.  Override per task with `maxLogLines` in the task config.
const DEFAULT_MAX_LOG_LINES = 5000;
const DEFAULT_MAX_MASTER_LOG_LINES = 5000;
// trim in chunks - splicing the front on every line is O(n) per line.
const LOG_TRIM_SLACK = 1024;

// how long a task gets to exit on its own before it is force-killed, and the
// hard cap after which a shutdown stops waiting for it at all rather than
// hanging the whole process on one task that will not die.
const TASK_STOP_KILL_MS = 1500;
const TASK_STOP_GIVEUP_MS = 15000;

function getLogPage( log, logBase, from = logBase + log.length, length = 20 ) {
	const lineCount = Math.max( 0, Math.floor( length ) || 0 );
	const start = Math.max( from - lineCount, logBase );
	const end   = Math.max( from, logBase );
	const atFloor = start <= logBase && logBase > 0;
	return { at: atFloor?0:start
	       , truncated: atFloor
	       , log: log.slice( start - logBase, end - logBase ) };
}

export function getMasterLog( from, length ) {
	return getLogPage( local.masterLog, local.masterLogBase, from, length );
}

function addMasterLogEntry( task, logEntry ) {
	local.masterLog.push( { taskId:task.id, taskName:task.name, log:logEntry } );
	const maxLog = config.config?.maxMasterLogLines || DEFAULT_MAX_MASTER_LOG_LINES;
	if( local.masterLog.length > maxLog + LOG_TRIM_SLACK ) {
		const drop = local.masterLog.length - maxLog;
		local.masterLog.splice( 0, drop );
		local.masterLogBase += drop;
	}
}

function taskWorkPath( work ) {
	return work ? path.resolve( config.pwdBare || process.cwd(), work ) : ( config.pwdBare || process.cwd() );
}

function resolveTaskBin( task, bin ) {
	if( !bin || !/[\\/]/.test( bin ) ) return bin;
	if( path.isAbsolute( bin ) ) return path.normalize( bin );

	const resolved = path.resolve( taskWorkPath( task.work ), bin );
	if( disk.exists( resolved ) ) return resolved;

	return bin;
}

function getPtySize( task ) {
	const ptySize = task.ptySize || task.pty || {};
	return {
		cols: ptySize.cols || ptySize.columns || 80,
		rows: ptySize.rows || ptySize.lines || 30,
		width: ptySize.width || 0,
		height: ptySize.height || 0,
	};
}

export class Task {
	started = new Date(0);
	starting = false;
	ended = new Date();
	running = false;
	failed = false;
	id = sack.Id();
	name = null;
	stopped = false;
	stopping = false;

	#autoEndBatch = false;
	#log = [];
	#logBase = 0; // lines discarded off the front of #log; `at` stays absolute
	#task = null; // task definition
	#run = null;  // running service instance handle
	#exitCode = null; // set before clearing #run
	#ws = []; // task definition
	#restart = false;
	#ranOnce = false;
	#dependsOn = [];
	#dependants = [];
	#killed = false;
	#path = null;
	#stopTimer = null;
	#stopWaiters = []; // resolvers waiting for this task to actually stop

	constructor(task) {
		this.#task = task;
		this.name = task.name;
		this.noAutoRun = task.noAutoRun;
		this.#autoEndBatch = task.autoEndBatch || false;
		if( task.moveTo ) {
			task.moveTo.cb = (yesno)=>{
				if( !yesno ) {
					console.log( "Timed out move... trying move again:", this.name );
					this.move();
				} else {
					console.log( "Moved task:", this.name );
					setTimeout( ()=>this.clickWindow(), 2000 );
				}
			}
		}
		if( task.style ) {
			task.style.cb = (stylesSet)=>{
				if( stylesSet !== 7 ) {
					console.log( "Timed out styles... trying style again:", this.name );
					this.style();
				} else {
					console.log( "Styled task:", this.name );
					//setTimeout( ()=>this.clickWindow(), 2000 );
				}
			}
		}
		//if( task.work && ( task.work[0] !== '/' && task.work[1] !== ':' )  )
		//	this.work = config.pwdBare + "/" + task.work;
		//else this.work = task.work;
		this.#restart = task.restart || false;
		if( task.prePath ) {
			if( process.platform === "win32" )
				this.#path = task.prePath + ";" + process.env.PATH;
			else
				this.#path = task.prePath + ":" + process.env.PATH;

		}
		if( task.postPath ) {
			if( process.platform === "win32" )
				this.#path = process.env.PATH + ";" + task.postPath;
			else
				this.#path = process.env.PATH + ":" + task.postPath;
		}

		if( task.dependsOn ) {
			for( let dep of task.dependsOn ) {
				let found = false;
				for( let testTask of config.local.tasks ) {
					if( testTask.name === dep ){
						testTask.#dependants.push( this );
						this.#dependsOn.push( testTask );
						found = true;
						break;
					}
				}
				if( !found ) {
					pendingDepends.push( {task:this, dep} );
				}
			}
		}
		for( let p = 0; p < pendingDepends.length; p++ ) {
			const pd = pendingDepends[p];
			if( pd.dep === this.name ) {
				pd.task.#dependsOn.push( this );
				this.#dependants.push( pd.task );
				pendingDepends.splice( p, 1 );
				p--;
			}
		}
	}

	clickWindow() {
		let x, y;
		if( "connector" in this.#task.moveTo || "display" in this.#task.moveTo || "monitor" in this.#task.moveTo) {
			const displays = sack.Task.getDisplays();
			let dev;
			// click on the connector, display or monitor the task is supposed to be at
			// otherwise use the expected position of the window to find where to click.
			for( let device of displays.device ) {
				if( device.connector === this.#task.moveTo.connector ) {
					dev = device;
				}
				else if( device.display === this.#task.moveTo.display ) {
					dev = device;
				}
				// fixup monitor links, join records.
				for( let monitor of displays.monitor ) {
					if( monitor.display === device.display ) {
						device.monitorName = device.monitor;
						device.monitor = monitor;
						monitor.device = device;
						break;
					}
				}
			}
			
			//console.log( "Dev?", dev, this.#task.moveTo.display );
			if( dev ) {
				x = dev.monitor.x + dev.monitor.width/2;
				y = dev.monitor.y + dev.monitor.width/2;
			} else {
				console.log( "Failed to match display..." );
				return;
			}
		} else {
			x = this.#task.moveTo.x + this.#task.moveTo.width/2;
			y = this.#task.moveTo.y + this.#task.moveTo.height/2;
		}
		//console.log( "Generate click after move:", this.name, x, y );
		sack.Mouse.clickAt( x, y );
	}

	get task() {
		// get the original task configuration
		return this.#task;
	}
	get title() {
		// get current task title of main window
		if( this.#run && "windowTitle" in this.#run)
			return this.#run.windowTitle();
		return "no title";
	}

	set autoEndBatch( val ) {
		// this was automatic code, but shouldn't
		// really be done, except on demenad....
		// nothing demands it.
		this.autoEndBatch = val;
	}

	get run() {
		// get run handle
		return this.#run;
	}
	get killed() {
		// get run handle
		return this.#killed;
	}

	get hasDepends() {
		return !!this.#dependsOn.length;
	}
	set restart(val) {
		if( !val ) this.#restart = val;

		if( val && this.#task.restart ) {
			// only enable restart if task configuration allows it
			this.#restart = val;
			if( this.running ) this.stop();
			else if( !this.running ) this.start();
		} else if( val && !this.running ) this.start();
	}

	get restart() {
		return this.#restart;
	}

	get log() {
		const total = this.#logBase + this.#log.length;
		if( this.#log.length > 20 )
			return { at:total-20, log:this.#log.slice( this.#log.length - 20 ) };
		else return { at:this.#logBase, log: this.#log.slice() };
	}

	// `from` is an absolute line index - the oldest line the client holds.
	// Returns the requested lines before it, clamped to what is still retained.
	// `at:0` tells the client to stop asking; `truncated` says why.
	getLog( from = this.#logBase + this.#log.length, length = 20 ) {
		//console.log( "reading log from:", from, from - lineCount, from  );
		return getLogPage( this.#log, this.#logBase, from, length );
	}

	set ws( val) {
		this.#ws.push( val );
		val.onclose = close;
		const this_ = this;
		// log is a getter that returns the tail of the log really
		const log = JSOX.stringify( {op:"log", system: local.id, id:this.id, log:this.log } );
		//console.log( "ws sends:", log );
		val.send( log );
		function close( code, reason ) {
			//console.log( "task websocket closed; removing self" );
			const ws = this_.#ws.findIndex( ws=>ws===val );
			if( ws > -1 ) this_.#ws.splice( ws, 1 );
		}
	}

	stopLog( ws ) {
		// don't send log to this socket anymore
		const wsid = this_.#ws.findIndex( val=>ws===val );
		if( wsid > -1 ) this_.#ws.splice( wsid, 1 );
	}

	get noKill() { return this.#task.noKill || false }

	set stopTimer( val ) { this.#stopTimer = val; }
	get stopTimer() { return this.#stopTimer; }
	// timeoutTaskStop() lives outside the class and needs to register itself
	get stopWaiters() { return this.#stopWaiters; }

	start() {
		this.stopped = false;
		if( this.running ) {
			console.log( "Already started:", this.#task.name );
			return;
		}
		// these track the state of a single run instance; a new #run gets a
		// clean slate, otherwise a forced kill in one run leaves #killed set
		// and timeoutTaskStop() will never escalate to kill() again.
		this.#killed = false;
		this.stopping = false;
		this.failed = false;
		if( this.#task.work && !disk.isDir( this.#task.work ) ){
			console.log( "Task not available (working path doesn't exist", this.#task.work );
			this.running = false;
			this.failed = true;
			const msg = {op:"status", id:this.id, running: false, ended: this.ended, started: this.started, failed:true };
			config.send( msg );
			return;
		}

		// set starting to prevent dependancies from starting dependants
		this.starting = true;
		for( let dep of this.#dependsOn ) {
			if( !dep.running ) dep.start();
		}
		let bin;
		if( process.platform === "linux" ) {
			bin = this.#task.bin; // linux will scan path for name
		}else if( !this.#task.bin.includes( ":" ) )
			bin = resolveTaskBin( this.#task, this.#task.bin );
		else {
			if( this.#task.altbin ) {
				if( disk.exists( this.#task.bin ) )
					bin = resolveTaskBin( this.#task, this.#task.bin );
				else bin = resolveTaskBin( this.#task, this.#task.altbin );
			} else
				bin = resolveTaskBin( this.#task, this.#task.bin ); // linux will scan path for name
		}
		if( this.#run ) this.#run.end();
		const this_ = this;
		console.log( "Starting:", this.#task.name );
		//console.log( "Starting:", bin, this );
		const env = Object.assign( {}, this.#task.env );
		if( this.#path ) env.PATH = this.#path;
		// Passing input/errorInput is what makes the launcher hand the child
		// pipes for stdout and stderr - so a `newConsole` task got a console
		// window with nothing routed to it, and cmd.exe's own banner and prompt
		// went to the log instead of the screen.  `noInheritStdio` is the switch
		// for "this task owns its stdio"; honour it by not capturing at all.
		// The cost is real and unavoidable: such a task has no Show Log output,
		// because output can go to the console or to us, not both.
		const ownStdio = !!this.#task.noInheritStdio;
		//env.PATH = this.#path;
		this.#run = sack.Task( {
		  work:this.#task.work,
		  bin:bin,
		  args:this.#task.args,
		  firstArgIsArg: this.#task.firstArgIsArg ?? true,
		  end: stop,
		  env,
		  input: ownStdio ? undefined : log,
		  errorInput: ( ownStdio || this.#task.usePty ) ? undefined : log2,
		hidden: this.#task.hidden,
		minimized: this.#task.minimized,
		maximized: this.#task.maximized,
			newGroup: this.#task.newGroup,
			noKill: this.#task.noKill,
			noWait: this.#task.noWait,
			newConsole : this.#task.newConsole,
			usePty : this.#task.usePty,
			useSignal : this.#task.useSignal,
			useBreak : this.#task.useBreak,
			moveTo : this.#task.moveTo,
			style : this.#task.style,
			noInheritStdio : this.#task.noInheritStdio,
			programName : this.#task.programName,
		} );
		//console.log( "Task:", this.#task );
		if( this.#run ) {
			if( this.#task.usePty && ( this.#task.ptySize || this.#task.pty ) && typeof this.#run.setPtySize === "function" ) {
				const ptySize = getPtySize( this.#task );
				this.#run.setPtySize( ptySize.cols, ptySize.rows, ptySize.width, ptySize.height );
			}
			this.running = true;
			this.starting = false; // is running, not just starting.
			this.started = new Date();
			const msg = {op:"status", id:this_.id, running: true, ended: this_.ended, started: this_.started };
			config.send( msg );
			for( let dep of this.#dependants ) {
				if( !dep.running && !dep.starting ) {
					console.log( "Task Started, starting Dep:", dep.name );
					dep.start();
				} else {
					console.log( "Dependant task is still running:", dep.name, dep.starting, dep.running );
				}
			}
		}else { 
			console.log( 'failed to start? try altbin?' );
		}

		function log(buffer) {
			//console.log( "Adding stdout log:", buffer );
			if( this_.#autoEndBatch ) {
				if( buffer === "Terminate batch job (Y/N)? " ) this_.#run.write( "y\n" );
			}
			// strip newlines - attempts to line gather...
			if( buffer.endsWith("\n" ) ) 
				buffer = buffer.slice( 0, -1 );

			const msg = { time:new Date(), error: false, line:buffer};
			this_.#send( msg );
			if( !this_.#task.temporary ) {
				const saneBuffer = buffer.replaceAll( '\r\r\n', '\n' ).replaceAll( "\r\n", "\n" );
				const lines = saneBuffer.split('\n' );
				for( let line of lines ) 
					console.log( this_.#task.name, ":", line );
			}
		}
		function log2(buffer) {
			//console.log( "stderr log:", buffer );
			if( buffer.endsWith("\n" ) ) 
				buffer = buffer.slice( 0, -1 );
			const msg = { time:new Date(), error: true, line:buffer};
			this_.#send( msg );
			if( !this_.#task.temporary ) {
				const saneBuffer = buffer.replaceAll( '\r\r\n', '\n' ).replaceAll( "\r\n", "\n" );
				const lines = saneBuffer.split('\n' );
				for( let line of lines ) 
					console.log( this_.#task.name, ":", line );
			}
		}
		/* this is the low level task end callback
         the native code has ended. */
		function stop() {
			this_.ended = new Date();
			this_.running = false;
			if( this_.#stopTimer) { 
				clearTimeout ( this_.#stopTimer )
				this_.#stopTimer = 0;
			} 
			// Settle everyone waiting on this task from the end event itself.
			// There is only one #stopTimer slot, so with more than one wait
			// outstanding the clearTimeout above would cancel the only armed
			// tick and orphan the promise a shutdown was waiting on.
			this_.#stopWaiters.splice( 0 ).forEach( resolve=>resolve( true ) );
			this_.stopping = false;
			/*
			if( this_.#stopTimer !== null ) {
				console.trace( "stop is clearing the stop timer...." );
				clearTimeout( this_.#stopTimer );
				this_.#stopTimer = null;
			}
			*/
			let exitCode = this_.#run?this_.#run.exitCode:this_.#exitCode;
			// exitCode can be null/undefined; a throw here would skip clearing
			// #run and the status broadcast below.
			console.log( "Task ended:", this_.name, this_.ended, exitCode
			           , (exitCode??0).toString(16) );
			this_.#ranOnce = true;
			this_.#exitCode = exitCode;
			this_.#run = null;
			for( let dep of this_.#dependants ) {
				dep.stop();
				dep.#ranOnce = false;
			}
			if( !this_.#stopTimer ) {
				if( this_.#restart ) {
					//console.log( "doing resume timeout", this_.#task.restartDelay)
					console.log( "this should restart?" );
					if( this_.#task.restartDelay )
						setTimeout( ()=>this_.start(), this_.#task.restartDelay );
					else 
						setTimeout( ()=>this_.start(), 200 );
				}
			}
			//console.log( "stopped:", this_.#task.name );
			const msg = {op:"status", id:this_.id, running: false, ended: this_.ended, started: this_.started };
			config.send( msg );
			
		}
		if( this.#task.multiStart ) {
			const sameConfig = local.tasks.find( t=>t.#task === this.#task );
			const unstarted = local.tasks.find( t=>t.#task === this.#task && !t.#run && !t.running && !t.starting );
			if( !unstarted ) {
				const noAutoRun = this.#task.noAutoRun;
				this.#task.noAutoRun = true;
				const nextTask = new Task( this.#task );
				this.#task.noAutoRun = noAutoRun;
				//config.tasks.push( task );
				local.tasks.push( nextTask );
				local.taskMap[nextTask.id] = nextTask;
				if( local.addTask ) local.addTask( nextTask.id, nextTask );
			}
		}
	}

	#send( buffer ) {
		this.#log.push( buffer );
		addMasterLogEntry( this, buffer );
		const maxLog = this.#task.maxLogLines || DEFAULT_MAX_LOG_LINES;
		if( this.#log.length > maxLog + LOG_TRIM_SLACK ) {
			const drop = this.#log.length - maxLog;
			this.#log.splice( 0, drop );
			this.#logBase += drop;
		}
		if( !this.#ws.length )
			return;
		const msg = { op:"log", system:local.id, id:this.id, log: buffer };
		const msg_ = JSOX.stringify( msg ) ;
		//console.log( "msg to send:", msg_ );
		this.#ws.forEach( ws=>ws.send( msg_ ) );
	}
	kill() {
		this.#killed = true;
		if( this.#run )
			this.#run.terminate();
	}
	stop() {
		//console.log( "Stop command: ", this.stopped, this.#run, this.stopped );
		if( this.stopped )
			// already asked to stop once - the caller still needs something to
			// wait on, and returning undefined here made closeAllTasks() fall
			// back to starting a second, competing wait loop.
			return this.running ? timeoutTaskStop( this ) : Promise.resolve( true );
		//console.trace( "STOPPED?", this.stopped, this.#run );
		if( this.#run )
			this.#run.end();
		// stop things this depends on.
		for( let dep of this.#dependants ) {
			if( dep.#run ) {
				//console.log( "dep running: ", dep.name );
				if( !dep.stopped ) {
					dep.stopped = true;
					dep.#run.end();
					timeoutTaskStop( dep );
				}
			}
			dep.#ranOnce = false;
		}
		const p = timeoutTaskStop( this );
		this.stopped = true;
		return p;
	}

	update( task ) {
		const keys = Object.keys( task );
		// update existing internal task config
		for( let key of keys ) {
			if( this.#task[key] !== task[key]){
				switch( key ) {
				case "bin":
					break;
				case "altbin":
					break;
				}
				this.#task[key] = task[key];
			}
		}
		this.noAutoRun = task.noAutoRun;
		// `name` is a public field of the Task itself - that is what gets
		// serialized into the client's task list - so writing it into #task
		// alone would leave every display on the old name until a restart.
		if( "name" in task ) this.name = task.name;
		//if( ( task.work[0] !== '/' && task.work[1] !== ':' )  )
		//	this.work = config.pwdBare + "/" + task.work;
		//else this.work = task.work;

		this.#restart = task.restart || false;
		if( task.dependsOn ) {
			if( ( "object" === typeof task.dependsOn )
			   && task.dependsOn.length ){
				// depends on more than one task...
				for( let dep of task.dependsOn )
					this.#addDep( dep );
			}else {
				this.#addDep( task.dependsOn );
			}
		}
	}
	#addDep( dep ){
		const oldTask = findTask( dep );
		if( oldTask ) {
			if( !oldTask.#dependants.find( t=>t === this )) {
				oldTask.#dependants.push( this );
			}
			this.#dependsOn.push( oldTask );
		} else {
			console.log( "Dependant task is not found:", dep, "for", this.name );
		}
	}

	move() {
		this.#run.moveWindow( this.#task.moveTo );
	}
	style() {
		this.#run.styleWindow( this.#task.style );
	}

}

function findTask( name ) {
	for( let testTask of config.local.tasks ) {
		if( testTask.name === name ){
			return testTask;
		}
	}
	return null;
}


export function terminateTasks() {
	const local = config.local;
	local.tasks.forEach( task=>{
		if (task.running && !task.noKill){
			task.restart = false;
		  	task.kill()
		};
	} );
}

export function closeAllTasks( ws ) {
	const local = config.local;
	const waits = [];

	local.tasks.forEach( task=>{
		if( task.noKill ) return;
		if (task.running){
			task.restart = false;
			// wait on the promise stop() already returns.  Pushing a second
			// timeoutTaskStop() here ran two tick loops per task through one
			// shared stopTimer slot: the first killed the task, the second only
			// ever logged "waiting for end", and the end callback then cancelled
			// the one armed tick - so this Promise.all never settled and the
			// shutdown hung with every task already reporting "Task ended:".
			waits.push( task.stop() );
		} } );

	return Promise.all( waits ).then( (waits)=>{
		//console.log( "Reply with a close?", ws );
		if( ws ) ws.close( 1000, "Tasks Stopped" );
		return waits;
	} );


}

function timeoutTaskStop( task ) {
	const started = Date.now();
	task.stopping = true;
	//console.log( "A stop started... and now we wait on", task.name );
	config.local.connections.forEach( (conn)=>
		{
			if( conn.ws.readyState == 1 ) {
				try {
					conn.ws.send( JSOX.stringify( {op:"stopping", task } ) )
				} catch(err) {
					console.log ("Send to connection error:", err );
				}
			}
			else console.log( "Connection is still in list but closed:", conn );
		});

	return new Promise( ( resolve )=>{
		// The end event is what really settles this - see Task's end callback.
		// The tick below only escalates to a kill and enforces the hard cap.
		task.stopWaiters.push( resolve );
		tick();

		function settle( stopped ) {
			const waiting = task.stopWaiters.indexOf( resolve );
			if( waiting >= 0 ) task.stopWaiters.splice( waiting, 1 );
			if( task.stopTimer ) {
				clearTimeout( task.stopTimer );
				task.stopTimer = null;
			}
			resolve( stopped );
		}

		function tick() {
			task.stopTimer = null;
			if( !task.running ) return settle( true );

			const del = Date.now() - started;
			if( del > TASK_STOP_KILL_MS && !task.killed ) {
				console.log( "Task is stubborn - forcing kill:", task.name );
				task.kill();
			}
			if( del > TASK_STOP_GIVEUP_MS ) {
				// it was killed and still has not gone; stop holding the
				// shutdown hostage to it.
				console.log( "Task will not end; stopped waiting for it:", task.name );
				return settle( false );
			}
			task.stopTimer = setTimeout( tick, 300 );
		}
	} );

}
