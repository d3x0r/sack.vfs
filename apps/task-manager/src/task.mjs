
import {sack} from "sack.vfs"
import {pwdBare as pwd,config,send} from "./main.mjs";

export class Task {
	started = new Date(0);
	ended = new Date();
	running = false;
	id = sack.Id();
	name = null;

	#log = [];
	#task = null; // task definition
	#run = null;  // running service instance handle
	#ws = []; // task definition
	#resume = false;

	constructor(task) {
		this.#task = task;
		this.name = task.name;
		task.work = pwd + "/" + task.work;
		//console.log( "Making work:", task.work );
	}

	set resume(val) {
		this.#resume = val;
	}
	
	static load( task ) {
		// convert config into task.
		const newTask =   new Task( task );
		return newTask
	}

	get resume() {
		return this.#resume;
	}

	get log() {
		if( this.#log.length > 20 )
			return { at:this.#log.length-20, log:this.#log.slice( this.#log.length - 20, this.#log.length ) };
		else return { at:0, log: this.#log };
	}

	getLog( from ) {
		console.log( "reading log from:", from, from - 20, from  );
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
		const log = JSOX.stringify( {op:"log", id:this.id, log:this.log } );
		//console.log( "ws sends:", log );
		val.send( log );
		function close( code, reason ) {
			//console.log( "task websocket closed; removing self" );
			const ws = this_.#ws.findIndex( ws=>ws===val );
			if( ws > -1 ) this_.#ws.splice( ws, 1 );
			else	console.log( "untracked socket closed:", ws );
		}
	}

	start() {
			if( this.running ) {
				//console.log( "Already started:", this.#task.name );
				return;
			}
		let bin;
		if( process.platform === "linux" ) {
			if( this.#task.linbin ) bin = this.#task.linbin
			else bin = this.#task.bin; // linux will scan path for name
		}else {
			if( this.#task.winbin ) bin = this.#task.winbin
			bin = config.winroot + this.#task.bin + config.winsuffix;
		}
		if( this.#run ) this.#run.end();
		const this_ = this;
		//console.log( "Starting:", this.#task.name );
		this.#run = sack.Task( {
		  work:this.#task.work,
		  bin:bin,
		  args:this.#task.args,
		  end: stop,
			env:this.#task.env,
		  input: log,
			errorInput: log2,
		} );
		if( this.#run ) {
			this.running = true;
			this.started = new Date();
			const msg = {op:"status", id:this_.id, running: true, ended: this_.ended, started: this_.started };
			send( msg );
		}else { 
			console.log( 'failed to start?' );
		}

		function log(buffer) {
			//console.log( "Adding stdout log:", buffer );
			if( buffer.endsWith("\n" ) ) 
				buffer = buffer.slice( 0, -1 );
			const msg = { time:new Date(), error: false, line:buffer};
			// should only be sending if someone has this log open...
			this_.#send( msg );
			//console.log( this_.#task.name, ":", buffer );
		}
		function log2(buffer) {
			if( buffer.endsWith("\n" ) ) 
				buffer = buffer.slice( 0, -1 );
			const msg = { time:new Date(), error: true, line:buffer};
			// should only be sending if someone has this log open...
			this_.#send( msg );
			//console.log( this_.#task.name, ":", buffer );
		}
		function stop() { 
			this_.ended = new Date();
			this_.running = false;
			this_.#run = null;
			if( this_.#resume ) 
				if( this_.#task.restartDelay )
					setTimeout( ()=>this_.start(), this_.#task.restartDelay );
				else 
					setTimeout( ()=>this_.start(), 200 );
			//console.log( "stopped:", this_.#task.name );
			const msg = {op:"status", id:this_.id, running: false, ended: this_.ended, started: this_.started };
			send( msg );
			
      }
   }

	#send( buffer ) {	
		this.#log.push( buffer );
		if( !this.#ws.length )
			return;	
		const msg = { op:"log", id:this.id, log: buffer };
		const msg_ = JSOX.stringify( msg ) ;
		//console.log( "msg to send:", msg_ );
		this.#ws.forEach( ws=>ws.send( msg_ ) );
	}

	stop() {
		if( this.#run )
			this.#run.end();
	}
}
