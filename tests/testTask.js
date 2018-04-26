
var sack = require( ".." );

if( Process.system === "linux" )
	sack.Task( { bin: "cat", args:"testTask.js" } );
else
	sack.Task( { bin: "notepad.exe", args:"testTask.js" } );

setTimeout( ()=>{
}, 5000 );
