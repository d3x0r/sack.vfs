
import {local} from "./local.mjs"
import {getMasterLog} from "./task.mjs"


const DEFAULT_LOG_LENGTH = 50;

function resolveRequestedTask( req ) {
	if( req.CGI.id ) {
		const task = local.taskMap[req.CGI.id];
		return task
			? { task }
			: { status:404, message:"Task Not Found", matches:[] };
	}
	if( !req.CGI.task )
		return { status:400, message:"Missing task or id", matches:[] };

	const name = req.CGI.task;
	const lowerName = name.toLowerCase();
	const exact = local.tasks.find( task=>task.name === name )
		|| local.tasks.find( task=>task.name.toLowerCase() === lowerName );
	if( exact )
		return { task:exact };

	const startsWith = local.tasks.filter( task=>task.name.toLowerCase().startsWith( lowerName ) );
	if( startsWith.length === 1 )
		return { task:startsWith[0] };
	if( startsWith.length > 1 )
		return { status:409, message:"Ambiguous Task", matches:startsWith };

	const includes = local.tasks.filter( task=>task.name.toLowerCase().includes( lowerName ) );
	if( includes.length === 1 )
		return { task:includes[0] };
	if( includes.length > 1 )
		return { status:409, message:"Ambiguous Task", matches:includes };

	return { status:404, message:"Task Not Found", matches:[] };
}

function sendTaskLookupError( req, res, result ) {
	if( getBoolArg( req, "json" ) || req.CGI.format === "json" ) {
		res.writeHead( result.status, { "Content-Type": "application/json; charset=utf-8" } );
		res.end( JSON.stringify( {
			error: result.message,
			matches: result.matches.map( task=>({ id:task.id, name:task.name }) )
		} ) + "\n" );
		return true;
	}
	res.writeHead( result.status, { "Content-Type": "text/plain; charset=utf-8" } );
	res.end( result.message + (result.matches.length
		? "\n" + result.matches.map( task=>task.name ).join( "\n" ) + "\n"
		: "\n" ) );
	return true;
}

function getBoolArg( req, ...names ) {
	for( const name of names ) {
		const val = req.CGI[name];
		if( val === undefined )
			continue;
		if( val === "" || val === true )
			return true;
		if( typeof val === "string" ) {
			const lowered = val.toLowerCase();
			return lowered !== "0" && lowered !== "false" && lowered !== "no" && lowered !== "off";
		}
		return !!val;
	}
	return false;
}

function getNumberArg( req, name, defaultValue ) {
	if( !( name in req.CGI ) )
		return defaultValue;
	const val = Number( req.CGI[name] );
	if( !Number.isFinite( val ) )
		return defaultValue;
	return val;
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
	if( showTime && isLogObject && line.time ) {
		const timestamp = formatLogTimestamp( line.time );
		if( timestamp )
			return "[" + timestamp + "] " + text;
	}
	return text;
}

function getMasterLogLineText( entry, showTime ) {
	return entry.taskName + ": " + getLogLineText( entry.log, showTime );
}

function getMasterLogEntry( entry ) {
	return {
		taskId: entry.taskId,
		taskName: entry.taskName,
		time: entry.log?.time,
		error: entry.log?.error,
		line: entry.log?.line
	};
}

export function setupRest( server ) {
	server.app.get( "/stop", (req,res)=>{
		const taskResult = resolveRequestedTask( req );
		const task = taskResult.task;
		if( task )  {
			task.stop();
			res.writeHead( 200 );
			res.end( "" );
		} else  {
			sendTaskLookupError( req, res, taskResult );
      }
			return true;
   } );


	server.app.get( "/start", (req,res)=>{
		const taskResult = resolveRequestedTask( req );
		const task = taskResult.task;
		if( task )  {
			if( task.running ) {
				task.stop();	
				function tick() {
					if( task.running ) return setTimeout(tick, 250 );
					task.start();
					res.writeHead( 200 );
					res.end( "started." );
				}
				tick();
			}else {
				task.start();
				res.writeHead( 200 );
				res.end( "started." );
			}
		} else  {
			sendTaskLookupError( req, res, taskResult );
      }
			return true;
   } );

	server.app.get( "/restart", (req,res)=>{
		const taskResult = resolveRequestedTask( req );
		const task = taskResult.task;
		console.log( "restart?", req.CGI.task || req.CGI.id, task?.running );
		if( task )  {
			if( task.running ) {
				task.stop();	
				function tick() {
					if( task.running ) return setTimeout(tick, 250 );
					task.restart = true; // setter with side effect start()
					res.writeHead( 200 );
					res.end( "set to restart." );
				}
				tick();
			}else {
				task.restart = true; // setter with side effect start()
				res.writeHead( 200 );
				res.end( "set to restart." );
			}
		} else  {
			sendTaskLookupError( req, res, taskResult );
      }
			return true;
   } );

	server.app.get( "/running", (req,res)=>{
		const taskResult = resolveRequestedTask( req );
		const task = taskResult.task;
		if( task )  {
			res.writeHead( 200 );
			if( task.running ) res.end( "true" );
			else               res.end( "false" );
		} else  {
			sendTaskLookupError( req, res, taskResult );
      }
		return true;
   } );

	server.app.get( "/list", (req,res)=>{
		const tasks = local.tasks.map( task=>({
			id: task.id,
			name: task.name,
			running: task.running,
			started: task.started,
			ended: task.ended,
			failed: task.failed
		}) );
		if( getBoolArg( req, "json" ) || req.CGI.format === "json" ) {
			res.writeHead( 200, { "Content-Type": "application/json; charset=utf-8" } );
			res.end( JSON.stringify( tasks ) + "\n" );
			return true;
		}
		res.writeHead( 200, { "Content-Type": "text/plain; charset=utf-8" } );
		res.end( tasks.map( task=>task.id + "\t" + (task.running?"running":"stopped") + "\t" + task.name ).join( "\n" ) + "\n" );
		return true;
   } );

	server.app.get( "/log", (req,res)=>{
		const at = "at" in req.CGI ? Number( req.CGI.at ) : NaN;
		const requestedLength = Math.max( 0, Math.floor( getNumberArg( req, "length", DEFAULT_LOG_LENGTH ) ) );
		const showTime = getBoolArg( req, "time", "timestamps", "showTime" );
		const wantsTaskLog = req.CGI.task || req.CGI.id;
		const taskResult = wantsTaskLog ? resolveRequestedTask( req ) : null;
		const task = taskResult?.task;
		if( wantsTaskLog && !task ) {
			return sendTaskLookupError( req, res, taskResult );
		}

		const page = task
			? task.getLog( Number.isFinite( at ) ? at : undefined, requestedLength )
			: getMasterLog( Number.isFinite( at ) ? at : undefined, requestedLength );
		const lines = task
			? page.log.map( line=>getLogLineText( line, showTime ) )
			: page.log.map( entry=>getMasterLogLineText( entry, showTime ) );
		const headers = {
			"Content-Type": "text/plain; charset=utf-8",
			"X-Task-Log-At": String( page.at || 0 ),
			"X-Task-Log-Length": String( lines.length )
		};
		if( page.truncated )
			headers["X-Task-Log-Truncated"] = "true";
		if( getBoolArg( req, "json" ) || req.CGI.format === "json" ) {
			headers["Content-Type"] = "application/json; charset=utf-8";
			res.writeHead( 200, headers );
			res.end( JSON.stringify( {
				at: page.at || 0,
				length: lines.length,
				truncated: !!page.truncated,
				log: task ? lines : page.log.map( getMasterLogEntry )
			} ) + "\n" );
			return true;
		}

		const body = lines.join( "\n" );
		res.writeHead( 200, headers );
		res.end( body + (body ? "\n" : "") );
		return true;
   } );


}
