
import {sack} from "sack.vfs";

if( process.argv.length < 2 ) {
	console.log( "Need more arguments" );
	process.exit(0);
}

if( !process.env.SELF_LOADED ) {

	const opts = { 
		work : process.cwd(),
                bin:process.argv[0],
                args:[ "--experimental-loader=../../../import.mjs" ,...(process.argv.slice(2))],
                //firstArgIsArg:true, // default true
                env:{
                	SELF_LOADED:"Yup",
                }, // extra environment.
                //binary:false, // default false
		        
		input( buffer ) {
			console.log( buffer.substr(0,buffer.length-1) );
		},
		end : process.exit,
	}

	const task = new sack.Task( opts );
	console.log( "task ran?", task );
}
