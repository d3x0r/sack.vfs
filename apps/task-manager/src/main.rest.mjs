
import {local} from "./local.mjs"


export function setupRest( server ) {
	server.app.get( "/stop", (req,res)=>{
		const stopName = req.CGI.task;
		const task = local.tasks.find( task=>task.name === stopName );
		if( task )  {
			task.stop();
			res.writeHead( 200 );
			res.end( "" );
		} else  {
			res.writeHead( 404 );
			res.end( "Task Not Found" );
      }
			return true;
   } );


	server.app.get( "/start", (req,res)=>{
		const stopName = req.CGI.task;
		const task = local.tasks.find( task=>task.name === stopName );
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
			res.writeHead( 404 );
			res.end( "Task Not Found" );
      }
			return true;
   } );

	server.app.get( "/restart", (req,res)=>{
		const stopName = req.CGI.task;
		const task = local.tasks.find( task=>task.name === stopName );
		console.log( "restart?", stopName, task?.running );
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
			res.writeHead( 404 );
			res.end( "Task Not Found" );
      }
			return true;
   } );

	server.app.get( "/running", (req,res)=>{
		const stopName = req.CGI.task;
		const task = local.tasks.find( task=>task.name === stopName );
		if( task )  {
			res.writeHead( 200 );
			if( task.running ) res.end( "true" );
			else               res.end( "false" );
		} else  {
			res.writeHead( 404 );
			res.end( "Task Not Found" );
      }
		return true;
   } );


}