
var sack = require( ".." );

function logOutput(buf ) {
	console.log( "TASK OUTPUT:", buf );
}

if( process.platform === "linux" )
	sack.Task( { bin: "cat", args:"testTask.js", input:logOutput } );
else {
	sack.Task( { bin: "notepad.exe", args:"testTask.js" } );
	setTimeout( ()=>{
		console.log( "TICK? fault and end program?")
	}, 5000 );
}

if(false)
setTimeout( ()=>{
	console.log( "TICK? fault and end program?")
}, 5000 );
