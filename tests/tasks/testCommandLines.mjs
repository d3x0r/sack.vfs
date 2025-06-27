import {sack} from "sack.vfs";

const all = sack.Task.getProcessList();

const chrome = sack.Task.getProcessList( "chrom" );

console.log( "All Processes:", all );
console.log( "Chrome Instances:", chrome );