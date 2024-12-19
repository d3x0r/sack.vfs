
import {local} from "./local.mjs"
import {sack} from "sack.vfs"
const JSOX = sack.JSOX;
const disk = sack.Volume();

export const config = {
	pwdBare:null, config:null, send:null, local : null
}
//import {pwdBare, config,send} from "./main.mjs";

let pendingDepends = [];

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

	#log = [];
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

	constructor(task) {
		this.#task = task;
		this.name = task.name;
		this.noAutoRun = task.noAutoRun;
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
		if( "display" in this.#task.moveTo || "monitor" in this.#task.moveTo) {					
			const displays = sack.Task.getDisplays();
			let dev;
			for( let device of displays.device ) {
				if( device.display === this.#task.moveTo.display ) dev = device;
				for( let monitor of displays.monitor ) {
					if( monitor.display === device.display ) {
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
		if( this.#log.length > 20 )
			return { at:this.#log.length-20, log:this.#log.slice( this.#log.length - 20, this.#log.length ) };
		else return { at:0, log: this.#log };
	}

	getLog( from ) {
		//console.log( "reading log from:", from, from - 20, from  );
		if( from > 20 )
			return { at:from-20, log:this.#log.slice( from - 20, from  ) };
		else {
			return { at:0, log:this.#log.slice( 0, from ) };
		}
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

	start() {
		this.stopped = false;
		if( this.running ) {
			console.log( "Already started:", this.#task.name );
			return;
		}
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
			bin = this.#task.bin;
		else {
			if( this.#task.altbin ) {
				if( disk.exists( this.#task.bin ) )
					bin = this.#task.bin;
				else bin = this.#task.altbin;
			} else
				bin = this.#task.bin; // linux will scan path for name
		}
		if( this.#run ) this.#run.end();
		const this_ = this;
		console.log( "Starting:", this.#task.name );
		//console.log( "Starting:", bin, this );
		const env = Object.assign( {}, this.#task.env );
		if( this.#path ) env.PATH = this.#path;
		//env.PATH = this.#path;
		this.#run = sack.Task( {
		  work:this.#task.work,
		  bin:bin,
		  args:this.#task.args,
		  end: stop,
		  env,
		  input: log,
		  errorInput: log2,
			newGroup: this.#task.newGroup,
			noKill: this.#task.noKill,
			noWait: this.#task.noWait,
			newConsole : this.#task.newConsole,
			useSignal : this.#task.useSignal,
			useBreak : this.#task.useBreak,
			moveTo : this.#task.moveTo,
			style : this.#task.style,
			noInheritStdio : this.#task.noInheritStdio,
		} );
		//console.log( "Task:", this.#task );
		if( this.#run ) {
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
			if( buffer === "Terminate batch job (Y/N)? " ) this_.#run.write( "y\n" );
			if( buffer.endsWith("\n" ) ) 
				buffer = buffer.slice( 0, -1 );
			const msg = { time:new Date(), error: false, line:buffer};
			this_.#send( msg );
			const saneBuffer = buffer.replaceAll( '\r\r\n', '\n' ).replaceAll( "\r\n", "\n" );
			const lines = saneBuffer.split('\n' );
			for( let line of lines ) 
				console.log( this_.#task.name, ":", line );
		}
		function log2(buffer) {
			//console.log( "stderr log:", buffer );
			if( buffer.endsWith("\n" ) ) 
				buffer = buffer.slice( 0, -1 );
			const msg = { time:new Date(), error: true, line:buffer};
			this_.#send( msg );
			console.log( this_.#task.name, "|", buffer );
		}
		function stop() {
			this_.ended = new Date();
			this_.running = false;
			this_.stopping = false;
			if( this_.#stopTimer !== null ) {
				clearTimeout( this_.#stopTimer );
				this_.#stopTimer = null;
			}

			let exitCode = this_.#run?this_.#run.exitCode:this_.#exitCode;
			console.log( "Task ended:", this_.name, this_.ended, exitCode, exitCode.toString(16) );
			this_.#ranOnce = true;
			this_.#exitCode = this_.#run.exitCode;
			this_.#run = null;
			for( let dep of this_.#dependants ) {
				dep.stop();
				dep.#ranOnce = false;
			}
			if( this_.#restart ) {
				//console.log( "doing resume timeout", this_.#task.restartDelay)
				console.log( "this should restart?" );
				if( this_.#task.restartDelay )
					setTimeout( ()=>this_.start(), this_.#task.restartDelay );
				else 
					setTimeout( ()=>this_.start(), 200 );
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
		console.log( "Stop command: ", this.stopped, this.#run, this.stopped );
		if( this.stopped ) return;
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
		timeoutTaskStop( this );
		this.stopped = true;
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
			task.stop()
			waits.push( timeoutTaskStop( task ) );
		} } );
	return Promise.all( waits ).then( (waits)=>{
		console.log( "Reply with a close?", ws );
		if( ws ) ws.close( 1000, "Tasks Stopped" );
		return waits;
	} );
}

function timeoutTaskStop( task ) {
	const started = Date.now();
	task.stopping = true;
	console.log( "A stop started... and now we wait on", task.name );
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

	let resolve = null;
	function tick() {
		let del;
		task.stopTimer = null;
		if( (del=Date.now()-started) > 1500 ) {
			if( task.running ) {
				//console.log( "Still waiting for task...", task.running, task.name, Date.now() -started);
				//console.log( "Task is stubborn - forcing kill:", task.name );
				if( !task.killed ) {
					console.log( "Task is stubborn - forcing kill:", task.name );
					task.kill();
					resolve( false );
				} else console.log( "Task is stubborn - forced kill (waiting for end):", task.name );
				//config.local.connections.forEach( (conn)=>
				//	conn.ws.send( JSOX.stringify( {op:"stop", task } ) ));
			}
		}

		if( task.running ) {
			console.log( "Still running...", task.name, del );
			task.stopTimer = setTimeout( tick, 300 );
		} else resolve( true );
	}
	return new Promise( (res,rej)=>{
		resolve = res;
		tick();
	} );

}
