/*
  Stop a service manager, from a task that the service manager runs itself.

  Configure it as an ordinary task entry, e.g.

    { name:"Shutdown"
    , bin:"node.exe"
    , args:["node_modules/sack.vfs/apps/task-manager/src/shutdown.mjs"]
    , noAutoRun:true, noKill:true, noWait:true }

  `noKill` is the one that matters: closeAllTasks() skips noKill tasks, so the
  launcher will not try to stop the very task that is asking it to stop.  Without
  it this still works - the request has already been delivered by then, and the
  launcher force-kills it after its grace period - but it costs a needless wait.

  By default this disconnects as soon as the request is sent, which is what you
  want from inside the launcher.  `--wait` instead asks the server to hold the
  socket open until every task has actually stopped (the shutdown op's `close`
  flag) and exits when it does; use that from an external script that needs to
  know the shutdown finished, the way shutdown.bat does.

  Options:
    --wait       wait for the launcher to report all tasks stopped
    --no-stop    launcher exits 0 instead of 1 (1 tells a wrapper not to loop)
    --port=N     defaults to $PORT, then 8089
    --host=NAME  defaults to $SHUTDOWN_HOST, then localhost
*/
import {sack} from "sack.vfs"
const JSOX = sack.JSOX;

const args = process.argv.slice( 2 );
function flagValue( name, fallback ) {
	const hit = args.find( arg=>arg.startsWith( name + "=" ) );
	return hit ? hit.slice( name.length + 1 ) : fallback;
}

const wait = args.includes( "--wait" );
const stop = !args.includes( "--no-stop" );
const host = flagValue( "--host", process.env.SHUTDOWN_HOST || "localhost" );
const port = flagValue( "--port", process.env.PORT || 8089 );

const target = "ws://" + host + ":" + port;
const ws = sack.WebSocket.Client( target, "tasks" );

ws.onopen = ()=>{
	// `close` makes the server hold this socket open until closeAllTasks()
	// settles, so onclose below is the completion signal.
	ws.send( JSOX.stringify( { op:"shutdown", close:wait, stop } ) );
	if( wait ) console.log( "shutdown: requested, waiting for tasks to stop..." );
	else
		// let the frame flush before going away; the socket dying along with the
		// server would also do, but not reliably before the process exits.
		setTimeout( ()=>process.exit( 0 ), 500 );
};

ws.onclose = ()=>{
	if( wait ) console.log( "shutdown: tasks stopped." );
	process.exit( 0 );
};

// a launcher that never answers should not leave this task running forever
setTimeout( ()=>{
	console.log( "shutdown: no response from", target );
	process.exit( 1 );
}, 30000 );
