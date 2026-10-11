import {sack} from "sack.vfs";
import {JSOX} from "jsox"

JSOX.begin( o=>{
	console.log( "got object:", o );
} ).write( '{a:1} ' );
