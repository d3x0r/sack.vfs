
import {sack} from "sack.vfs"

let launches = 0;
sack.system.dumpMemory(true);
let tries = 0;
function start() {
	launches++;
	new sack.Task( {bin:"echo", args:["this", "is", "a", "test"], 
		zinput(msg){
			console.log( "Have to have a reader for linus?", msg );
		},
		end() {
        	start();
			tries++;
			if( tries > 10000 ) {
				sack.system.dumpMemory(true);
				tries = 0;
			}
        } } );
}

start()
