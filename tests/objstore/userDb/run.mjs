
import {sack} from "sack.vfs";

if( process.argv.length < 3 ) {
	console.log( "Need more arguments" );
	process.exit(0);
}

const opts = { 
	work : process.cwd(),
        bin:process.argv[0],
        args:[ "--experimental-loader=../../../import.mjs" ,...(process.argv.slice(2))],
        //firstArgIsArg:true, // default true
        env:{
        	SACK_LOADED:"Yup",
        }, // extra environment.
        //binary:false, // default false
	        
	input: taskInput,
	end : process.exit,
}

function taskInput( buffer ) {
	console.log( buffer );
}
console.log( "Run:", opts );

const task = new sack.Task( opts );
console.log( "task ran?", task );
